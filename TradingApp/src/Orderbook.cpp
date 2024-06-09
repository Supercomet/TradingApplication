#include "Orderbook.h"

#ifndef NOMINMAX   /* don't define min() and max(). */
#define NOMINMAX
#endif // !NOMINMAX
#include <chrono>

OrderBook::OrderBook()
{
	GFDPruneThread = std::jthread([this](std::stop_token s) { this->PruneGoodForDay(s); });
}

OrderBook::~OrderBook()
{
	std::this_thread::sleep_for(std::chrono::seconds(4));

	GFDPruneThread.request_stop();
	GFDPruneThread.join();
}

bool OrderBook::CanMatch(Side side, Price price) const
{
	switch (side)
	{
	case Side::Buy:
	{
		if (allAsks.empty())
			return false;

		const auto& [bestAsk, entry] = *allAsks.begin();
		return price >= bestAsk;
	}
	break;
	case Side::Sell:
	{
		if (allBids.empty())
			return false;

		const auto& [bestBid, entry] = *allBids.begin();
		return price <= bestBid;
	}
	break;
	default:
	assert(true && "Side not implemented");
	return false;
	break;
	}
}

Trades OrderBook::MatchOrders()
{
	Trades trades;
	trades.reserve(allOrders.size());

	while (true)
	{

		if (allBids.empty() || allAsks.empty())
		{
			// no more trades
			break;
		}

		auto& [refBidPrice, bids] = *allBids.begin();
		auto& [refAskPrice, asks] = *allAsks.begin();

		Price bidPrice = refBidPrice;
		Price askPrice = refAskPrice;

		if (bidPrice < askPrice)
		{
			// can no longer fulfill trades
			break;
		}

		while (bids.size() && asks.size())
		{
			OrderRef bid = bids.front();
			OrderRef ask = asks.front();

			Quantity fillQuantity = std::min(bid->remainingQuantity, ask->remainingQuantity);

			bid->Fill(fillQuantity);
			ask->Fill(fillQuantity);

			if (bid->IsFilled())
			{
				bids.pop_front(); // completed so remove
				allOrders.erase(bid->id);
			}
			if (ask->IsFilled())
			{
				asks.pop_front(); // completed so remove
				allOrders.erase(ask->id);
			}

			trades.emplace_back(Trade{
				TradeInfo{bid->id,bid->price, fillQuantity},
				TradeInfo{ask->id,ask->price, fillQuantity}
				});
		}
		
		// clean up sides
		if (bids.empty())
		{
			allBids.erase(bidPrice);
		}
		if (asks.empty())
		{
			allAsks.erase(askPrice);
		}
	}

	// handle fill and kill
	if (allBids.empty() == false)
	{
		auto& [_, bids] = *allBids.begin();
		auto& order = bids.front();
		if (order->type == OrderType::FillAndKill)
		{
			CancelOrder(order->id);
		}
	}

	if (allAsks.empty() == false)
	{
		auto& [_, asks] = *allAsks.begin();
		auto& order = asks.front();
		if (order->type == OrderType::FillAndKill)
		{
			CancelOrder(order->id);
		}
	}

	return trades;
}

Trades OrderBook::AddOrder(OrderRef _order)
{
	if (allOrders.contains(_order->id))
		return {};

	if (_order->type == OrderType::FillAndKill && CanMatch(_order->side, _order->price) == false)
		return {};

	OrderReferences::iterator iter;

	if (_order->side == Side::Buy)
	{
		auto& orders = allBids[_order->price];
		orders.push_back(_order);
		iter = std::next(orders.begin(), orders.size() - 1);
	}
	else // side::sell
	{
		auto& orders = allAsks[_order->price];
		orders.push_back(_order);
		iter = std::next(orders.begin(), orders.size() - 1);
	}

	allOrders.insert({ _order->id, OrderEntry{_order,iter}});
	return MatchOrders();
}

void OrderBook::CancelOrder(OrderID _orderID)
{
	std::scoped_lock lock(ordersMutex);

	CancelOrderInternal(_orderID);	
}

void OrderBook::CancelOrders(OrderIDs orders)
{
	std::scoped_lock lock(ordersMutex);

	for (OrderID id : orders)
	{
		CancelOrderInternal(id);
	}
}

Trades OrderBook::ModifyOrder(OrderModify _order)
{
	if (allOrders.contains(_order.orderID) == false)
	{
		return {};
	}

	const auto& [existingOrder, _] = allOrders[_order.orderID];
	CancelOrder(_order.orderID);
	return AddOrder(_order.CreateOrder(existingOrder->type));
}

OrderBookLevelInfos OrderBook::GetOrderInfos() const
{
	LevelInfos bidInfos, askInfos;
	bidInfos.reserve(allOrders.size());
	askInfos.reserve(allOrders.size());

	auto CreateLevelInfos = [](Price price, const OrderReferences& orders) {
		Quantity numOrders = std::accumulate(orders.begin(), orders.end(), Quantity{},
			[](Quantity running, const OrderRef& order) {
				return running + order->remainingQuantity; }
		);
		return LevelInfo{ price, numOrders };
		};

	for (const auto& [price, orders] : allBids)
	{
		bidInfos.push_back(CreateLevelInfos(price, orders));
	}

	for (const auto& [price, orders] : allAsks)
	{
		askInfos.push_back(CreateLevelInfos(price, orders));
	}

	return OrderBookLevelInfos{ std::move(bidInfos), std::move(askInfos) };
}

void OrderBook::PruneGoodForDay(std::stop_token stoken)
{
	printf("Starting prune thread  \n");

	constexpr auto dayEndHour = std::chrono::hours(16); // 4pm

	int count = 0;

	while (stoken.stop_requested() == false)
	{
		const auto now = std::chrono::system_clock::now();
		const auto now_c = std::chrono::system_clock::to_time_t(now);

		tm now_parts;
		localtime_s(&now_parts, &now_c);

		if (now_parts.tm_hour >= dayEndHour.count())
		{
			now_parts.tm_mday += 1;
		}
		now_parts.tm_min = 0;
		now_parts.tm_sec = 0;

		auto next = std::chrono::system_clock::from_time_t(mktime(&now_parts));
		auto till = next - now + std::chrono::milliseconds(100);

		std::mutex mutex;
		std::unique_lock lock(mutex);

		// wait until day ends or stop token requested
		std::condition_variable_any().wait_until(lock, stoken, now + std::chrono::milliseconds(100), [] { return false; });
		if (stoken.stop_requested())
		{
			printf("Prune worker is requested to stop\n");
			break;
		}

		printf("Prune Iteration %2d \n", ++count);

	}

	printf("Prune thread shutdown \n");
}

void OrderBook::CancelOrderInternal(OrderID _orderID)
{
	if (allOrders.contains(_orderID) == false)
	{
		return;
	}

	const auto [order, iter] = allOrders[_orderID]; // make copy of OrderRef
	allOrders.erase(_orderID);

	if (order->side == Side::Buy)
	{
		auto price = order->price;
		auto& orders = allBids[price];
		orders.erase(iter);
		if (orders.empty())
		{
			allBids.erase(price);
		}
	}
	else if(order->side == Side::Sell)
	{
		auto price = order->price;
		auto& orders = allAsks[price];
		orders.erase(iter);
		if (orders.empty())
		{
			allAsks.erase(price);
		}
	}
	
}
