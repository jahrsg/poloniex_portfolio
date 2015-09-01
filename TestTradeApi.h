// Copyright (c) 2015 Scruffy Scruffington
// Distributed under the Apache 2.0 software license, see the LICENSE file
#pragma once

#include <string>
#include <vector>
#include <map>

class TestTradeApi
{
public:
	TestTradeApi(const std::map<std::string, CoinInfo>& tickers, 
		const std::map<std::string, double>& balances);

	virtual double balance(const std::string& coin);
	virtual CoinInfo info(const std::string& coin);

	virtual void execute(const std::vector<Order>& orders, unsigned timeout);
	virtual unsigned createOrder(const Order& order);
	virtual void deleteOrder(unsigned id, const std::string& coin);
	virtual bool checkOrder(unsigned id, const std::string& coin);
private:
	std::map<std::string, CoinInfo> m_tickers;
	std::map<std::string, double> m_balances;
	unsigned m_orderNounce;
};

