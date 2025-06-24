#ifndef MATCHINGENGINE_H
#define MATCHINGENGINE_H

#include "OrderBook.h"
#include "Order.h"
#include "Utils.h"
#include "Logger.h"
#include "ConsoleOutput.h"
#include <vector>
using namespace std;

class MatchingEngine {
private:
    OrderBook orderBook;
    int orderIdCounter = loadLastAssignedId(); 

    Order* findOrderInFiles(int orderId) {
        ifstream buyFile("buy book.txt");
        string line;
        while (getline(buyFile, line)) {
            if (line.find("ID#" + to_string(orderId)) != string::npos) {
                size_t qtyPos = line.find("Qty: ") + 5;
                size_t pricePos = line.find("Price: ") + 7;
                int qty = stoi(line.substr(qtyPos, line.find(" |", qtyPos) - qtyPos));
                double price = stod(line.substr(pricePos));
                return new Order(orderId, "BUY", "LIMIT", price, qty, getCurrentTimestamp());
            }
        }

        ifstream sellFile("sell book.txt");
        while (getline(sellFile, line)) {
            if (line.find("ID#" + to_string(orderId)) != string::npos) {
                size_t qtyPos = line.find("Qty: ") + 5;
                size_t pricePos = line.find("Price: ") + 7;
                int qty = stoi(line.substr(qtyPos, line.find(" |", qtyPos) - qtyPos));
                double price = stod(line.substr(pricePos));
                return new Order(orderId, "SELL", "LIMIT", price, qty, getCurrentTimestamp());
            }
        }

        return nullptr;
    }

public:
    struct Trade {
        int buyId;
        int sellId;
        double price;
        int quantity;
        string timestamp;
    };

    struct CanceledOrder {
        Order order;
        string reason; 
        string type;    
    };

    vector<Trade> tradeLog;
    vector<CanceledOrder> canceledOrders;

    void logCanceledOrder(const Order& order, const string& reason, const string& type) {
        string entry = "ID#" + to_string(order.id) +
                    " | " + order.side + " " + order.type +
                    " | Qty: " + to_string(order.quantity) +
                    " | Reason: " + reason + " | Type: " + type;

        appendToFile("cancelledorder.txt", "ORDER CANCELED: " + entry);
        appendToFile("all_info.txt", "ORDER CANCELED: " + entry);
        appendToCSV("ORDER CANCELED", entry);
        
        canceledOrders.push_back({order, reason, type});
        writeOrderBookToFile();
    }


    void placeOrder(string side, string type, double price, int quantity) {
        string timestamp = getCurrentTimestamp();
        Order newOrder(orderIdCounter++, side, type, price, quantity, timestamp);
        
        string logEntry = "ID#" + to_string(newOrder.id) +
                       " | " + side + " " + type +
                       " | Price: " + to_string(price) +
                       " | Qty: " + to_string(quantity);

        appendToFile("order history log.txt", "ORDER PLACED: " + logEntry);
        appendToFile("all_info.txt", "ORDER PLACED: " + logEntry);
        appendToCSV("ORDER PLACED", logEntry);
        
        if (side == "BUY") {
            matchBuyOrder(newOrder);
        } else if (side == "SELL") {
            matchSellOrder(newOrder);
        }
        writeOrderBookToFile();
    }

