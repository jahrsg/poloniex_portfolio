// Copyright (c) 2015 Scruffy Scruffington
// Distributed under the Apache 2.0 software license, see the LICENSE file
#pragma once

#include <string>
#include <vector>
#include <map>
#include "TradeApi.h"

class Portfolio
{
public:
	void addCoin(const std::string& coinSymbol, double part);

	std::vector<TradeApi::Order> checkCurrentState(TradeApi& trade, 
		double threshold);

	bool completed() const
	{
		return m_completed;
	}
protected:
	std::map<std::string, double> m_parts;
	bool m_completed;
};

