import streamlit as st
import subprocess
import pandas as pd
import altair as alt
import re
from collections import defaultdict
from io import BytesIO
from fpdf import FPDF  
import zipfile
import os

def execute_command(command):
    try:
        with open("command.txt", "w") as f:
            f.write(command)

        exe_exists = os.path.exists("./orderbook")

        if not exe_exists:
            compile_result = subprocess.run(
                ["g++", "main.cpp", "-o", "orderbook"], 
                capture_output=True,
                text=True
            )
            if compile_result.returncode != 0:
                return False, f"Compilation error: {compile_result.stderr}"

        run_result = subprocess.run(
            ["./orderbook"],
            capture_output=True,
            text=True
        )
        if run_result.returncode != 0:
            return False, f"Runtime error: {run_result.stderr}"

        return True, "Command executed successfully"

    except Exception as e:
        return False, f"Error: {str(e)}"


st.set_page_config(page_title="Order Book Dashboard", layout="wide")

if st.button("🔄 Refresh Now"):
    st.rerun()

@st.cache_data(ttl=5)
def load_data():
    try:
        csv_path = "all_info.csv"
        if not os.path.exists(csv_path):
            st.error(f"File not found: {csv_path}")
            return pd.DataFrame(columns=["Timestamp", "Type", "Details"])
        
        df = pd.read_csv(csv_path, parse_dates=["Timestamp"])
        
        return df
    except Exception as e:
        st.error(f"Error loading CSV: {str(e)}")
        return pd.DataFrame(columns=["Timestamp", "Type", "Details"])

df = load_data()

with st.sidebar:
    st.header("Filters")
    selected_types = st.multiselect("Select Types", df["Type"].unique(), default=df["Type"].unique())

filtered_df = df[df["Type"].isin(selected_types)]

tab1, tab2, tab3, tab4, tab5, tab6 = st.tabs([
    "⚙️ Execute Command",   
    "📖 Book View",         
    "📈 Graphs",            
    "📋 Tables",            
    "⬇️ Download",         
    "ℹ️ About (How it works?)"  
])

st.markdown("""
    <style>
        /* Target the tab labels */
        button[data-baseweb="tab"] > div {
            font-size: 20px !important;
            font-weight: 600;
        }
    </style>
    """, unsafe_allow_html=True)



with tab2:

    st.subheader("📋 Raw Logs")
    st.dataframe(filtered_df.sort_values("Timestamp", ascending=False).set_axis(
        range(1, len(filtered_df) + 1), axis=0
    ), use_container_width=True)

    st.header("📄 Current Order Book")

    def parse_book_data(text):
        pattern = r"ID#(\d+)\s*\|\s*Qty:\s*(\d+)\s*\|\s*Price:\s*(\d+)"
        matches = re.findall(pattern, text)
        if matches:
            df = pd.DataFrame(matches, columns=["ID", "Qty", "Price"]).astype(int)
            df.index += 1  
            return df
        return pd.DataFrame(columns=["ID", "Qty", "Price"], index=pd.RangeIndex(start=1, stop=2, step=1))

    col1, col2 = st.columns(2)

    with col1:
        st.subheader("🟢 Buy Book")
        try:
            with open("buy book.txt") as f:
                st.dataframe(parse_book_data(f.read()))
        except:
            st.warning("buy book.txt not found")

    with col2:
        st.subheader("🔴 Sell Book")
        try:
            with open("sell book.txt") as f:
                st.dataframe(parse_book_data(f.read()))
        except:
            st.warning("sell book.txt not found")
    
    def parse_depth(file):
        depth = defaultdict(int)
        try:
            with open(file) as f:
                for line in f:
                    match = re.search(r"Qty: (\d+) \| Price: (\d+)", line)
                    if match:
                        qty = int(match.group(1))
                        price = int(match.group(2))
                        depth[price] += qty
        except:
            pass
        return depth

    buy_depth = parse_depth("buy book.txt")
    sell_depth = parse_depth("sell book.txt")

    st.subheader("📊 Market Depth Chart")

    if buy_depth or sell_depth:
        prices = sorted(set(buy_depth.keys()) | set(sell_depth.keys()))
        df_depth = pd.DataFrame({
            "Price": prices,
            "Buy Quantity": [buy_depth.get(p, 0) for p in prices],
            "Sell Quantity": [sell_depth.get(p, 0) for p in prices]
        })

        melted = df_depth.melt(id_vars="Price", var_name="Type", value_name="Quantity")
        chart = alt.Chart(melted).mark_bar().encode(
            x=alt.X("Price:O", title="Price"),
            y="Quantity:Q",
            color=alt.Color("Type:N", scale=alt.Scale(domain=["Buy Quantity", "Sell Quantity"], range=["green", "red"])),
            tooltip=["Price", "Type", "Quantity"]
        ).properties(title="Market Depth")

        st.altair_chart(chart, use_container_width=True)
    else:
        st.info("No depth data available.")

    trade_df = df[df["Type"].str.upper() == "TRADE"].copy()
    if not trade_df.empty:
        st.subheader("⏱️ Trades Timeline")
        st.line_chart(trade_df.groupby("Timestamp").size())


