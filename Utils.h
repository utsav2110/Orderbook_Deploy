#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
using namespace std;

inline string getCurrentTimestamp() {
    auto now = chrono::system_clock::now();
    time_t rawTime = chrono::system_clock::to_time_t(now);
    tm* timeInfo = localtime(&rawTime);
    ostringstream oss;
    oss << put_time(timeInfo, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

#endif