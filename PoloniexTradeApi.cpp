// Copyright (c) 2015 Scruffy Scruffington
// Distributed under the Apache 2.0 software license, see the LICENSE file
#include "PoloniexTradeApi.h"
#define BOOST_NETWORK_ENABLE_HTTPS
#include <boost/network/protocol/http/client.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/format.hpp>
#include <chrono>
#include <thread>
#include <algorithm>
#include <list>

using namespace boost::network;
using namespace boost::property_tree;
using namespace std;

PoloniexTradeApi::PoloniexTradeApi(const std::string& key, const std::string& secret):
	m_key(key),
	m_secret(secret), 
	m_nonce(time(0))
{

}

struct OrderChecker
{
	PoloniexTradeApi* m_api;

	OrderChecker(PoloniexTradeApi* api) : m_api(api) {}

	bool operator()(const pair<unsigned, std::string>& p)
	{
		return !m_api->checkOrder(p.first, p.second);
	}
};

bool PoloniexTradeApi::execute(const std::vector<Order>& orders, unsigned timeout)
{
	chrono::steady_clock::time_point start = chrono::steady_clock::now();
	list<pair<unsigned, std::string>> ids;
	for (const Order& o : orders)
		ids.push_back(make_pair(createOrder(o), o.coin));
	OrderChecker check(this);
	while (true)
	{
		if (ids.empty())
			break;
		ids.remove_if(check);
		if (chrono::steady_clock::now() - start > chrono::minutes(timeout))
			break;
		std::this_thread::sleep_for(chrono::seconds(30));
	}
	bool res = !ids.empty();
	for (auto p : ids)
		deleteOrder(p.first, p.second);
	return res;
}

unsigned PoloniexTradeApi::createOrder(const Order& order)
{
	std::map<std::string, std::string> params;
	params["command"] = (order.action == BUY) ? "buy" : "sell";
	params["currencyPair"] = "BTC_" + order.coin;
	params["rate"] = boost::lexical_cast<std::string>(order.price);
	params["amount"] = boost::lexical_cast<std::string>(order.amount);
	std::istringstream is(call(params));
	ptree pt;
	read_json(is, pt);
	std::string err = pt.get("error", "");
	if (!m_log.empty())
	{
		ofstream fout(m_log, ofstream::app);
		time_t ttp = chrono::system_clock::to_time_t(chrono::system_clock::now());
		fout << "****" << std::ctime(&ttp) << "****" << endl;
		fout << "Create order: " << endl;
		fout << "Action: " << ((order.action == BUY) ? "buy" : "sell") << endl;
		fout << "Currency: " << order.coin << endl;
		fout << "Rate: " << order.price << endl;
		fout << "Amount: " << order.amount << endl;
		fout << "Result: ";
		if (err.size())
			fout << "error [" << err << "]" << endl;
		else
			fout << pt.get<unsigned>("orderNumber") << endl;
	}
	if (err.size())
		throw std::runtime_error(err);
	return pt.get<unsigned>("orderNumber");
}

bool PoloniexTradeApi::checkOrder(unsigned id, const std::string& coin)
{
	std::map<std::string, std::string> params;
	params["command"] = "returnOpenOrders";
	params["currencyPair"] = "BTC_" + coin;
	std::istringstream is(call(params));
	ptree pt;
	read_json(is, pt);
	std::string err = pt.get("error", "");
	if (err.size())
		throw std::runtime_error(err);
	for (ptree::iterator it = pt.begin(); it != pt.end(); ++it)
	{
		ptree cpt = it->second;
		if (id == cpt.get<unsigned>("orderNumber"))
			return true;
	}
	if (!m_log.empty())
	{
		ofstream fout(m_log, ofstream::app);
		time_t ttp = chrono::system_clock::to_time_t(chrono::system_clock::now());
		fout << "****" << std::ctime(&ttp) << "****" << endl;
		fout << "Order " << id << " for " << coin << " is executed" << endl;
	}
	return false;
}

void PoloniexTradeApi::deleteOrder(unsigned id, const std::string& coin)
{
	std::map<std::string, std::string> params;
	params["command"] = "cancelOrder";
	params["currencyPair"] = "BTC_" + coin;
	params["orderNumber"] = boost::lexical_cast<std::string>(id);
	std::istringstream is(call(params));
	ptree pt;
	read_json(is, pt);
	std::string err = pt.get("error", "");
	if (!m_log.empty())
	{
		ofstream fout(m_log, ofstream::app);
		time_t ttp = chrono::system_clock::to_time_t(chrono::system_clock::now());
		fout << "****" << std::ctime(&ttp) << "****" << endl;
		fout << "Delete order " << id << " for " << coin << endl;
		if (err.size())
			fout << "Error: " << err << endl;
	}
	if (err.size())
		throw std::runtime_error(err);
}