    void matchBuyOrder(Order& buyOrder) {
        auto& sellBook = orderBook.getSellBook();
        int originalQty = buyOrder.quantity; 

        for (auto it = sellBook.begin(); it != sellBook.end();) {
            if (buyOrder.quantity == 0) break;

            if (it->first > buyOrder.price && buyOrder.type == "LIMIT") break;

            auto& sellQueue = it->second;
            while (!sellQueue.empty() && buyOrder.quantity > 0) {
                Order& sellOrder = sellQueue.front();

                int tradedQty = min(buyOrder.quantity, sellOrder.quantity);
                buyOrder.quantity -= tradedQty;
                sellOrder.quantity -= tradedQty;

                tradeLog.push_back({buyOrder.id, sellOrder.id, sellOrder.price, tradedQty, getCurrentTimestamp()});
                string entry = "BUY#" + to_string(buyOrder.id) + 
                    " <--> SELL#" + to_string(sellOrder.id) +
                    " | Price: " + to_string(sellOrder.price) +
                    " | Qty: " + to_string(tradedQty);

                appendToFile("trades.txt","TRADE: " + entry);
                appendToFile("all_info.txt", "TRADE: " + entry);
                appendToCSV("TRADE", entry);
                
                if (sellOrder.quantity == 0) {
                    sellQueue.pop_front();
                }
                writeOrderBookToFile();
            }

            if (sellQueue.empty()) {
                it = sellBook.erase(it);
            } else {
                it++;
            }
        }

        if (buyOrder.quantity > 0) {
            if (buyOrder.type == "LIMIT") {
                orderBook.addOrder(buyOrder);
            } else {
                writeToConsole("[MARKET BUY#" + to_string(buyOrder.id) + "] Partial or no match - "
                        + to_string(buyOrder.quantity) + " units canceled.");
                string reason = (buyOrder.quantity == originalQty) ? "market_unfilled" : "partial_market_unfilled";
                logCanceledOrder(buyOrder, reason, "automatic");
            }
        }

    }

    void matchSellOrder(Order& sellOrder) {
        auto& buyBook = orderBook.getBuyBook();
        int originalQty = sellOrder.quantity;


        for (auto it = buyBook.begin(); it != buyBook.end();) {
            if (sellOrder.quantity == 0) break;

            if (it->first < sellOrder.price && sellOrder.type == "LIMIT") break;

            auto& buyQueue = it->second;
            while (!buyQueue.empty() && sellOrder.quantity > 0) {
                Order& buyOrder = buyQueue.front();

                int tradedQty = min(sellOrder.quantity, buyOrder.quantity);
                sellOrder.quantity -= tradedQty;
                buyOrder.quantity -= tradedQty;

                tradeLog.push_back({buyOrder.id, sellOrder.id, buyOrder.price, tradedQty, getCurrentTimestamp()});
                string entry = "BUY#" + to_string(buyOrder.id) + 
                    " <--> SELL#" + to_string(sellOrder.id) +
                    " | Price: " + to_string(buyOrder.price) +
                    " | Qty: " + to_string(tradedQty);

                appendToFile("trades.txt","TRADE: " + entry);
                appendToFile("all_info.txt", "TRADE: " + entry);
                appendToCSV("TRADE", entry);
                
                if (buyOrder.quantity == 0) {
                    buyQueue.pop_front();
                }
                writeOrderBookToFile();
            }

            if (buyQueue.empty()) {
                it = buyBook.erase(it);
            } else {
                it++;
            }
        }

        if (sellOrder.quantity > 0) {
            if (sellOrder.type == "LIMIT") {
                orderBook.addOrder(sellOrder);
            } else {
                writeToConsole("[MARKET SELL#" + to_string(sellOrder.id) + "] Partial or no match - "
                        + to_string(sellOrder.quantity) + " units canceled.");
                string reason = (sellOrder.quantity == originalQty) ? "market_unfilled" : "partial_market_unfilled";
                logCanceledOrder(sellOrder, reason, "automatic");
            }
        }
    }

    void cancelOrder(int orderId) {
        Order* order = findOrderInFiles(orderId);
        if (!order) {
            writeToConsole("Order ID " + to_string(orderId) + " not found.");
            return;
        }

        bool orderFound = false;
        if (order->side == "BUY") {
            auto& buyBook = orderBook.getBuyBook();
            for (auto& priceLevelPair : buyBook) {
                auto& queue = priceLevelPair.second;
                for (auto it = queue.begin(); it != queue.end(); ++it) {
                    if (it->id == orderId) {
                        logCanceledOrder(*order, "user_cancel", "manual");
                        queue.erase(it);
                        orderFound = true;
                        break;
                    }
                }
                if (orderFound) {
                    if (queue.empty()) {
                        buyBook.erase(priceLevelPair.first);
                    }
                    break;
                }
            }
        } else {
            auto& sellBook = orderBook.getSellBook();
            for (auto& priceLevelPair : sellBook) {
                auto& queue = priceLevelPair.second;
                for (auto it = queue.begin(); it != queue.end(); ++it) {
                    if (it->id == orderId) {
                        logCanceledOrder(*order, "user_cancel", "manual");
                        queue.erase(it);
                        orderFound = true;
                        break;
                    }
                }
                if (orderFound) {
                    if (queue.empty()) {
                        sellBook.erase(priceLevelPair.first);
                    }
                    break;
                }
            }
        }

        if (orderFound) {
            writeToConsole("Order ID " + to_string(orderId) + " canceled.");
            delete order;
            writeOrderBookToFile();
        } else {
            writeToConsole("Order ID " + to_string(orderId) + " not found in book.");
            delete order;
        }
    }

