# 📘 Order Book Trading System

A C++ implementation of a **limit order book** with a **Streamlit web interface** for trading system simulation.

---

## 🌟 Features

- ⚡ Real-time order book management  
- 🎯 Support for **limit** and **market** orders  
- ⏱️ Price-time priority matching engine  
- 📝 Order modification and cancellation  
- 📚 Comprehensive logging system  
- 🖥️ Interactive web interface  
- 📊 Data visualization and analytics  

---

## 🛠️ System Components

### 🧠 Core C++ Components
- 🧾 **OrderBook**: Manages buy/sell order queues  
- 🔁 **MatchingEngine**: Handles order matching and execution  
- 🏷️ **Order**: Defines the order structure  
- 📓 **Logger**: Handles system logging  
- 🧰 **Utils**: Utility functions  

### 🌐 Web Interface (Python/Streamlit)
- 📈 Real-time order book visualization  
- 🧮 Interactive command execution  
- 📜 Trade history and analytics  
- 💾 Data export capabilities  

---

## 📋 Order Types

1. **Limit Orders** 🧷  
   - Specify price and quantity  
   - Wait in the book until matched  
   - Example: `PLACE BUY LIMIT 100 5`  

2. **Market Orders** 🚀  
   - Specify only quantity  
   - Execute immediately at best available price  
   - Example: `PLACE SELL MARKET 0 3`  

---
## 💻 Commands

```
PLACE [BUY/SELL] [LIMIT/MARKET] [PRICE] [QUANTITY]
CANCEL [ORDER_ID]
MODIFY [ORDER_ID] [PRICE/QTY] [NEW_VALUE]
CLEAR
```


---

## 📊 Data Management

### 📁 File Outputs
- `buy book.txt/csv`: Current buy orders  
- `sell book.txt/csv`: Current sell orders  
- `trades.txt`: Executed trades  
- `all_info.csv`: Complete system log  
- `console_output.txt`: Command execution results  

### 🗂️ Export Formats
- 📄 CSV  
- 📊 Excel (XLSX)  
- 📃 PDF  

---

## 🚀 Getting Started

### 📦 Prerequisites
- 🧰 C++ compiler (C++11 or later)  
- 🐍 Python 3.7+  
- 🌐 Streamlit 

### 🔧 Installation
```bash
pip install -r requirements.txt
```

### ▶️Running the System
   ```bash
   streamlit run app.py
   ```

## 🔄 **Order Matching Logic**

1. 🏷️ **Price-Time Priority**  
   - 💲 Better prices get matched first  
   - ⏰ For same price, earlier orders get priority  

2. ⚡ **Market Order Execution**  
   - 🚀 Immediate execution at best available price  
   - ✂️ Partial fills possible  

3. 🎯 **Limit Order Processing**  
   - 📘 Added to book if no immediate match  
   - 📊 Matched according to price-time priority  


## 📈 **Visualization Features**

- 📖 Real-time order book display  
- 🌊 Market depth visualization  
- 📉 Trade history charts  
- 🧮 Order type distribution analysis  
- ⚖️ Buy/Sell ratio tracking  

