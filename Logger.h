#pragma once
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <ctime>
using namespace std;

inline string currentTimestamp() {
    auto now = chrono::system_clock::now();
    time_t now_c = chrono::system_clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

inline void appendToFile(const string& filename, const string& content) {
    ofstream file(filename, ios::app);  
    file << "[" << currentTimestamp() << "] " << content << "\n";
}

inline void clearLogs() {
    vector<string> filenames = {
        "trades.txt",
        "cancelledorder.txt",
        "buy book.txt",
        "sell book.txt",
        "order history log.txt",
        "all_info.txt",
        "all_info.csv",
        "buy book.csv",
        "sell book.csv",
        "last_id.txt",
        "console_output.txt"  
    };

    
    for (const auto& file : filenames) {
        ofstream ofs(file, ios::trunc); 

        if (!ofs.is_open()) {
            cerr << "Error: Could not open file " << file << " for writing.\n";
            continue;
        }

        if (file == "all_info.csv") {
            ofs << "Timestamp,Type,Details\n";
        } else if (file == "buy book.csv" || file == "sell book.csv") {
            ofs << "ID,Side,Type,Price,Quantity,Timestamp\n";
        } else if (file == "last_id.txt") {
            ofs << "0";  
        } else{
            ofs << ""; 
        }
    }
}

inline void appendToCSV(const string& logType, const string& details) {
    ofstream file("all_info.csv", ios::app); 
    file << "\"" << currentTimestamp() << "\","
         << "\"" << logType << "\","
         << "\"" << details << "\"\n";
}