    void modifyOrder(int orderId, string field, double value) {
        Order* oldOrder = findOrderInFiles(orderId);
        if (!oldOrder) {
            writeToConsole("Order ID " + to_string(orderId) + " not found.");
            return;
        }

        Order* newOrder = new Order(*oldOrder);
        
        if (field == "PRICE") {
            newOrder->price = value;
            string entry = "ID#" + to_string(orderId) + " | New Price: " + to_string(value);
            appendToFile("all_info.txt", "ORDER MODIFIED: " + entry);
            appendToCSV("ORDER MODIFIED", entry);
        } else if (field == "QTY") {
            newOrder->quantity = (int)value;
            string entry = "ID#" + to_string(orderId) + " | New QTY: " + to_string(value);
            appendToFile("all_info.txt", "ORDER MODIFIED: " + entry);
            appendToCSV("ORDER MODIFIED", entry);    
        } else {
            cout << "Invalid field. Use PRICE or QTY.\n";
            delete newOrder;
            return;
        }

        bool found = false;
        if (oldOrder->side == "BUY") {
            auto& buyBook = orderBook.getBuyBook();
            for (auto& priceLevelPair : buyBook) {
                auto& queue = priceLevelPair.second;
                for (auto it = queue.begin(); it != queue.end(); ++it) {
                    if (it->id == orderId) {
                        queue.erase(it);
                        found = true;
                        break;
                    }
                }
                if (found) {
                    if (queue.empty()) {
                        buyBook.erase(priceLevelPair.first);
                    }
                    break;
                }
            }
        } else {
            auto& sellBook = orderBook.getSellBook();
            for (auto& priceLevelPair : sellBook) {
                auto& queue = priceLevelPair.second;
                for (auto it = queue.begin(); it != queue.end(); ++it) {
                    if (it->id == orderId) {
                        queue.erase(it);
                        found = true;
                        break;
                    }
                }
                if (found) {
                    if (queue.empty()) {
                        sellBook.erase(priceLevelPair.first);
                    }
                    break;
                }
            }
        }

        if (!found) {
            writeToConsole("Order ID " + to_string(orderId) + " not found in book.");
            delete newOrder;
            return;
        }

        delete oldOrder;
        
        if (newOrder->side == "BUY") {
            matchBuyOrder(*newOrder);
        } else {
            matchSellOrder(*newOrder);
        }
        
        writeOrderBookToFile();
        writeToConsole("Order ID " + to_string(orderId) + " modified.");
    }
    
    void printOrderBook() {
        orderBook.printOrderBook();
    }

    void printTradeLog() {
        cout << "\n=== TRADE LOG ===\n";
        for (const auto& trade : tradeLog) {
            cout << "BUY#" << trade.buyId << " <--> SELL#" << trade.sellId
                      << " | Price: " << trade.price << " | Qty: " << trade.quantity
                      << " | Time: " << trade.timestamp << "\n";
        }
    }

    void printCanceledOrders() {
        cout << "\n=== Canceled Orders ===\n";
        for (const auto& entry : canceledOrders) {
            const auto& order = entry.order;
            cout << "ID#" << order.id 
                    << " | " << order.side << " " << order.type
                    << " | Qty: " << order.quantity 
                    << " | Price: " << order.price 
                    << " | Time: " << order.timestamp
                    << " | Type: " << entry.type
                    << " | Reason: " << entry.reason << "\n";
        }
    }

