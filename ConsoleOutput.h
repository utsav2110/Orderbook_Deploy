#ifndef CONSOLE_OUTPUT_H
#define CONSOLE_OUTPUT_H

#include <fstream>
#include <string>

inline void clearConsoleLog() {
    std::ofstream ofs("console_output.txt", std::ios::trunc);
    ofs.close();
}

inline void writeToConsole(const std::string& message) {
    std::ofstream file("console_output.txt", std::ios::app);
    file << message << "\n";
    file.close();
}

#endif
