#pragma once
#include "Orders.h"
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <numeric>

struct LevelInfo
{
	Price price;
	Quantity quantity;
};
using LevelInfos = std::vector<LevelInfo>;

class OrderBookLevelInfos
{
public:
	OrderBookLevelInfos(LevelInfos&& _bids, LevelInfos&& _asks) 
		: bids{ std::move(_bids) }
		, asks{ std::move(_asks) }
	{}
public:
	LevelInfos bids;
	LevelInfos asks;
};

class OrderModify
{
public:
	OrderModify(OrderID _orderID, Side _side, Price _price, Quantity _quantity)
		: orderID{ _orderID }
		, side{ _side }
		, price{ _price }
		, quantity{ _quantity }
	{}

	OrderRef CreateOrder(OrderType type) {
		return std::make_shared<Order>(type, orderID, side, price, quantity);
	}
public:
	OrderID orderID;
	Side side;
	Price price;
	Quantity quantity;	
};

struct TradeInfo
{
	TradeInfo() {};
	TradeInfo(OrderID _orderID, Price _price, Quantity _quantity) :
		orderID{ _orderID }
		, price{_price}
		, quantity{ _quantity }
	{}

public:
	OrderID orderID{};
	Price price{};
	Quantity quantity{};
};

class Trade
{
public:
	Trade(const TradeInfo& _bidTrade, const TradeInfo& _askTrade):
		bidTrade{_bidTrade}
		, askTrade{_askTrade}
	{}
public:
	TradeInfo bidTrade;
	TradeInfo askTrade;
};

using Trades = std::vector<Trade>;

class OrderBook
{
public:
	struct OrderEntry
	{
		OrderRef order{nullptr};
		OrderReferences::iterator location;
	};

	bool CanMatch(Side side, Price price) const;
	Trades MatchOrders();
	Trades AddOrder(OrderRef _order);
	void CancelOrder(OrderID _orderID);
	Trades ModifyOrder(OrderModify _order);
	OrderBookLevelInfos GetOrderInfos() const;

	size_t Size() { return allOrders.size(); }
private:
	std::map< Price, OrderReferences, std::greater<int> > allBids;
	std::map< Price, OrderReferences, std::less<int>    > allAsks;
	std::unordered_map< OrderID, OrderEntry > allOrders;
};