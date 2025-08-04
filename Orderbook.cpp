//
// Created by Rawn Kelly.
//

#include <iostream>
#include <map>
#include <set>
#include <list>
#include <cmath>
#include <ctime>
#include <deque>
#include <queue>
#include <stack>
#include <limits>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <variant>
#include <optional>
#include <tuple>
#include <format>
#include <chrono>
#include <random>

enum class OrderType
{
    GoodTllCancelled,
    FillAndKill
};

enum class Side
{
    Buy,
    Sell
};

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;

struct LevelInfo
{
    Price price_;
    Quantity quantity_;
};

using LevelInfos = std::vector<LevelInfo>;

class OrderbookLevelInfos
{
    public:
    OrderbookLevelInfos(const LevelInfos& bids, const LevelInfos& asks)
    : bids_{  bids }
    , asks_{ asks }
    {}

    const LevelInfos& GetBids() const { return bids_; }
    const LevelInfos& GetAsks() const { return asks_; }

    private:
    LevelInfos bids_;
    LevelInfos asks_;

};

class Order
{
public:
    Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
    : orderType_{ orderType }
    , orderId_{ orderId }
    , side_{ side }
    , price_{ price }
    , initialQuantity_{ quantity }
    , remainingQuantity_{ quantity }
    { }

    OrderId GetOrderId() const { return orderId_; }
    Side GetSide() const { return side_; }
    Price GetPrice() const { return price_; }
    OrderType GetOrderType() const { return orderType_; }
    Quantity GetInitialQuantity() const { return initialQuantity_; }
    Quantity GetRemainingQuantity() const { return remainingQuantity_; }
    Quantity GetFilledQuantity() const { return GetInitialQuantity() - GetRemainingQuantity(); }
    bool IsFilled() const { return GetRemainingQuantity() == 0; }
    void Fill(Quantity quantity)
    {
        if (quantity > remainingQuantity_)
        {
            throw std::logic_error("Fill quantity is larger than remaining quantity.");
        }
        remainingQuantity_ -= quantity;
    }

private:
    OrderType orderType_;
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;

class OrderModify
{
public:
    OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
    : orderId_{ orderId }
    , side_{ side }
    , price_{ price }
    , quantity_{ quantity }
    {}

    OrderId GetOrderId() const {return orderId_;}
    Price GetPrice() const {return price_;}
    Side GetSide() const {return side_;}
    Quantity GetQuantity() const {return quantity_;}

    OrderPointer ToOrderPointer(OrderType type) const
    {
        return std::make_shared<Order>(type, GetOrderId(), GetSide(), GetPrice(), GetQuantity());
    }

private:
    OrderId orderId_;
    Price price_;
    Side side_;
    Quantity quantity_;
};

struct TradeInfo
{
    OrderId orderId_;
    Price price_;
    Quantity quantity_;
};

class Trade
{
public:
    Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade)
    : bidTrade_{ bidTrade }
    , askTrade_{ askTrade }
    { }

    const TradeInfo& GetBidTrade() const { return bidTrade_; }
    const TradeInfo& GetAskTrade() const { return askTrade_; }

private:
    TradeInfo bidTrade_;
    TradeInfo askTrade_;
};

using Trades = std::vector<Trade>;

class Orderbook
{
private:
    struct OrderEntry
    {
        OrderPointer order_{ nullptr };
        OrderPointers::iterator location_;
    };

    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;

    bool CanMatch(Side side, Price price) const
    {
        if (side == Side::Buy)
        {
            if (asks_.empty())
            return false;

            const auto& [bestAsk, _] = *asks_.begin();
            return price >= bestAsk;
        }
        else
        {
            if (bids_.empty())
            return false;

            const auto& [bestBid, _] = *bids_.begin();
            return price <= bestBid;

        }
    }

    Trades MatchOrders()
    {
        Trades trades;
        trades.reserve(orders_.size());

        while (true)
        {
            if (bids_.empty() || asks_.empty())
            break;

            auto bid_iter = bids_.begin();
            auto ask_iter = asks_.begin();

            auto& [bidPrice, bids] = *bid_iter;
            auto& [askPrice, asks] = *ask_iter;

            if (bidPrice < askPrice)
            break;

            while (bids.size() && asks.size())
            {
                auto& bid = bids.front();
                auto& ask = asks.front();

                Quantity quantity = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

                bid->Fill(quantity);
                ask->Fill(quantity);

                trades.push_back(Trade{
                    TradeInfo{bid->GetOrderId(), bid->GetPrice(), quantity },
                    TradeInfo{ask->GetOrderId(), ask->GetPrice(), quantity }
                });

                if (bid->IsFilled())
                {
                    bids.pop_front();
                    orders_.erase(bid->GetOrderId());
                }

                if (ask->IsFilled())
                {
                    asks.pop_front();
                    orders_.erase(ask->GetOrderId());
                }
            }

            if (bids.empty())
                bids_.erase(bidPrice);

            if (asks.empty())
                asks_.erase(askPrice);
        }

        if (!bids_.empty())
        {
            auto& [_, bids] = *bids_.begin();
            auto& order = bids.front();
            if (order->GetOrderType() == OrderType::FillAndKill)
               CancelOrder(order->GetOrderId());
        }

          if (!asks_.empty())
        {
            auto& [_, asks] = *asks_.begin();
            auto& order = asks.front();
            if (order->GetOrderType() == OrderType::FillAndKill)
               CancelOrder(order->GetOrderId());
        }

        return trades;
    }

public:

