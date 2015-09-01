This utility allows you to automatically rebalance your [Poloniex](http://www.poloniex.com) cryptocoin portfolio.

#USAGE

You can read about portfolio rebalance [here](http://www.bogleheads.org/wiki/Rebalancing)

Example: 
If you want to have equal parts of BTC and NXT and twice bigger part of BBR (25% of BTC, 25% of NXT and 50% of BBR), for example,  you can call periodically 
**portfolio_manager -c BTC -p 1 -c BBR -p 2 -c NXT -p 1 -k your_poloniex_api_key -s your_poloniex_api_secret -t 10 --timeout 60**
with Task Scheduler on Windows or cron on Linux or just manually.

You can start 
**portfolio_manager --help**
to read about command line options

#BUILD
Project depends on Boost, cppnetlib and OpenSSL. After installing this libs use CMake to build it with you favorite compiler.