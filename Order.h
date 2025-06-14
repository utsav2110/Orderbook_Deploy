#ifndef ORDER_H

#define ORDER_H
#include <string>
using namespace std;

struct Order{
    int id;
    string side;  // "BUY" or "SELL"
    string type;  // "LIMIT" or "MARKET"
    double price; // Only used for LIMIT
    int quantity;
    string timestamp; // For time priority

    Order(int id, string side, string type, double price, int quantity, string timestamp)
    : id(id),
      side(side),
      type(type),
      price(price),
      quantity(quantity),
      timestamp(timestamp) {}
};

#endif