    Trades AddOrder(OrderPointer order)
    {
        if (orders_.count(order->GetOrderId()) > 0)
            return {};

        if (order->GetOrderType() == OrderType::FillAndKill && !CanMatch(order->GetSide(), order->GetPrice()))
            return {};

        OrderPointers::iterator iterator;

        if (order->GetSide() == Side::Buy)
        {
            auto& orders = bids_[order->GetPrice()];
            orders.push_back(order);
            iterator = std::prev(orders.end());
        }
        else
        {
            auto& orders = asks_[order->GetPrice()];
            orders.push_back(order);
            iterator = std::prev(orders.end());
        }

        orders_.insert({order->GetOrderId(), OrderEntry{order, iterator}});
        return MatchOrders();
    }

    void CancelOrder(OrderId orderId)
    {
        if (orders_.count(orderId) == 0)
            return;

        const auto& [order, iterator] = orders_.at(orderId);
        orders_.erase(orderId);

        if (order->GetSide() == Side::Sell)
        {
            auto price = order->GetPrice();
            auto& orders = asks_.at(price);
            orders.erase(iterator);
            if (orders.empty())
                asks_.erase(price);
        }
        else
        {
            auto price = order->GetPrice();
            auto& orders = bids_.at(price);
            orders.erase(iterator);
            if (orders.empty())
            {
                bids_.erase(price);
            }
        }
    }

    Trades ModifyOrder(OrderModify order)
    {
        if (orders_.count(order.GetOrderId()) == 0)
            return {};

        const auto& [existingOrder, _] = orders_.at(order.GetOrderId());
        CancelOrder(order.GetOrderId());
        return AddOrder(order.ToOrderPointer(existingOrder->GetOrderType()));
    }

    OrderbookLevelInfos GetOrderInfos() const
    {
        LevelInfos bidInfos, askInfos;
        bidInfos.reserve(bids_.size());
        askInfos.reserve(asks_.size());

        auto CreateLevelInfos = [](Price price, const OrderPointers& orders)
        {
            return LevelInfo { price, std::accumulate(orders.begin(), orders.end(),(Quantity)0,
            [](Quantity runningSum, const OrderPointer& order)
            { return runningSum + order->GetRemainingQuantity(); }) };

        };

        for (const auto& [price, orders] : bids_)
            bidInfos.push_back(CreateLevelInfos(price, orders));

         for (const auto& [price, orders] : asks_)
            askInfos.push_back(CreateLevelInfos(price, orders));

        return OrderbookLevelInfos { bidInfos, askInfos };
    }
};

// --- Test Harness ---

// Generates a random order
OrderPointer GenerateRandomOrder(OrderId& currentOrderId) {
    static std::mt19937 rng(std::time(nullptr));
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<Price> price_dist(90, 110);
    std::uniform_int_distribution<Quantity> quantity_dist(1, 100);

    Side side = side_dist(rng) == 0 ? Side::Buy : Side::Sell;
    Price price = price_dist(rng);
    Quantity quantity = quantity_dist(rng);

    return std::make_shared<Order>(
        OrderType::GoodTllCancelled,
        currentOrderId++,
        side,
        price,
        quantity
    );
}

int main()
{
    Orderbook orderbook;
    OrderId orderId = 1;
    long long total_trades = 0;
    const int test_duration_seconds = 60;

    std::cout << "Starting orderbook performance test for " << test_duration_seconds << " seconds..." << std::endl;

    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(test_duration_seconds);

    while (std::chrono::steady_clock::now() < end_time)
    {
        OrderPointer order = GenerateRandomOrder(orderId);
        Trades trades = orderbook.AddOrder(order);
        total_trades += trades.size();
    }

    auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
    double duration_sec = actual_duration.count() / 1000.0;
    double trades_per_second = total_trades / duration_sec;

    std::cout << "Test finished." << std::endl;
    std::cout << "--------------------------------" << std::endl;
    std::cout << "Total trades processed: " << total_trades << std::endl;
    std::cout << "Total duration: " << duration_sec << " seconds" << std::endl;
    std::cout << "Transactions per second: " << trades_per_second << std::endl;
    std::cout << "--------------------------------" << std::endl;

    return 0;
}