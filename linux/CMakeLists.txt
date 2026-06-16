cmake_minimum_required(VERSION 3.20)
project(rawaccel-linux CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RAWACCEL_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../common")

# nlohmann-json: prefer system package, fall back to FetchContent
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVDEV REQUIRED libevdev)

add_subdirectory(daemon)

find_package(Qt6 QUIET COMPONENTS Widgets Charts Network)
if(Qt6_FOUND AND Qt6Widgets_FOUND AND Qt6Charts_FOUND AND Qt6Network_FOUND)
    message(STATUS "Qt6 with Charts found — building GUI")
    add_subdirectory(gui)
else()
    message(WARNING "Qt6 or Qt6Charts not found — skipping GUI. Install with: sudo pacman -S qt6-charts")
endif()
