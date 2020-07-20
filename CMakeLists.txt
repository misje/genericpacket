# 3.1.0 is the absolute minimum to use CMake with Qt 5:
cmake_minimum_required(VERSION 3.1.0)
# Allow setting PROJECT_VERSION through project():
cmake_policy(SET CMP0048 NEW)

set(CMAKE_AUTOMOC ON)

project(
	genericpacket
	VERSION 1.0.0
	LANGUAGES CXX
	)

find_package(
	Qt5
	REQUIRED
	COMPONENTS
		Core
	CONFIG
	)

add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME}
	INTERFACE
		"${CMAKE_CURRENT_SOURCE_DIR}/include/GenericPacket.h"
	)
target_link_libraries(${PROJECT_NAME} INTERFACE Qt5::Core)
target_include_directories(${PROJECT_NAME}
	INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	)
