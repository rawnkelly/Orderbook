# Orderbook

C++ Order Book Implementation

A low-latency limit order book implementation in modern C++20. Designed to handle high-throughput market data processing while maintaining clean, efficient code structure.

Architecture

The order book maintains separate buy and sell sides using price-time priority. Each price level tracks limit orders in arrival sequence. Core operations include order placement, execution, and cancellation.

Key design choices:
- STL containers optimized for frequent insertion/deletion
- Memory pool allocation for order objects
- Minimal string operations in critical paths
- Price-level indexing for O(1) best bid/ask lookup

Build Requirements

- C++20 or later
- CMake 3.15+
- Google Test (for unit tests)

## Building

```bash
mkdir build && cd build
cmake ..
make
```

## Testing

```bash
./test/orderbook_test
```

## Performance

Targets sub-microsecond latency for core operations with minimum allocation in critical paths. Benchmark results will be added as development progresses.
