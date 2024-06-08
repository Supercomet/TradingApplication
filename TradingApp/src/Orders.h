#pragma once
#include <cstdint>
#include <cassert>
#include <memory>
#include <list>

enum class OrderType
{
	GoodTillCancel,
	FillAndKill
};

enum class Side
{
	Buy,
	Sell
};

using Price = int32_t;
using Quantity = uint32_t;
using OrderID = uint64_t;

class Order
{
public:
	Order(OrderType _type, OrderID _id, Side _side, Price _price, Quantity _quantity) :
		type{ _type }
		, id{ _id }
		, side{ _side }
		, price{_price}
		, initialQuantity{ _quantity }
		, remainingQuantity{_quantity}
	{}

	bool IsFilled() const {
		return remainingQuantity == 0;
	}

	void Fill(Quantity quantity) {
		assert(quantity <= remainingQuantity);

		remainingQuantity -= quantity;
	}

	Side side{};
	OrderType type{};
	OrderID id{};
	Price price{};
	Quantity initialQuantity{};
	Quantity remainingQuantity{};
};

using OrderRef = std::shared_ptr<Order>;
using OrderReferences = std::list<OrderRef>;