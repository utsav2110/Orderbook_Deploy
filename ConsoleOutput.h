#ifndef CONSOLE_OUTPUT_H
#define CONSOLE_OUTPUT_H

#include <fstream>
#include <string>
using namespace std;

inline void clearConsoleLog() {
    ofstream ofs("console_output.txt", ios::trunc);
    ofs.close();
}

inline void writeToConsole(const string& message) {
    ofstream file("console_output.txt", ios::app);
    file << message << "\n";
    file.close();
}

#endif