with tab3:
    st.header("📊 Market Graphs")

    st.subheader("Order Type Distribution")
    st.bar_chart(df["Type"].value_counts())

    st.subheader("🧮 Buy vs Sell Order Ratio")
    order_df = df[df["Type"].str.upper() == "ORDER PLACED"].copy()
    order_df["Side"] = order_df["Details"].str.extract(r"\|\s*(BUY|SELL)\s")[0]
    side_counts = order_df["Side"].value_counts()
    st.bar_chart(side_counts)

    st.subheader("📊 Market vs Limit Order Ratio")
    order_df = df[df["Type"].str.upper() == "ORDER PLACED"].copy()

    order_df["OrderType"] = order_df["Details"].str.extract(r"\|\s*(?:BUY|SELL)\s+(MARKET|LIMIT)\s")[0]

    order_type_counts = order_df["OrderType"].value_counts()
    st.bar_chart(order_type_counts)

    st.subheader("✏️ Order Modification Type Ratio")
    mod_df = df[df["Type"].str.upper() == "ORDER MODIFIED"].copy()
    mod_df["ModificationType"] = mod_df["Details"].apply(
        lambda x: "Price" if "New Price" in x else ("Quantity" if "New QTY" in x else "Other")
    )
    mod_counts = mod_df["ModificationType"].value_counts()
    st.bar_chart(mod_counts)

    st.subheader("🛑 Cancellation Reason and Type")

    cancel_df = df[df["Type"].str.upper() == "ORDER CANCELED"].copy()

    cancel_df["CancelReason"] = cancel_df["Details"].str.extract(r"Reason:\s*([\w_]+)")
    cancel_df["CancelType"] = cancel_df["Details"].str.extract(r"Type:\s*([\w_]+)")

    st.markdown("### 📌 Cancellation Reasons")
    reason_counts = cancel_df["CancelReason"].value_counts()
    st.bar_chart(reason_counts)

    st.markdown("### 🧾 Cancellation Types")
    type_counts = cancel_df["CancelType"].value_counts()
    st.bar_chart(type_counts)


