// Copyright (c) 2015 Scruffy Scruffington
// Distributed under the Apache 2.0 software license, see the LICENSE file
#include <string>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include "PoloniexTradeApi.h"
#include "Portfolio.h"
#include "Log.h"

namespace po = boost::program_options;
using namespace std;

int main(int argc, char* argv[])
{
	try
	{
		Log::init();
		po::options_description desc("Available options");
		desc.add_options()
			("help,h", "show options list")
			("key,k", po::value<string>(), "Poloniex API key")
			("secret,s", po::value<string>(), "Poloniex API secret")
			("coins,c", po::value< vector<string> >()->multitoken(), "Poloniex currency symbols, several values")
			("parts,p", po::value< vector<double> >()->multitoken(), "Currency parts, several values")
			("threshold,t", po::value<double>(), "Threshold to align currency part, in percents")
			("timeout", po::value<unsigned>(), "Order timeout in minutes")
			("balancelog,b", po::value<string>(), "File to log current balance")
			("orderlog,o", po::value<string>(), "File to log all orders operations");
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help"))
		{
			cout << desc << endl;
			return 0;
		}

		string key = vm["key"].as<string>();
		string secret = vm["secret"].as<string>();
		vector<string> coins = vm["coins"].as< vector<string> >();
		vector<double> parts = vm["parts"].as< vector<double> >();
		if (coins.size() != parts.size())
			throw runtime_error("Coins number differs from parts number");
		double threshold = vm["threshold"].as<double>();
		unsigned timeout = vm["timeout"].as<unsigned>();

        PoloniexTradeApi trade(key, secret);
        trade.cancelCurrentOrders();
        map<string, double> bs = trade.nonZeroBalances();
        map<string, double> btcbs = trade.nonZeroBalancesInBTC();
        double total = 0.0;
        for (auto b : btcbs)
        {
            cout << b.first << ": " << bs[b.first] << " (" << b.second << "BTC)" << std::endl;
            total += b.second;
        }
        cout << "Total: " << total << "BTC" << std::endl;
        if (vm.count("balancelog"))
        {
            string fname = vm["balancelog"].as<string>();
            ofstream fout(fname, ofstream::app);
            if (!fout.is_open())
                throw runtime_error("Failed to open file " + fname);
            time_t ttp = chrono::system_clock::to_time_t(chrono::system_clock::now());
            fout << ttp << "," << total << endl;
        }
        Portfolio p;
        for (unsigned i = 0; i < coins.size(); ++i)
            p.addCoin(coins[i], parts[i]);

        vector<TradeApi::Order> orders = p.checkCurrentState(trade, threshold);
        if (!orders.size())
            return 0;

        cout << "Execute " << orders.size() << " orders..." << endl;
        if (vm.count("orderlog"))
            trade.set_log(vm["orderlog"].as<string>());
        trade.execute(orders, timeout);
	}
	catch (const exception& e)
	{
		cerr << e.what() << endl;
		system("pause");
	}
	return 0;
}

