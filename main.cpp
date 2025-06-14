#include <bits/stdc++.h>
#include "Order.h"
#include "OrderBook.h"
#include "Utils.h"
#include "MatchingEngine.h"
#include "ConsoleOutput.h"
using namespace std;

void createCSVFile() {
    const string filename = "all_info.csv";

    // Try to open the file in read mode to check if it exists
    ifstream infile(filename);
    if (infile.good()) {
        cout << "File already exists. No need to create.\n";
        return;
    }

    // File does not exist; create and write header
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not create file.\n";
        return;
    }

    file << "Timestamp,Type,Details\n";
    file.close();
    cout << "File created successfully.\n";
}


int main() {
    clearConsoleLog();  // Clear previous console output
    createCSVFile();
    MatchingEngine engine;
    
    engine.loadBuyBookFromCSVtoBuyOrderBook();
    engine.loadSellBookFromCSVtoSellOrderBook();
    engine.writeOrderBookToFile();

    // Read command from file
    ifstream cmdFile("command.txt");
    if (!cmdFile.is_open()) {
        cerr << "Error: Could not open command file.\n";
        return 1;
    }

    string input;
    getline(cmdFile, input);
    cmdFile.close();

    // Clear command file
    ofstream clearCmd("command.txt", ofstream::trunc);
    clearCmd.close();

    // Process single command
    stringstream ss(input);
    string command;
    ss >> command;
    
    if (command == "PLACE") {
        string side, type;
        double price;
        int qty;
        ss >> side >> type >> price >> qty;

        if (side != "BUY" && side != "SELL") {
            writeToConsole("Invalid side. Use BUY or SELL.");
            return 1;
        }
        if (type != "LIMIT" && type != "MARKET") {
            writeToConsole("Invalid type. Use LIMIT or MARKET.");
            return 1;
        }
        engine.placeOrder(side, type, price, qty);
    }
    else if (command == "CANCEL") {
        int id;
        ss >> id;
        engine.cancelOrder(id);
    }
    else if (command == "MODIFY") {
        int id;
        string field;
        double val;
        ss >> id >> field >> val;
        engine.modifyOrder(id, field, val);
    }
    else if (command == "CLEAR") {
        clearLogs();
        writeToConsole("Logs cleared.");
        engine = MatchingEngine();
    }
    else {
        writeToConsole("Unknown command: " + command);
        return 1;
    }

    engine.writeOrderBookToFile();
    engine.writeBuyBookToCSV();
    engine.writeSellBookToCSV();
    engine.saveLastAssignedId();
    return 0;
}