double PoloniexTradeApi::balance(const std::string& coin)
{
	if (m_balances.empty())
		readBalances();
	auto it = m_balances.find(coin);
	if (it == m_balances.end())
		throw runtime_error("Invalid coin");
	return it->second;
}

TradeApi::CoinInfo PoloniexTradeApi::info(const std::string& coin)
{
	if (m_tickers.empty())
		readTickers();
	auto it = m_tickers.find(coin);
	if (it == m_tickers.end())
		throw runtime_error("Invalid coin");
	return it->second;
}

void PoloniexTradeApi::readTickers()
{
	http::client client;
	http::client::request request("https://poloniex.com/public?command=returnTicker");
	request << header("Connection", "close");
	http::client::response response = client.get(request);
	std::istringstream is(body(response));
	ptree pt;
	read_json(is, pt);
	for (ptree::iterator it = pt.begin(); it != pt.end(); ++it)
	{
		std::string name = it->first;
		if (name.length() < 4)
			continue;
		if (name.substr(0, 4) != "BTC_")
			continue;
		name.erase(0, 4);
		ptree child = it->second;
		CoinInfo t;
		t.coin = name;
		t.lastPrice = child.get<double>("last");
		t.buyPrice = child.get<double>("highestBid");
		t.sellPrice = child.get<double>("lowestAsk");
		m_tickers[name] = t;
	}
}

void PoloniexTradeApi::readBalances()
{
	m_balances.clear();
	std::map<std::string, std::string> params;
	params["command"] = "returnBalances";
	std::istringstream is(call(params));
	ptree pt;
	read_json(is, pt);
	std::string err = pt.get("error", "");
	if (err.size())
		throw std::runtime_error(err);
	for (ptree::iterator it = pt.begin(); it != pt.end(); ++it)
		m_balances[it->first] = boost::lexical_cast<double>(it->second.data());
}

std::string PoloniexTradeApi::call(const std::map<std::string, std::string>& params)
{
	std::string postData = postBody(params);
	std::string sign = signBody(postData);

	http::client client;
	http::client::request request("https://poloniex.com/tradingApi");
	request << header("Connection", "close");
	request << header("Content-Type", "application/x-www-form-urlencoded");
	request << header("Key", m_key);
	request << header("Sign", sign);
	request << header("Content-Length",
		(boost::format("%d") % postData.length()).str());
	request << body(postData);
	http::client::response response = client.post(request);
	return body(response);
}

std::string PoloniexTradeApi::postBody(const std::map<std::string, std::string>& params)
{
	std::string res = (boost::format("nonce=%d") % m_nonce++).str();
	for (auto param : params)
		res += "&" + param.first + "=" + param.second;
	return res;
}

std::string PoloniexTradeApi::signBody(const std::string& body)
{
	unsigned char* digest = HMAC(EVP_sha512(),
		m_secret.c_str(), m_secret.length(),
		(unsigned char*)body.c_str(), body.length(), NULL, NULL);
	char mdString[129];
	for (int i = 0; i < 64; i++)
		sprintf(&mdString[i * 2], "%02x", (unsigned int)digest[i]);
	mdString[128] = 0;
	return mdString;
}

map<std::string, double> PoloniexTradeApi::nonZeroBalancesInBTC()
{
	if (m_balances.empty())
		readBalances();
	if (m_tickers.empty())
		readTickers();
	map<std::string, double> balances;
	for (auto b : m_balances)
	{
		if (b.second == 0.0)
			continue;
		if (b.first == "BTC")
		{
			balances[b.first] = b.second;
		}
		else
		{
			TradeApi::CoinInfo ci = m_tickers[b.first];
			balances[b.first] = b.second * (ci.buyPrice + ci.sellPrice) / 2;
		}
	}
	return balances;
}

std::map<std::string, double> PoloniexTradeApi::nonZeroBalances()
{
	if (m_balances.empty())
		readBalances();
	if (m_tickers.empty())
		readTickers();
	map<std::string, double> balances;
	for (auto b : m_balances)
	{
		if (b.second == 0.0)
			continue;
		balances[b.first] = b.second;
	}
	return balances;
}

