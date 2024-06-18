#include "Orders.h"

Order::Order(OrderType _type, OrderID _id, Side _side, Price _price, Quantity _quantity)
	:
	type{ _type }
	, id{ _id }
	, side{ _side }
	, price{_price}
	, initialQuantity{ _quantity }
	, remainingQuantity{_quantity}
{}

bool Order::IsFilled() const
{
	return remainingQuantity == 0;
}

void Order::Fill(Quantity quantity)
{
	assert(quantity <= remainingQuantity);

	remainingQuantity -= quantity;
}

void Order::ToGoodTillCancel(Price _price)
{
	type = OrderType::GoodTillCancel;
	price = price;
}
