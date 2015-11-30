cmake_minimum_required(VERSION 2.8)
project( 473Proj )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++11")
set(BOOST_ROOT /opt/boost)
set(Boost_USE_STATIC_LIBS ON)
find_package( OpenCV REQUIRED )
find_package( Boost COMPONENTS system)
include_directories(${Boost_INCLUDE_DIR})

add_executable( client client.cpp )
target_link_libraries( client ${OpenCV_LIBS} ${Boost_LIBRARIES} )

add_executable(examine examine.cpp)
target_link_libraries(examine ${OpenCV_LIBS})
