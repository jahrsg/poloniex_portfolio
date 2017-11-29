// Copyright (c) 2015 Scruffy Scruffington
// Distributed under the Apache 2.0 software license, see the LICENSE file
#include "PoloniexTradeApi.h"
#include "Log.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <chrono>
#include <thread>
#include <algorithm>
#include <list>
#include <iostream>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>
using namespace boost::property_tree;
using namespace std;

PoloniexTradeApi::PoloniexTradeApi(const std::string& key, const std::string& secret):
	m_key(key),
	m_secret(secret), 
	m_nonce(time(0))
{
	Log l("PoloniexTradeApi::PoloniexTradeApi()");
}

struct OrderChecker
{
	PoloniexTradeApi* m_api;

	OrderChecker(PoloniexTradeApi* api) : m_api(api) {}

    bool operator()(const pair<long long, std::string>& p)
	{
		return !m_api->checkOrder(p.first, p.second);
	}
};

bool PoloniexTradeApi::execute(const std::vector<Order>& orders, unsigned timeout)
{
	Log l("PoloniexTradeApi::execute");
	chrono::steady_clock::time_point start = chrono::steady_clock::now();
    list<pair<long long, std::string>> ids;
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
        deleteOrder(p.first);
	return res;
}

long long PoloniexTradeApi::createOrder(const Order& order)
{
	Log l("PoloniexTradeApi::createOrder");
	std::map<std::string, std::string> params;
	params["command"] = (order.action == BUY) ? "buy" : "sell";
	params["currencyPair"] = "BTC_" + order.coin;
	params["rate"] = boost::lexical_cast<std::string>(order.price);
	params["amount"] = boost::lexical_cast<std::string>(order.amount);
    if(order.coin == "USDT")
    {
        params["command"] = (order.action == BUY) ? "sell" : "buy";
        params["currencyPair"] = "USDT_BTC";
        params["rate"] = boost::lexical_cast<std::string>(1.0 / order.price);
        params["amount"] = boost::lexical_cast<std::string>(order.amount * order.price);
    }
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
			fout << pt.get<long long>("orderNumber") << endl;
	}
	if (err.size())
	{
		Log::write("throw");
		throw std::runtime_error(err);
	}
	return pt.get<long long>("orderNumber");
}

