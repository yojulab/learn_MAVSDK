set(PROJECT_NAME regist_telemetry)

cmake_minimum_required(VERSION 2.8.12)

project(${PROJECT_NAME})

if(NOT MSVC)
    add_definitions("-std=c++11 -Wall -Wextra")
else()
    add_definitions("-std=c++11 -WX -W2")
endif()

find_package(MAVSDK REQUIRED)

add_executable(${PROJECT_NAME}
    ${PROJECT_NAME}.cpp
)

target_link_libraries(${PROJECT_NAME}
    MAVSDK::mavsdk_telemetry
    MAVSDK::mavsdk_action
    MAVSDK::mavsdk
)
