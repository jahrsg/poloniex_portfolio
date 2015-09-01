// Copyright (c) 2015 Scruffy Scruffington
// Distributed under the Apache 2.0 software license, see the LICENSE file
#pragma once

#include <string>
#include <vector>
#include <map>
#include "TradeApi.h"

class PoloniexTradeApi : public TradeApi
{
public:
	PoloniexTradeApi(const std::string& key, const std::string& secret);

	virtual double balance(const std::string& coin);
	virtual CoinInfo info(const std::string& coin);
	virtual std::map<std::string, double> nonZeroBalances();
	virtual std::map<std::string, double> nonZeroBalancesInBTC();

	virtual bool execute(const std::vector<Order>& orders, unsigned timeout);
	virtual unsigned createOrder(const Order& order);
	virtual void deleteOrder(unsigned id, const std::string& coin);
	virtual bool checkOrder(unsigned id, const std::string& coin);

	void set_log(const std::string& logfile) {
		m_log = logfile;
	}
private:
	void readTickers();
	void readBalances();

	std::string call(const std::map<std::string, std::string>& params);
	std::string postBody(const std::map<std::string, std::string>& params);
	std::string signBody(const std::string& body);

	std::string m_key;
	std::string m_secret;
	std::map<std::string, CoinInfo> m_tickers;
	std::map<std::string, double> m_balances;
	unsigned  m_nonce;

	std::string m_log;
};

