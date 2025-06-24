#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <map>
#include <deque>
#include <iostream>
#include "Order.h"
using namespace std;

class OrderBook {
    
private:
    map<double, deque<Order>, greater<double>> buyBook;
    map<double, deque<Order>> sellBook;

public:
    map<double, deque<Order>, greater<double>>& getBuyBook() {
        return buyBook;
    }

    map<double, deque<Order>>& getSellBook() {
        return sellBook;
    }

    void addOrder(const Order& order) {
        if (order.side == "BUY") {
            buyBook[order.price].push_back(order);
        } else if (order.side == "SELL") {
            sellBook[order.price].push_back(order);
        }
    }

    void printOrderBook() {
        cout << "\n=== ORDER BOOK ===\n";

        cout << "\nSELL ORDERS (Price Ascending):\n";
        for (auto& pair : sellBook) {
            double price = pair.first;
            auto& orders = pair.second;
            int totalQty = 0;
            for (auto& order : orders) {
                totalQty += order.quantity;
            }
            cout << "Price: " << price << " | Qty: " << totalQty << "\n";
        }

        cout << "\nBUY ORDERS (Price Descending):\n";
        for (auto& pair : buyBook) {
            double price = pair.first;
            auto& orders = pair.second;
            int totalQty = 0;
            for (auto& order : orders) {
                totalQty += order.quantity;
            }
            cout << "Price: " << price << " | Qty: " << totalQty << "\n";
        }

        cout << "====================\n";
    }
};

#endif
