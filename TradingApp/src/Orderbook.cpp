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
	break;
	}
	return false;
}

bool OrderBook::CanFullyFill(Side side, Price price, Quantity quantity) const
{
	if (CanMatch(side, price) == false)
	{
		return false;
	}

	std::optional<Price> threshold;

	if (side == Side::Buy)
	{
		const auto [askPrice, _] = *allAsks.begin();
		threshold = askPrice;
	}
	else
	{
		const auto [bidPrice, _] = *allBids.begin();
		threshold = bidPrice;
	}

	for (const auto&[levelPrice, levelData] : allData)
	{
		if (threshold.has_value() &&
			(side == Side::Buy && *threshold > levelPrice)
			|| (side == Side::Sell && *threshold < levelPrice) )
			continue;

		if ((side == Side::Buy && levelPrice > price)
			|| (side == Side::Sell && levelPrice < price))
			continue;

		if (quantity <= levelData.quantity)
			return true;

		quantity -= levelData.quantity;
	}

	return false;
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
	std::scoped_lock lock(ordersMutex);

	if (allOrders.contains(_order->id))
		return {};

	if (_order->type == OrderType::FillAndKill && CanMatch(_order->side, _order->price) == false)
		return {};

	// process order by type
	switch (_order->type)
	{
	case OrderType::GoodTillCancel:
	break;
	case OrderType::FillAndKill:
	{
		if (CanMatch(_order->side, _order->price) == false)
		{
			return {};
		}
	}
	break;
	case OrderType::FillOrKill:
	{
		if (CanFullyFill(_order->side, _order->price, _order->initialQuantity) == false)
		{
			return {};
		}
	}
	break;
	case OrderType::GoodForDay:
	break;
	case OrderType::Market:
	{
		if (_order->side == Side::Buy)
		{
			const auto& [worstAsk, _] = *allAsks.rbegin();
		}
		else if (_order->side == Side::Sell)
		{
			const auto& [worstBid, _] = *allBids.rbegin();
		}
		else
		{
			return{};
		}
	}
	break;
	default:
		return {};
	break;
	}

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

	OnOrderAdded(_order);
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

		OrderIDs ordersToCancel;
		{
			std::unique_lock lock(this->ordersMutex);
			// wait until day ends or stop token requested
			std::condition_variable_any().wait_until(lock, stoken, now + std::chrono::milliseconds(100), [] { return false; });
			if (stoken.stop_requested())
			{
				printf("Prune worker is requested to stop\n");
				break;
			}
			// mutex is aquired by wait_until
			for (auto& [entryID,entry] : allOrders)
			{
				const auto& [order, _] = entry;
				if (order->type == OrderType::GoodForDay)
				{
					ordersToCancel.push_back(order->id);
				}
			}
			printf("Prune Iteration %2d \n", ++count);
		}

		if (ordersToCancel.size())
		{
			printf("GoodForDay orders pruned [%llu]\n", ordersToCancel.size());
			CancelOrders(ordersToCancel);
		}
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
	
	OnOrderCancelled(order);
}

void OrderBook::OnOrderAdded(OrderRef order)
{
	UpdateLevelData(order->price, order->initialQuantity, LevelData::Action::Add);
}

void OrderBook::OnOrderCancelled(OrderRef order)
{
	UpdateLevelData(order->price, order->remainingQuantity, LevelData::Action::Remove);
}

void OrderBook::OnOrderMatched(Price price, Quantity quantity, bool isFullyFilled)
{
	UpdateLevelData(price, quantity, isFullyFilled ? LevelData::Action::Remove : LevelData::Action::Match);
}

void OrderBook::UpdateLevelData(Price price, Quantity quantity, LevelData::Action action)
{
	auto& data = allData[price];

	switch (action)
	{
	case OrderBook::LevelData::Action::Add:
	{
		data.count += 1;
		data.quantity += quantity;
	}
	break;	
	case OrderBook::LevelData::Action::Remove:
		data.count -= 1; // only if remove
	case OrderBook::LevelData::Action::Match:
		data.quantity -= quantity; // both cases we should reduce quant
	break;
	default:
	assert(false && "invalid action");
	break;
	}

	// if we run out of orders
	if (data.count == 0)
		allData.erase(price);

}