with tab4:
    st.header("🔁 Trade & Order Activity")

    def parse_trades(df):
        df = df[df["Type"].str.upper() == "TRADE"]
        pattern = r"BUY#(\d+)\s*<-->\s*SELL#(\d+)\s*\|\s*Price:\s*([\d.]+)\s*\|\s*Qty:\s*(\d+)"
        data = []
        for _, row in df.iterrows():
            m = re.search(pattern, row["Details"])
            if m:
                b, s, p, q = m.groups()
                data.append({"Timestamp": row["Timestamp"], "Buy ID": int(b), "Sell ID": int(s), "Price": float(p), "Qty": int(q)})
        return pd.DataFrame(data,index=range(1, len(data) + 1))

    def parse_order_placed(df):
        df = df[df["Type"].str.upper() == "ORDER PLACED"]
        pattern = r"ID#(\d+)\s*\|\s*(BUY|SELL)\s+(LIMIT|MARKET)?\s*\|\s*Price:\s*([\d.]+)\s*\|\s*Qty:\s*(\d+)"
        data = []
        for _, row in df.iterrows():
            m = re.search(pattern, row["Details"])
            if m:
                id_, side, otype, price, qty = m.groups()
                data.append({
                    "Timestamp": row["Timestamp"],
                    "ID": int(id_),
                    "Side": side,
                    "Order Type": otype or "UNKNOWN",
                    "Price": float(price),
                    "Qty": int(qty)
                })
        return pd.DataFrame(data,index=range(1, len(data) + 1))

    col1, col2 = st.columns(2)

    with col1:
        st.subheader("📄 Orders Placed")
        st.dataframe(parse_order_placed(df), use_container_width=True)

    with col2:
        st.subheader("⚖️ Trades")
        st.dataframe(parse_trades(df), use_container_width=True)
        

    st.header("⚙️ Order Logs")

    def parse_modifications(df):
        df = df[df["Type"].str.upper() == "ORDER MODIFIED"]
        pattern = r"ID#(\d+)\s*\|\s*New (Price|QTY):\s*([\d.]+)"
        data = []
        for _, row in df.iterrows():
            m = re.search(pattern, row["Details"])
            if m:
                id_, field, value = m.groups()
                data.append({
                    "Timestamp": row["Timestamp"],
                    "ID": int(id_),
                    "Modified Field": field.upper(),
                    "New Value": float(value)
                })
        return pd.DataFrame(data,index=range(1, len(data) + 1))

    def parse_cancellations(df):
        df = df[df["Type"].str.upper() == "ORDER CANCELED"]
        pattern = r"ID#(\d+)\s*\|\s*(BUY|SELL)\s+(LIMIT|MARKET)?\s*\|\s*Qty:\s*(\d+)\s*\|\s*Reason:\s*(\w+)\s*\|\s*Type:\s*(\w+)"
        data = []
        for _, row in df.iterrows():
            m = re.search(pattern, row["Details"])
            if m:
                id_, side, otype, qty, reason, ctype = m.groups()
                data.append({
                    "Timestamp": row["Timestamp"],
                    "ID": int(id_),
                    "Side": side,
                    "Order Type": otype or "UNKNOWN",
                    "Qty": int(qty),
                    "Reason": reason,
                    "Cancel Type": ctype
                })
        return pd.DataFrame(data,index=range(1, len(data) + 1))

    st.subheader("✏️ Modified Orders")
    st.dataframe(parse_modifications(df), use_container_width=True)

    st.subheader("❌ Canceled Orders")
    st.dataframe(parse_cancellations(df), use_container_width=True)

