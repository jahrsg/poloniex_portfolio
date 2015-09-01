cmake_minimum_required (VERSION 2.6)
project (portfolio_manager)
# The version number.
set (portfolio_manager_VERSION_MAJOR 1)
set (portfolio_manager_VERSION_MINOR 0)
# Boost dependency
set(Boost_USE_MULTITHREAD ON)
find_package(Boost)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
# cppnetlib dependency
set ( CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ~/cpp-netlib-build )
find_package ( cppnetlib 0.11.0 REQUIRED )
include_directories ( ${CPPNETLIB_INCLUDE_DIRS} )
# OpenSSL dependency
find_package( OpenSSL )
include_directories(${OPENSSL_INCLUDE_DIR})
# Sources
add_executable(portfolio_manager PoloniexTradeApi.cpp Portfolio.cpp main.cpp)
target_link_libraries ( portfolio_manager ${CPPNETLIB_LIBRARIES} )
target_link_libraries ( portfolio_manager ${OPENSSL_LIBRARIES} )
# Unit tests
add_executable(portfolio_test Portfolio.cpp test.cpp)
