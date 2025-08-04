# Orderbook

Implementation

A limit order book implementation in modern C++ that supports GoodTillCancelled (GTC) and FillAndKill (FAK) orders. Maintains price-time priority matching with efficient order management.

Architecture

The order book uses separate maps for bid and ask sides, with price-time priority enforced through ordered containers. Core features include:

- Order matching with price-time priority
- Support for GTC and FAK orders
- Order modification and cancellation
- Market depth tracking

Implementation Details

Key components:
- `std::map` with custom comparators for bid/ask price ordering
- `std::list` for order queues ensuring iterator stability
- `std::shared_ptr` for automated memory management
- Price level aggregation for market data

Data Structures

- Bids: `std::map<Price, OrderPointers, std::greater<Price>>`
- Asks: `std::map<Price, OrderPointers, std::less<Price>>`
- Order tracking: `std::unordered_map<OrderId, OrderEntry>`

Features

- Order lifecycle management (add/modify/cancel)
- Immediate-or-cancel behavior for FAK orders
- Level-based market data access
- Automatic order queue cleanup

Usage Example

```cpp
OrderPointer order = std::make_shared<Order>(
    OrderType::GoodTillCancelled,
    orderId,
    Side::Buy,
    price,
    quantity
);
orderbook.AddOrder(order);
```

Requirements

- C++17 or later compiler
- Standard Template Library

The implementation focuses on clean code structure while maintaining efficient order processing through careful container selection and memory management.
