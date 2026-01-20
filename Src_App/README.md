# 📈 Stock Market Trading System - Simulation

## 📋 Description

This project is a **complete stock market trading system simulation** in C++ with client-server architecture. It implements a financial market with order management, secure authentication, and data persistence via SQLite.

The system simulates the real operation of a stock market with:
- **Trading sessions**: fixing periods and continuous trading
- **Complex orders**: market, limit, stop and limit-stop
- **Portfolio management**: tracking of stocks and client balances
- **Secure authentication**: AES encryption of passwords
- **Graphical interface**: visualization with SDL2

---

## 🏗️ Project Architecture

### 🔧 Main Components

#### **Server (`server.cpp`)**
- Multiple client connection management (multi-threading)
- Order processing and transaction execution
- Market synchronization (fixing and continuous trading phases)
- Authentication and session management

#### **Client (`client_account.cpp`)**
- User interface to connect to the server
- Buy/sell order submission
- Portfolio and order consultation
- Server connection monitoring

#### **Market (`market.hpp/cpp`)**
- Centralized stock market management
- Order accumulation and processing
- Matching algorithm between buyers and sellers
- Equilibrium price calculation

#### **Client (`client.hpp/cpp`)**
- Client account representation
- Balance and stock portfolio management
- Order history (pending and completed)

#### **Order (`order.hpp/cpp`)**
- Trading order definition
- Types: BUY, SELL
- Triggers: MARKET, LIMIT, STOP, LIMIT_STOP
- Quantity and price management

#### **Action (`action.hpp/cpp`)**
- Stock representation
- Price history
- Available quantity information

#### **Database Management (`database_management.hpp/cpp`)**
- SQLite3 interface
- Persistence of clients, stocks, orders and messages
- Transaction and SQL query management

#### **Messages (`messages.hpp/cpp`)**
- Event logging system
- Message types: authentication, transactions, market phases
- Operation traceability

#### **Graphic (`graphic.hpp/cpp`)**
- Graphical interface with SDL2
- Market and data visualization

#### **Utility (`utility.hpp/cpp`)**
- Utility functions
- Time management
- AES encryption for passwords
- Global constants

---

## 📦 Dependencies

The project requires the following libraries:

- **C++20**: compiler with C++20 support
- **SQLite3**: embedded database
- **OpenSSL**: AES password encryption
- **SDL2**: main graphics library
- **SDL2_ttf**: text rendering
- **SDL2_image**: image management
- **fmt**: modern string formatting
- **POSIX Sockets**: network communication

### Dependency Installation (macOS with Homebrew)

```bash
brew install sqlite3
brew install openssl@3
brew install sdl2
brew install sdl2_ttf
brew install sdl2_image
brew install fmt
```

---

## 🔨 Compilation

The project uses a **Makefile** for compilation.

### Compile all executables

```bash
make
```

This generates:
- `server.x`: stock market server
- `client_account.x`: client to connect to the market

### Compile server only

```bash
make server.x
```

### Compile client only

```bash
make client_account.x
```

### Clean object files

```bash
make clean
```

### Full cleanup (objects + executables)

```bash
make realclean
```

---

## 🚀 Usage

### 1️⃣ Launch the server

```bash
./server.x
```

The server:
- Listens on port **8080**
- Initializes the SQLite database
- Waits for client connections
- Manages different market phases

### 2️⃣ Launch a client

```bash
./client_account.x <username> <password>
```

Example:
```bash
./client_account.x john_doe mypassword123
```

The client:
- Automatically connects to the server
- Authenticates with provided credentials
- Allows interaction with the market

---

## 💼 Features

### 🔐 Authentication
- Secure registration and login
- AES password encryption
- Server-side credential validation

### 📊 Trading
- **Order types**:
  - **MARKET**: execution at market price
  - **LIMIT**: execution at specified limit price
  - **STOP**: triggered at a price threshold
  - **LIMIT_STOP**: combination of limit and stop

- **Actions**:
  - Stock purchase (BUY)
  - Stock sale (SELL)
  - Pending order cancellation

### 💰 Account Management
- Balance inquiry
- Fund deposit
- Fund withdrawal
- Portfolio visualization

### 📈 Market
- **Fixing phase**: equilibrium price determination
- **Continuous trading phase**: real-time execution
- Transaction history
- Buy/sell order visualization

### 📝 Logging
- Complete operation history
- System and client messages
- Transaction traceability

---

## 🗄️ Database Structure

The SQLite database contains several tables:

### **Clients**
- ID, name, encrypted password
- Account balance
- Stock portfolio

### **Actions**
- ID, stock name
- Available quantity
- Price history with timestamps

### **Orders**
- Order ID
- Associated client
- Type (BUY/SELL)
- Trigger (MARKET/LIMIT/STOP/LIMIT_STOP)
- Price and quantity
- Expiration date

### **Messages**
- Message ID
- Event type
- Timestamp
- Content

---

## 🔄 Trading Flow

1. **Connection**: Client authenticates with the server
2. **Consultation**: Market and portfolio visualization
3. **Order**: Submit a buy or sell order
4. **Accumulation**: Server accumulates orders
5. **Fixing**: Equilibrium price calculation
6. **Execution**: Matching and transaction execution
7. **Update**: Portfolio and balance refresh
8. **Notification**: Client receives confirmation

---

## ⚙️ Configuration

### Network Parameters
In the source code, you can modify:
- **SERVER_IP**: server IP address (default: `127.0.0.1`)
- **PORT**: listening port (default: `8080`)

### Database
- SQLite file automatically created at startup
- Reset possible via `reset_database()` functions

---

## 🔒 Security

- **AES Encryption**: Passwords are encrypted with OpenSSL
- **Validation**: Credential verification at each connection
- **Isolation**: Each client has its own session
- **Thread-safe**: Mutex usage for concurrent management

---

## 🐛 Error Handling

The system handles several types of errors:
- Server connection failure
- Incorrect credentials
- Insufficient funds
- Unavailable stocks
- Invalid orders
- Unexpected disconnections

---

## 📝 Session Example

```bash
# Terminal 1 - Start the server
./server.x
> Server launched...
> Waiting for clients...

# Terminal 2 - Connect as client
./client_account.x alice secretpass
> Client launched...
> Authentification success
> Welcome alice!

# Check portfolio
> DISPLAY_PORTFOLIO
> Balance: 10000.00 EUR
> Actions: AAPL x 10 (150.00 EUR)

# Place a buy order
> ORDER BUY AAPL 5 LIMIT 145.00
> Order accumulated successfully

# Check pending orders
> DISPLAY_PENDING_ORDERS
> Order #123: BUY AAPL 5 @ 145.00 EUR [LIMIT]
```

---

## 🤝 Contributing

This project is an educational simulation system. To contribute:
1. Fork the project
2. Create a branch for your feature
3. Commit your changes
4. Push to the branch
5. Open a Pull Request

---

## 📄 License

Educational project - free use for learning purposes.

---

## 👥 Authors

Developed as part of an academic project for stock market trading system simulation by Tom Cuel, Rémi Durand and Thomas Chrétienne. 


