# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.10.0)
# 定义项目名称和版本
project(muduo VERSION 0.1.0 LANGUAGES C CXX)

# 添加可执行文件
add_executable(server muduo_server.cpp)

# 链接库
# 注意：这里假设 muduo_net 和 muduo_base 是自定义的库，需要确保这些库的路径正确
# 如果这些库是系统库或者已经在其他地方配置好了路径，可直接链接
# 若库不在标准搜索路径，可使用 link_directories 指定库文件所在目录
# 例如：link_directories(/path/to/muduo_libs)
target_link_libraries(server 
    muduo_net 
    muduo_base 
    pthread
)

# g++ -o server muduo_server.
# cpp -lmuduo_net -lmuduo_base -lpthread


# gcc -I头文件搜索路径 -L库文件搜索路径 -lmuduo_net