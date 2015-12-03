cmake_minimum_required(VERSION 2.8)
project(473Proj)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++11")
add_definitions(-DBOOST_TEST_DYN_LINK)

find_package(OpenCV REQUIRED)
find_package(Boost COMPONENTS system unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(Qt4 REQUIRED)
set(CMAKE_AUTOMOC ON)

add_library(pkt STATIC pkt.cpp)

add_executable(client client.cpp)
target_link_libraries(client
    ${Boost_SYSTEM_LIBRARY}
    ${OpenCV_LIBS}
    Qt4::QtCore
    Qt4::QtGui
    pkt
    )

add_executable(examine examine.cpp)
target_link_libraries(examine pkt ${OpenCV_LIBS})

enable_testing()

add_executable(pkt_test pkt_test.cpp)
target_link_libraries(pkt_test pkt ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(NAME pkt_test COMMAND pkt_test)
