cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED Core Network Widgets)

add_library(xbotgo-device-discovery STATIC)
add_library(OBS::xbotgo-device-discovery ALIAS xbotgo-device-discovery)

target_sources(
  xbotgo-device-discovery
  PRIVATE
    XBotGoDeviceSearchDialog.cpp
    XBotGoDeviceSearchDialog.hpp
    XBotGoSsdpParser.cpp
    XBotGoSsdpParser.hpp
  PUBLIC XBotGoDevice.hpp
)

set_target_properties(
  xbotgo-device-discovery
  PROPERTIES AUTOMOC ON POSITION_INDEPENDENT_CODE ON FOLDER "shared"
)

target_include_directories(xbotgo-device-discovery PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(xbotgo-device-discovery PUBLIC Qt::Core Qt::Network Qt::Widgets)
