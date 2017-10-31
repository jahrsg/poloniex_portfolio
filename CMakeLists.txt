cmake_minimum_required (VERSION 2.6)
project (portfolio_manager)
# The version number.
set (portfolio_manager_VERSION_MAJOR 1)
set (portfolio_manager_VERSION_MINOR 1)
# Boost dependency
set(Boost_USE_MULTITHREAD ON)
find_package(Boost COMPONENTS program_options system unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIR_RELEASE})
# Beast dependency (will be part of Boost since 1.66 version)
set(BEAST_INCLUDE_DIR "~/libs/beast/include" )
include_directories(${BEAST_INCLUDE_DIR})
# OpenSSL dependency
find_package( OpenSSL )
include_directories(${OPENSSL_INCLUDE_DIR})
# Sources
add_executable(portfolio_manager PoloniexTradeApi.cpp Portfolio.cpp Log.cpp main.cpp)
target_link_libraries ( portfolio_manager pthread ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} )
# Unit tests
add_executable(portfolio_test Portfolio.cpp test.cpp)
target_link_libraries ( portfolio_test ${Boost_LIBRARIES} )