    void writeOrderBookToFile() {
        ofstream buyFile("buy book.txt"), sellFile("sell book.txt");

        buyFile << "=== BUY BOOK ===\n";
        buyFile << "BUY ORDER BOOK (Last updated: " << currentTimestamp() << ")\n";
        for (const auto& pair : orderBook.getBuyBook()) {
            double price = pair.first;
            const auto& queue = pair.second;
            for (const auto& order : queue) {
                buyFile << "ID#" << order.id << " | Qty: " << order.quantity 
                        << " | Price: " << order.price << "\n";
            }
        }

        sellFile << "=== SELL BOOK ===\n";
        sellFile << "SELL ORDER BOOK (Last updated: " << currentTimestamp() << ")\n";
        for (const auto& pair : orderBook.getSellBook()) {
            double price = pair.first;
            const auto& queue = pair.second;
            for (const auto& order : queue) {
                sellFile << "ID#" << order.id << " | Qty: " << order.quantity 
                        << " | Price: " << order.price << "\n";
            }
        }
    }

    void writeBuyBookToCSV() {
        ofstream file("buy book.csv");
        file << "ID,Side,Type,Price,Quantity,Timestamp\n";
        for (const auto& pair : orderBook.getBuyBook()) {
            double price = pair.first;
            const auto& queue = pair.second;
            for (const auto& order : queue) {
                file << order.id << ",BUY," << order.type << "," 
                     << order.price << "," << order.quantity << "," 
                     << order.timestamp << "\n";
            }
        }
    }

    void writeSellBookToCSV() {
        ofstream file("sell book.csv");
        file << "ID,Side,Type,Price,Quantity,Timestamp\n";
        for (const auto& pair : orderBook.getSellBook()) {
            double price = pair.first;
            const auto& queue = pair.second;
            for (const auto& order : queue) {
                file << order.id << ",SELL," << order.type << "," 
                     << order.price << "," << order.quantity << "," 
                     << order.timestamp << "\n";
            }
        }
    }

    void loadBuyBookFromCSVtoBuyOrderBook() {
        ifstream file("buy book.csv");
        string line;
        getline(file, line); 
        while (getline(file, line)) {
            stringstream ss(line);
            string idStr, side, type, priceStr, qtyStr, timestamp;
            getline(ss, idStr, ',');
            getline(ss, side, ',');
            getline(ss, type, ',');
            getline(ss, priceStr, ',');
            getline(ss, qtyStr, ',');
            getline(ss, timestamp);

            int id = stoi(idStr);
            double price = stod(priceStr);
            int quantity = stoi(qtyStr);

            Order order(id, side, type, price, quantity, timestamp);
            orderBook.addOrder(order);
        }
    }

    void loadSellBookFromCSVtoSellOrderBook() {
        ifstream file("sell book.csv");
        string line;
        getline(file, line);  
        while (getline(file, line)) {
            stringstream ss(line);
            string idStr, side, type, priceStr, qtyStr, timestamp;
            getline(ss, idStr, ',');
            getline(ss, side, ',');
            getline(ss, type, ',');
            getline(ss, priceStr, ',');
            getline(ss, qtyStr, ',');
            getline(ss, timestamp);

            int id = stoi(idStr);
            double price = stod(priceStr);
            int quantity = stoi(qtyStr);

            Order order(id, side, type, price, quantity, timestamp);
            orderBook.addOrder(order);
        }
    }

    void saveLastAssignedId() {
        ofstream file("last_id.txt");
        file << orderIdCounter - 1; 
    }

    int loadLastAssignedId() {
        ifstream file("last_id.txt");
        if (file.is_open()) {
            file >> orderIdCounter; 
            file.close();
        } else {
            ofstream outfile("last_id.txt");
            if (outfile.is_open()) {
                outfile << 0;
                outfile.close();
            } else {
                cerr << "Error: Unable to create last_id.txt file." << endl;
            }
            return 1;
        }
        return ++orderIdCounter;
    }   
};

#endif