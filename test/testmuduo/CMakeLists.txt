#g++ -o server -g muduo_server.cpp xxx.cpp -I/usr/include -L/usr/lib -lmuduo_net -lmuduo_base -lpthread 
#如果我们去写一个编译命令，涉及到
#1.可执行文件 -o server
#2.编译参数：-g 调试版本 -O 优化级别
#3.需要的源文件 muduo_server.cpp xxx.cpp
#4.头文件的搜索路径 -l/usr/include
#5.库文件的搜索路径 -L/usr/lib
#6.所要连接的动态、静态库 -lmuduo_net -lmuduo_base -lpthread
#也就是说我们写一个编译命令主要设计的就是以上六点


# 配置头文件搜索路径 #4
# include_directories(/usr/include)
# 配置库文件搜索路径 #5
# link_directories(/usr/lib)

# 设置需要编译的源文件列表 #3
set(SRC_LIST ./muduo_server.cpp)
# 把 . 指定路径下的所有源文件放入变量名 SRC_LIST 里面 #3
# aux_source_directory(. SRC_LIST)

# 设置可执行文件最终存储的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 生成可执行文件 由SRC_LIST变量所定义的源文件编译而来 #1 
add_executable(server ${SRC_LIST}) # add_library()生成库文件

# 可执行文件需要连接的库 #6
target_link_libraries(server muduo_net muduo_base pthread)