bool PoloniexTradeApi::checkOrder(long long id, const std::string& coin)
{
    Log l(boost::str(boost::format("PoloniexTradeApi::checkOrder(%d, %s)") %
                            id % coin.c_str()));
	std::map<std::string, std::string> params;
	params["command"] = "returnOpenOrders";
	params["currencyPair"] = "BTC_" + coin;
    if(coin == "USDT")
        params["currencyPair"] = "USDT_BTC";
    std::istringstream is(call(params));
	ptree pt;
	read_json(is, pt);
	std::string err = pt.get("error", "");
	if (err.size())
	{
		Log::write("throw");
		throw std::runtime_error(err);
	}
	for (ptree::iterator it = pt.begin(); it != pt.end(); ++it)
	{
		ptree cpt = it->second;
		if (id == cpt.get<long long>("orderNumber"))
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

void PoloniexTradeApi::cancelCurrentOrders()
{
    Log l("PoloniexTradeApi::cancelCurrentOrders()");
    std::vector<long long> orders = getCurrentOrders();
    for(auto id: orders)
        deleteOrder(id);
}

std::vector<long long> PoloniexTradeApi::getCurrentOrders()
{
    Log l("PoloniexTradeApi::getCurrentOrders()");
    std::map<std::string, std::string> params;
    params["command"] = "returnOpenOrders";
    params["currencyPair"] = "all";
    std::istringstream is(call(params));
    ptree pt;
    read_json(is, pt);
    std::string err = pt.get("error", "");
    if (err.size())
    {
        Log::write("throw");
        throw std::runtime_error(err);
    }
    std::vector<long long> res;
    for (auto it: pt)
    {
        ptree coinpt = it.second;
        for(auto oit: coinpt)
        {
            ptree entry = oit.second;
            res.push_back(entry.get<long long>("orderNumber"));
        }
    }
    return res;
}

std::vector<long long> PoloniexTradeApi::getCurrentOrders(const string &coin)
{
    Log l(boost::str(boost::format("PoloniexTradeApi::getCurrentOrders(%s)") %
                            coin.c_str()));
    std::map<std::string, std::string> params;
    params["command"] = "returnOpenOrders";
    params["currencyPair"] = "BTC_" + coin;
    if(coin == "USDT")
        params["currencyPair"] = "USDT_BTC";
    std::istringstream is(call(params));
    ptree pt;
    read_json(is, pt);
    std::string err = pt.get("error", "");
    if (err.size())
    {
        Log::write("throw");
        throw std::runtime_error(err);
    }
    std::vector<long long> res;
    for (ptree::iterator it = pt.begin(); it != pt.end(); ++it)
    {
        ptree cpt = it->second;
        res.push_back(cpt.get<long long>("orderNumber"));
    }
    return res;
}

void PoloniexTradeApi::deleteOrder(long long id)
{
    Log l(boost::str(boost::format("PoloniexTradeApi::deleteOrder(%d)") % id));
	std::map<std::string, std::string> params;
	params["command"] = "cancelOrder";
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
        fout << "Delete order " << id << endl;
		if (err.size())
			fout << "Error: " << err << endl;
	}
	if (err.size())
	{
		Log::write("throw");
		throw std::runtime_error(err);
	}
}

double PoloniexTradeApi::balance(const std::string& coin)
{
    Log l(boost::str(boost::format("PoloniexTradeApi::balance(%s)") % coin.c_str()));
	if (m_balances.empty())
		readBalances();
	auto it = m_balances.find(coin);
	if (it == m_balances.end())
	{
		Log::write("throw");
		throw runtime_error("Invalid coin");
	}
	return it->second;
}

TradeApi::CoinInfo PoloniexTradeApi::info(const std::string& coin)
{
    Log l(boost::str(boost::format("PoloniexTradeApi::info(%s)") % coin.c_str()));
	if (m_tickers.empty())
		readTickers();
	auto it = m_tickers.find(coin);
	if (it == m_tickers.end())
	{
		Log::write("throw");
		throw runtime_error("Invalid coin");
	}
	return it->second;
}

static void
load_root_certificates(ssl::context& ctx)
{
	std::string const cert =
		/*  This is the DigiCert root certificate.

		CN = DigiCert High Assurance EV Root CA
		OU = www.digicert.com
		O = DigiCert Inc
		C = US

		Valid to: Sunday, ?November ?9, ?2031 5:00:00 PM

		Thumbprint(sha1):
		5f b7 ee 06 33 e2 59 db ad 0c 4c 9a e6 d3 8f 1a 61 c7 dc 25
		*/
		"-----BEGIN CERTIFICATE-----\n"
		"MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n"
		"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
		"d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n"
		"ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n"
		"MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n"
		"LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n"
		"RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n"
		"+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n"
		"PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n"
		"xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n"
		"Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n"
		"hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n"
		"EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n"
		"MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n"
		"FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n"
		"nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n"
		"eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n"
		"hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n"
		"Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n"
		"vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n"
		"+OkuE6N36B9K\n"
		"-----END CERTIFICATE-----\n"
		/*  This is the GeoTrust root certificate.

		CN = GeoTrust Global CA
		O = GeoTrust Inc.
		C = US
		Valid to: Friday, ‎May ‎20, ‎2022 9:00:00 PM

		Thumbprint(sha1):
		‎de 28 f4 a4 ff e5 b9 2f a3 c5 03 d1 a3 49 a7 f9 96 2a 82 12
		*/
		"-----BEGIN CERTIFICATE-----\n"
		"MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n"
		"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
		"d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n"
		"ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n"
		"MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n"
		"LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n"
		"RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n"
		"+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n"
		"PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n"
		"xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n"
		"Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n"
		"hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n"
		"EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n"
		"MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n"
		"FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n"
		"nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n"
		"eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n"
		"hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n"
		"Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n"
		"vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n"
		"+OkuE6N36B9K\n"
		"-----END CERTIFICATE-----\n"
		;

	boost::system::error_code ec;
	ctx.add_certificate_authority(
		boost::asio::buffer(cert.data(), cert.size()), ec);
	if (ec)
		throw boost::system::system_error{ ec };
}

void PoloniexTradeApi::readTickers()
{
	Log l("PoloniexTradeApi::readTickers()");

	// connection params
	std::string host("poloniex.com");
	std::string port("443");
	std::string target("/public?command=returnTicker"); ///?????

	// The io_service is required for all I/O
	boost::asio::io_service ios;

	// The SSL context is required, and holds certificates
	ssl::context ctx{ ssl::context::sslv23_client };

	// This holds the root certificate used for verification
	load_root_certificates(ctx);

	// These objects perform our I/O
	tcp::resolver resolver{ ios };
	ssl::stream<tcp::socket> stream{ ios, ctx };

	// Look up the domain name
	auto const lookup = resolver.resolve({ host, port });

	// Make the connection on the IP address we get from a lookup
	boost::asio::connect(stream.next_layer(), lookup);

	// Perform the SSL handshake
	stream.handshake(ssl::stream_base::client);

	// Set up an HTTP GET request message
	http::request<http::string_body> req{ http::verb::get, target, 11 };
	req.set(http::field::host, host);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

	// Send the HTTP request to the remote host
	http::write(stream, req);

	// This buffer is used for reading and must be persisted
	boost::beast::flat_buffer buffer;

	// Declare a container to hold the response
    http::response<http::string_body> res;

	// Receive the HTTP response
	http::read(stream, buffer, res);

	// Write the message to string stream to read in parser
	std::stringstream is; 
    is << res.body();

	// Gracefully close the stream
	boost::system::error_code ec;
	stream.shutdown(ec);
/*	if (ec == boost::asio::error::eof)
	{
		// Rationale:
		// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
		ec.assign(0, ec.category());
	}
	if (ec)
		throw boost::system::system_error{ ec };
*/
	// If we get here then the connection is closed gracefully

	// parse responce
	ptree pt;
	read_json(is, pt);
	for (ptree::iterator it = pt.begin(); it != pt.end(); ++it)
	{
		std::string name = it->first;
		if (name.length() < 4)
			continue;
        if (name == "USDT_BTC")
        {
            name = "USDT";
            ptree child = it->second;
            CoinInfo t;
            t.coin = name;
            t.lastPrice = 1.0 / child.get<double>("last");
            t.buyPrice = 1.0 / child.get<double>("highestBid");
            t.sellPrice = 1.0 / child.get<double>("lowestAsk");
            m_tickers[name] = t;
            continue;
        }
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
	Log l("PoloniexTradeApi::readTickers()");
	m_balances.clear();
	std::map<std::string, std::string> params;
	params["command"] = "returnBalances";
	std::istringstream is(call(params));
	ptree pt;
	read_json(is, pt);
	std::string err = pt.get("error", "");
	if (err.size())
	{
		Log::write("throw");
		throw std::runtime_error(err);
	}
	for (ptree::iterator it = pt.begin(); it != pt.end(); ++it)
	{
        double balance = boost::lexical_cast<double>(it->second.data());
        if(balance > 0.0001)
            m_balances[it->first] = balance;
	}
}

std::string PoloniexTradeApi::call(const std::map<std::string, std::string>& params)
{
	Log l("PoloniexTradeApi::call");
    // connection params
    std::string host("poloniex.com");
    std::string port("443");
    std::string target("/tradingApi");

    std::string postData = postBody(params);
	std::string sign = signBody(postData);

    // The io_service is required for all I/O
    boost::asio::io_service ios;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ ssl::context::sslv23_client };

    // This holds the root certificate used for verification
    load_root_certificates(ctx);

    // These objects perform our I/O
    tcp::resolver resolver{ ios };
    ssl::stream<tcp::socket> stream{ ios, ctx };

    // Look up the domain name
    auto const lookup = resolver.resolve({ host, port });

    // Make the connection on the IP address we get from a lookup
    boost::asio::connect(stream.next_layer(), lookup);

    // Perform the SSL handshake
    stream.handshake(ssl::stream_base::client);

    // Set up an HTTP POST request message
    http::request<http::string_body> req;
    req.method(http::verb::post);
    req.target(target);
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::content_type,
            "application/x-www-form-urlencoded");
    req.set("Key", m_key);
    req.set("Sign", sign);
    req.body() = postData;
    req.prepare_payload();

    // Send the HTTP request to the remote host
    http::write(stream, req);

    // This buffer is used for reading and must be persisted
    boost::beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    http::read(stream, buffer, res);

    // Write the message to output stream
    std::string reply(res.body());

    // Gracefully close the stream
    boost::system::error_code ec;
    stream.shutdown(ec);
/*    if (ec == boost::asio::error::eof)
    {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec.assign(0, ec.category());
    }
    else if (ec)
        throw boost::system::system_error{ ec };*/
    // If we get here then the connection is closed gracefully
    return reply;
}

