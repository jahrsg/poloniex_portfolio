// Copyright (c) 2015 Scruffy Scruffington
// Distributed under the Apache 2.0 software license, see the LICENSE file
#include "Portfolio.h"

using namespace std;

void Portfolio::addCoin(const string& coinSymbol, double part)
{
	m_parts[coinSymbol] = part;
}

map<string, double> combineParts(const map<string, double>& src,
	const map<string, double>& additional)
{
	map<string, double> res(src);
	for (auto p : additional)
	{
		if (res.find(p.first) == res.end())
			res[p.first] = 0.0;
	}
	return res;
}

vector<TradeApi::Order> Portfolio::checkCurrentState(TradeApi& trade, 
	double threshold)
{
	m_completed = true;
	map<string, double> current_parts = trade.nonZeroBalancesInBTC();
	bool maxBuy = current_parts["BTC"];
	current_parts = combineParts(current_parts, m_parts);
	map<string, double> parts = combineParts(m_parts, current_parts);
	double sum = 0.0;
	for (auto p : parts)
		sum += p.second;
	double current_sum = 0.0;
	for (auto p : current_parts)
		current_sum += p.second;
	vector<TradeApi::Order> orders;
	for (auto p : parts)
	{
		if (p.first == "BTC")
			continue;
		double diff = (current_parts[p.first] / current_sum) / (p.second / sum) - 1.0;
		if (abs(diff) < threshold)
			continue;
		TradeApi::CoinInfo ci = trade.info(p.first);
		TradeApi::Order o;
		o.coin = p.first;
		o.action = (diff > 0) ? TradeApi::SELL : TradeApi::BUY;
		o.price = (ci.buyPrice + ci.sellPrice) / 2;
		o.amount = abs(current_sum * (p.second / sum) - current_parts[p.first]) / o.price;
		if (o.action == TradeApi::BUY)
		{
			double order_sum = o.price * o.amount;
			if (order_sum > maxBuy)
			{
				m_completed = false;
				continue;
			}
			maxBuy -= order_sum;
		}
		orders.push_back(o);
	}
	return orders;
}
