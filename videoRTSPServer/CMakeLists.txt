# 设置项目信息和标准
cmake_minimum_required(VERSION 3.28)
project(MinVideoRTSPServer VERSION 1.0)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 查找库
find_package(Boost 1.89 COMPONENTS filesystem REQUIRED)
message(STATUS "Using Boost: ${Boost_VERSION}")


# 定义源文件列表
set(MAIN_SOURCE
    videoRTSPserver.cpp
)
file(GLOB ALL_CPP_SOURCES "*.cpp")
set(SERVER_SOURCES
    ${ALL_CPP_SOURCES}
)
list(REMOVE_ITEM ALL_CPP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${MAIN_SOURCE}")


# 创建主可执行文件
add_executable(${PROJECT_NAME}
    ${MAIN_SOURCE}
    ${SERVER_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    Boost::filesystem
)