std::string PoloniexTradeApi::postBody(const std::map<std::string, std::string>& params)
{
	Log l("PoloniexTradeApi::postBody");
	std::string res = (boost::format("nonce=%d") % m_nonce++).str();
	for (auto param : params)
		res += "&" + param.first + "=" + param.second;
    Log::write(boost::str(boost::format("Result: %s") % res.c_str()));
	return res;
}

std::string PoloniexTradeApi::signBody(const std::string& body)
{
	Log l("PoloniexTradeApi::signBody");
    Log::write(boost::str(boost::format("Body: %s") % body.c_str()));
	unsigned char* digest = HMAC(EVP_sha512(),
		m_secret.c_str(), m_secret.length(),
		(unsigned char*)body.c_str(), body.length(), NULL, NULL);
	char mdString[129];
	for (int i = 0; i < 64; i++)
		sprintf(&mdString[i * 2], "%02x", (unsigned int)digest[i]);
	mdString[128] = 0;
    Log::write(boost::str(boost::format("Sign: %s") % mdString));
	return mdString;
}

map<std::string, double> PoloniexTradeApi::nonZeroBalancesInBTC()
{
	Log l("PoloniexTradeApi::nonZeroBalancesInBTC()");
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
            Log::write(boost::str(boost::format("%s:%f") % b.first.c_str() % b.second));
		}
		else
		{
			TradeApi::CoinInfo ci = m_tickers[b.first];
			double amount = b.second * (ci.buyPrice + ci.sellPrice) / 2;
			balances[b.first] = amount;
            Log::write(boost::str(boost::format("%s:%f") % b.first.c_str() % amount));
		}
	}
	return balances;
}

std::map<std::string, double> PoloniexTradeApi::nonZeroBalances()
{
	Log l("PoloniexTradeApi::nonZeroBalances()");
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
        Log::write(boost::str(boost::format("%s:%f") % b.first.c_str() % b.second));
	}
	return balances;
}

