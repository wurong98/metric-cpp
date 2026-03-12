cmake_minimum_required(VERSION 3.16)
project(metric_hyperliquid VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(libwebsockets REQUIRED libwebsockets)
pkg_check_modules(yaml-cpp REQUIRED yaml-cpp)

# Set library variables for subdirectories
set(LIBWEBSOCKETS_LIBRARIES ${libwebsockets_LIBRARIES})
set(LIBWEBSOCKETS_INCLUDE_DIRS ${libwebsockets_INCLUDE_DIRS})

# Fetch nlohmann/json
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

# Add subdirectories
add_subdirectory(src)

# Enable testing
enable_testing()
add_subdirectory(tests)