with tab5:

    tables = {
        "Orders": parse_order_placed(df),
        "Trades": parse_trades(df),
        "Modifications": parse_modifications(df),
        "Cancellations": parse_cancellations(df),
        "Raw Logs": filtered_df,
    }

    books = {}
    try:
        with open("buy book.txt") as f:
            books["Buy Book"] = f.read()
    except FileNotFoundError:
        st.warning("buy book.txt not found")

    try:
        with open("sell book.txt") as f:
            books["Sell Book"] = f.read()
    except FileNotFoundError:
        st.warning("sell book.txt not found")

    for book_name, book_text in books.items():
        tables[book_name] = parse_book_data(book_text)

    st.header("📦 Download Data Archives")

    def create_zip_csv():
        zip_buf = BytesIO()
        with zipfile.ZipFile(zip_buf, "w") as zip_file:
            for name, df_table in tables.items():
                zip_file.writestr(f"{name}.csv", df_table.to_csv(index=False))
        zip_buf.seek(0)
        return zip_buf.getvalue()

    def create_zip_xlsx():
        zip_buf = BytesIO()
        with zipfile.ZipFile(zip_buf, "w") as zip_file:
            for name, df_table in tables.items():
                excel_buf = BytesIO()
                df_table.to_excel(excel_buf, index=False, engine="xlsxwriter")
                zip_file.writestr(f"{name}.xlsx", excel_buf.getvalue())
        zip_buf.seek(0)
        return zip_buf.getvalue()

    def create_zip_pdf():
        zip_buf = BytesIO()
        with zipfile.ZipFile(zip_buf, "w") as zip_file:
            for name, df_table in tables.items():
                orientation = "L" if name == "Raw Logs" else "P"
                pdf = FPDF(orientation=orientation)
                pdf.add_page()
                pdf.set_font("Arial", size=10)

                total_width = pdf.w - 20  
                col_ratios = []

                if name == "Raw Logs":
                    col_ratios = [1, 1, 2]
                elif name == "Orders" and len(df_table.columns) >= 2:
                    col_ratios = [1.5, 0.5] + [1] * (len(df_table.columns) - 2)
                elif name == "Cancellations" and len(df_table.columns) == 7:
                    col_ratios = [1.5, 0.5, 0.5, 1, 0.5, 2, 1]
                else:
                    col_ratios = [1] * len(df_table.columns)

                ratio_sum = sum(col_ratios)
                col_widths = [total_width * (r / ratio_sum) for r in col_ratios]
                row_height = pdf.font_size + 2

                
                for i, col in enumerate(df_table.columns):
                    pdf.cell(col_widths[i], row_height, str(col), border=1)
                pdf.ln(row_height)

            
                for _, row in df_table.iterrows():
                    for i, item in enumerate(row):
                        pdf.cell(col_widths[i], row_height, str(item), border=1)
                    pdf.ln(row_height)

                pdf_bytes = pdf.output(dest="S").encode("latin-1")
                zip_file.writestr(f"{name}.pdf", pdf_bytes)

        zip_buf.seek(0)
        return zip_buf.getvalue()

    col1, col2, col3 = st.columns(3)

    with col1:
        if st.button("⬇️ Download All CSV ZIP"):
            csv_zip = create_zip_csv()
            st.download_button("Download CSV ZIP", csv_zip, "all_csv_files.zip", mime="application/zip")

    with col2:
        if st.button("⬇️ Download All XLSX ZIP"):
            xlsx_zip = create_zip_xlsx()
            st.download_button("Download XLSX ZIP", xlsx_zip, "all_xlsx_files.zip", mime="application/zip")

    with col3:
        if st.button("⬇️ Download All PDF ZIP"):
            pdf_zip = create_zip_pdf()
            st.download_button("Download PDF ZIP", pdf_zip, "all_pdf_files.zip", mime="application/zip")


def read_console_output():
    try:
        with open("console_output.txt", "r") as f:
            return f.read().strip()
    except FileNotFoundError:
        return ""
    
    
with tab1: 
    st.header("🎮 Command Input")
        
    if "command_type" not in st.session_state:
        st.session_state.command_type = "PLACE"

    command_type = st.selectbox(
        "Command Type",
        ["PLACE", "CANCEL", "MODIFY", "CLEAR"],
        key="command_type"
    )

    command = None

    if command_type == "PLACE":
        col1, col2, col3, col4 = st.columns(4)
        with col1:
            side = st.selectbox("Side", ["BUY", "SELL"], key="place_side")
        with col2:
            order_type = st.selectbox("Type", ["LIMIT", "MARKET"], key="place_type")
        with col4:
            price = 0 if order_type == "MARKET" else st.number_input(
                "Price", 
                min_value=10, 
                step=1, 
                key="place_price",
                disabled=(order_type == "MARKET")
            )
        with col3:
            quantity = st.number_input("Quantity", min_value=1, step=1, key="place_qty")
        command = f"PLACE {side} {order_type} {price} {quantity}"

    elif command_type == "CANCEL":
        order_id = st.number_input("Order ID", min_value=1, step=1, key="cancel_id")
        command = f"CANCEL {order_id}"

    elif command_type == "MODIFY":
        col1, col2, col3 = st.columns(3)
        with col1:
            order_id = st.number_input("Order ID", min_value=1, step=1, key="modify_id")
        with col2:
            field = st.selectbox("Field", ["PRICE", "QTY"], key="modify_field")
        with col3:
            if field == "QTY":
                value = st.number_input("New Quantity", min_value=1, step=1, key="modify_qty")
            else:
                value = st.number_input("New Price", min_value=10, step=1, key="modify_price")
        command = f"MODIFY {order_id} {field} {value}"

    else:  
        st.info("This will clear all orders and logs. Admin password required.")
        password = st.text_input("Admin Password", type="password", key="admin_password")
        correct_password = st.secrets.admin.password
        if password:
            if password == correct_password:
                st.success("Password correct! You can execute CLEAR command.")
                command = f"CLEAR {password}"
            else:
                st.error("Invalid password!")
                command = None
        else:
            command = None
            st.warning("Please enter admin password")

    if st.button("Execute Command", disabled=(command_type == "CLEAR" and command is None)):
        if not command:
            st.error("Please fill all required fields")
        else:
            success, message = execute_command(command)
            if success:
                st.success(f"Command executed successfully")
                new_output = read_console_output()
                if new_output:
                    st.info(new_output)
            else:
                st.error(message)

