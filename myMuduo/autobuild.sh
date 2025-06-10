# 告知系统解释器路径
#!/bin/bash 

# 创建build目录
if [ ! -d "build" ]; then
    mkdir build
fi

# 设置脚本执行失败时，立即退出
set -e

#  删除build目录下的所有文件
rm -rf `pwd`/build/*

# cmake  编译
cd `pwd`/build &&
    cmake .. &&
    make

# 回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mymuduo so库拷贝到 /usr/lib/mymuduo
if  [ ! -d "/usr/include/mymuduo" ]; then
    sudo mkdir -p /usr/include/mymuduo
fi

for header in `ls *.h`
do
    sudo cp $header /usr/include/mymuduo
done

sudo cp `pwd`/lib/libmyMuduo.so /usr/lib

ldconfig #  更新系统链接库缓存



