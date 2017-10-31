#include "Portfolio.h"
#include "TradeApi.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE PortfolioTest
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

using namespace std;

class TestTradeApi : public TradeApi
{
public:
	void set(const map<std::string, CoinInfo>& tickers,
		const map<std::string, double>& balances)
	{
		m_tickers = tickers;
		m_balances = balances;
	}

	virtual double balance(const string& coin)
	{
		auto it = m_balances.find(coin);
		if (it == m_balances.end())
			return 0.0;
		return it->second;
	}

	virtual CoinInfo info(const string& coin)
	{
		auto it = m_tickers.find(coin);
		if (it == m_tickers.end())
			return CoinInfo();
		return it->second;
	}

	virtual map<std::string, double> nonZeroBalancesInBTC()
	{
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

	virtual map<std::string, double> nonZeroBalances()
	{
		map<std::string, double> balances;
		for (auto b : m_balances)
		{
			if (b.second == 0.0)
				continue;
			balances[b.first] = b.second;
		}
		return balances;
	}

	virtual bool execute(const vector<Order>& orders, unsigned timeout)
	{
		return true;
	}

	virtual long long createOrder(const Order& order)
	{
		return 0;
	}

    virtual void deleteOrder(long long id)
	{
	}

	virtual bool checkOrder(long long id, const string& coin)
	{
		return false;
	}

    virtual void cancelCurrentOrders()
    {

    }

private:
	map<string, CoinInfo> m_tickers;
	map<string, double> m_balances;
};

struct TradeFixture1
{
	TradeFixture1()
	{
		map<string, double> balances;
		balances["BTC"] = 0.5;
		map<string, TradeApi::CoinInfo> tickers;
		TradeApi::CoinInfo ci;
		ci.coin = "BBR";
		ci.buyPrice = 0.076;
		ci.sellPrice = 0.078;
		ci.lastPrice = 0.076;
		tickers["BBR"] = ci;
		trade.set(tickers, balances);
	}

	TestTradeApi trade;
};

struct TradeFixture2
{
	TradeFixture2()
	{
		map<string, double> balances;
		balances["BTC"] = 0.21352728;
		balances["BBR"] = 1265.05127482;
		balances["ETH"] = 2.53003383;
		balances["NXT"] = 330.53391706;
		balances["XMR"] = 8.20689865;
		map<string, TradeApi::CoinInfo> tickers;
		TradeApi::CoinInfo ci;
		ci.coin = "BBR";
		ci.buyPrice = 0.00006251;
		ci.sellPrice = 0.00006697;
		ci.lastPrice = 0.00006251;
		tickers["BBR"] = ci;
		ci.coin = "XMR";
		ci.buyPrice = 0.00231134;
		ci.sellPrice = 0.00232417;
		ci.lastPrice = 0.00231134;
		tickers["XMR"] = ci;
		ci.coin = "ETH";
		ci.buyPrice = 0.00470002;
		ci.sellPrice = 0.00473000;
		ci.lastPrice = 0.00470002;
		tickers["ETH"] = ci;
		ci.coin = "NXT";
		ci.buyPrice = 0.00003533;
		ci.sellPrice = 0.00003469;
		ci.lastPrice = 0.00003469;
		tickers["NXT"] = ci;
		trade.set(tickers, balances);
	}

	TestTradeApi trade;
};

struct TradeFixture3
{
	TradeFixture3()
	{
		map<string, double> balances;
		balances["BBR"] = 1000.0;
		map<string, TradeApi::CoinInfo> tickers;
		TradeApi::CoinInfo ci;
		ci.coin = "BBR";
		ci.buyPrice = 0.00006251;
		ci.sellPrice = 0.00006697;
		ci.lastPrice = 0.00006251;
		tickers["BBR"] = ci;
		ci.coin = "XMR";
		ci.buyPrice = 0.00231134;
		ci.sellPrice = 0.00232417;
		ci.lastPrice = 0.00231134;
		tickers["XMR"] = ci;
		ci.coin = "ETH";
		ci.buyPrice = 0.00470002;
		ci.sellPrice = 0.00473000;
		ci.lastPrice = 0.00470002;
		tickers["ETH"] = ci;
		ci.coin = "NXT";
		ci.buyPrice = 0.00003533;
		ci.sellPrice = 0.00003469;
		ci.lastPrice = 0.00003469;
		tickers["NXT"] = ci;
		trade.set(tickers, balances);
	}