with tab6:
    st.markdown("""
    # 📚 Understanding Order Book Trading System

    ## 📖 What is an Order Book?
    An order book is the heart of a trading system that keeps track of all buy and sell orders for a specific asset. It consists of two main parts:
    - 🟢 **Buy Orders (Bids)**: Orders to purchase at specified prices (Arranged in descending order - highest price first)
    - 🔴 **Sell Orders (Asks)**: Orders to sell at specified prices (Arranged in ascending order - lowest price first)

    ---

    ## 📊 Order Book Arrangement

    ### 🟩 1. Buy Book (Bids):
    - 🔽 Sorted in **descending** order by price  
    - 🥇 Highest buy price appears at the top  
    - 👍 Better for buyers as they get the best offers first

    ```
    BUY ORDERS:
    💰 105 - 10 units
    💰 103 - 5 units
    💰 100 - 15 units
    ```

    ### 🟥 2. Sell Book (Asks):
    - 🔼 Sorted in **ascending** order by price  
    - 🥇 Lowest sell price appears at the top  
    - 👍 Better for sellers as they get exposure to buyers first

    ```
    SELL ORDERS:
    💸 110 - 8 units
    💸 112 - 12 units
    💸 115 - 3 units
    ```

    ---

    ## 🛠️ Types of Orders

    ### 1️⃣ LIMIT Orders
    - 🧾 You specify both **price** and **quantity**
    - ⏳ Order waits in the book until matched
    - 🧪 Example: `PLACE BUY LIMIT 100 5`  
      _(Buy 5 units at price 100)_

    ### 2️⃣ MARKET Orders
    - 🎯 You only specify **quantity**, price is automatic
    - ⚡ Executes immediately at best available price
    - 🧪 Example: `PLACE SELL MARKET 0 3`  
      _(Sell 3 units at market price)_

    ---

    ## 💬 Basic Commands

    ```
    🛒 1. Place Order:
       PLACE [BUY/SELL] [LIMIT/MARKET] [PRICE] [QUANTITY]
       Example: PLACE BUY LIMIT 100 5

    ❌ 2. Cancel Order:
       CANCEL [ORDER_ID]
       Example: CANCEL 123

    ✏️ 3. Modify Order:
       MODIFY [ORDER_ID] [PRICE/QTY] [NEW_VALUE]
       Example: MODIFY 123 PRICE 105

    🧹 4. Clear All:
       CLEAR
    ```

    ---

    ## 🧪 Simple Example Scenario

    1. 👩 Alice places buy order:
       ```
       PLACE BUY LIMIT 100 5
       # Creates order to buy 5 units at price 100
       ```

    2. 👨‍🦱 Bob places sell order:
       ```
       PLACE SELL LIMIT 100 3
       # Creates order to sell 3 units at price 100
       ```

    3. ✅ Result:
       - ✅ Trade executes for 3 units at price 100
       - 🕒 Alice's remaining order (2 units) stays in the book

    ---

    ## ⚖️ Order Matching Rules

    1. 🕐 **Price-Time Priority**:
       - 💵 Better prices get matched first
       - ⏱️ For same price, earlier orders get priority

    2. ⚡ **Market Orders**:
       - Always execute immediately
       - 🧲 Take the best available price

    3. 🎯 **Limit Orders**:
       - Must specify price
       - Only execute if price conditions are met

    ---

    ## 🧠 Key Takeaway
                
    The order book ensures **transparency**, **fairness**, and **efficiency** in matching buyers and sellers. Mastering it is essential for serious traders. 📈
   
    ---
    ## 📄 Test Cases
                
    If you want testcase examples, you can refer to the `testcases.txt` file in the repository. It contains various scenarios to help you understand how the system behaves under different conditions.

    ---
    """)
