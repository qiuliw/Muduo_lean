cmake_minimum_required(VERSION 2.5)

project(myMuduo)


# 设置调试信息,启动c++11语言标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

# 定义参与编译的源源文件
aux_source_directory(. SRC_LIST)

# 编译生成动态库myMuduo
add_library(myMuduo SHARED ${SRC_LIST})

# 设置最终编译成的库的路径
set(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)