	TestTradeApi trade;
};



BOOST_FIXTURE_TEST_CASE(one_coin_cases, TradeFixture1)
{
	Portfolio p;
	p.addCoin("BBR", 1);
	vector<TradeApi::Order> o = p.checkCurrentState(trade, 0.1);
	BOOST_CHECK(o.size() == 1);
	BOOST_CHECK(o[0].action == TradeApi::BUY);
	BOOST_CHECK(o[0].coin == "BBR");
	BOOST_CHECK_CLOSE(o[0].price, 0.077, 0.001);
	BOOST_CHECK_CLOSE(o[0].amount, 0.5 / 0.077, 0.001);
	p.addCoin("BTC", 1);
	o = p.checkCurrentState(trade, 0.1);
	BOOST_CHECK(o.size() == 1);
	BOOST_CHECK(o[0].action == TradeApi::BUY);
	BOOST_CHECK(o[0].coin == "BBR");
	BOOST_CHECK_CLOSE(o[0].price, 0.077, 0.001);
	BOOST_CHECK_CLOSE(o[0].amount, 0.25 / 0.077, 0.001);
}

BOOST_FIXTURE_TEST_CASE(many_coin_cases, TradeFixture2)
{
	Portfolio p;
	p.addCoin("BBR", 1);
	p.addCoin("XMR", 1);
	p.addCoin("BTC", 4);
	p.addCoin("ETH", 1);
	p.addCoin("NXT", 1);
	vector<TradeApi::Order> os = p.checkCurrentState(trade, 0.1);
	BOOST_CHECK(os.size() == 4);
	for (auto o : os)
	{
		if (o.coin == "BBR")
		{
			BOOST_CHECK(o.action == TradeApi::SELL);
			BOOST_CHECK_CLOSE(o.price, 0.00006474, 0.01);
			BOOST_CHECK_CLOSE(o.amount, 612.54, 0.01);
		}
		if (o.coin == "ETH")
		{
			BOOST_CHECK(o.action == TradeApi::BUY);
			BOOST_CHECK_CLOSE(o.price, 0.004715, 0.01);
			BOOST_CHECK_CLOSE(o.amount, 6.43, 0.01);
		}
		if (o.coin == "NXT")
		{
			BOOST_CHECK(o.action == TradeApi::BUY);
			BOOST_CHECK_CLOSE(o.price, 0.00003501, 0.01);
			BOOST_CHECK_CLOSE(o.amount, 876.08, 0.01);
		}
		if (o.coin == "XMR")
		{
			BOOST_CHECK(o.action == TradeApi::BUY);
			BOOST_CHECK_CLOSE(o.price, 0.00231775, 0.01);
			BOOST_CHECK_CLOSE(o.amount, 10.02, 0.01);
		}
	}
}

BOOST_FIXTURE_TEST_CASE(completed_cases, TradeFixture3)
{
	Portfolio p;
	p.addCoin("BBR", 1);
	vector<TradeApi::Order> o = p.checkCurrentState(trade, 0.1);
	BOOST_CHECK(p.completed());
	p.addCoin("XMR", 1);
	o = p.checkCurrentState(trade, 0.1);
	BOOST_CHECK(!p.completed());
	BOOST_REQUIRE(o.size() == 1);
	BOOST_CHECK(o[0].coin == "BBR");
	BOOST_CHECK(o[0].action == TradeApi::SELL);
}


