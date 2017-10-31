// Copyright (c) 2015 Scruffy Scruffington
// Distributed under the Apache 2.0 software license, see the LICENSE file
#pragma once

#include <string>
#include <vector>

class TradeApi
{
public:
	enum Operation
	{
		BUY, 
		SELL
	};

	struct Order
	{
		std::string coin;
		double amount;
		double price;
		Operation action;

		Order() : amount(0.0), price(0.0), action(BUY) {}
	};

	struct CoinInfo
	{
		std::string coin;
		double buyPrice;
		double sellPrice;
		double lastPrice;

		CoinInfo() : buyPrice(0.0), sellPrice(0.0), lastPrice(0.0) {}
	};

	virtual double balance(const std::string& coin) = 0;
	virtual CoinInfo info(const std::string& coin) = 0;
	virtual std::map<std::string, double> nonZeroBalances() = 0;
	virtual std::map<std::string, double> nonZeroBalancesInBTC() = 0;

	virtual bool execute(const std::vector<Order>& orders, unsigned timeout) = 0;
	virtual long long createOrder(const Order& order) = 0;
        virtual void deleteOrder(long long id) = 0;
	virtual bool checkOrder(long long id, const std::string& coin) = 0;
        virtual void cancelCurrentOrders() = 0;
};

