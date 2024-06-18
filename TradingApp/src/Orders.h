#pragma once
#include <cstdint>
#include <cassert>
#include <vector>
#include <memory>
#include <list>

enum class OrderType
{
	GoodTillCancel,
	FillAndKill,
	FillOrKill,
	GoodForDay,
	Market,

};

enum class Side
{
	Buy,
	Sell
};

using Price = int32_t;
using Quantity = uint32_t;
using OrderID = uint64_t;
using OrderIDs = std::vector<OrderID>;

class Order
{
public:
	Order(OrderType _type, OrderID _id, Side _side, Price _price, Quantity _quantity);

	bool IsFilled() const;

	void Fill(Quantity quantity);

	void ToGoodTillCancel(Price _price);

	Side side{};
	OrderType type{};
	OrderID id{};
	Price price{};
	Quantity initialQuantity{};
	Quantity remainingQuantity{};
};

using OrderRef = std::shared_ptr<Order>;
using OrderReferences = std::list<OrderRef>;