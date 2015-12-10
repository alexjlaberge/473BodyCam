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

# Image resources
set(RESOLUTIONS "160x120" "320x240" "640x480" "1024x768")
set(SVG_INPUTS "disconnected-4x3")
foreach(input ${SVG_INPUTS})
    foreach(res ${RESOLUTIONS})
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${input}-${res}.png
            COMMAND convert ${CMAKE_CURRENT_SOURCE_DIR}/${input}.svg -resize ${res} ${CMAKE_CURRENT_BINARY_DIR}/${input}-${res}.png
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${input}.svg
            VERBATIM)
        add_custom_target(res_${input}_${res}
            SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${input}-${res}.png)
        add_dependencies(client res_${input}_${res})
    endforeach()
endforeach()

add_executable(client-tcp client-tcp.cpp)
target_link_libraries(client-tcp
    ${Boost_SYSTEM_LIBRARY}
    ${OpenCV_LIBS}
    Qt4::QtCore
    Qt4::QtGui
    pkt
    )

add_executable(dump dump.cpp)

add_executable(examine examine.cpp)
target_link_libraries(examine pkt ${OpenCV_LIBS})

add_executable(send send.cpp)

add_executable(view view.cpp)
target_link_libraries(view ${OpenCV_LIBS})

add_executable(view-wifi view-wifi.cpp)
target_link_libraries(view-wifi ${OpenCV_LIBS})

enable_testing()

add_executable(pkt_test pkt_test.cpp)
target_link_libraries(pkt_test pkt ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_test(NAME pkt_test COMMAND pkt_test)
