#需求最低cmke程序版本
cmake_minimum_required(VERSION 3.12)

#本工程的名字
project(Texture_projection)

#本工程支持的C++版本
set(CMAKE_CXX_STANDARD 17)

include_directories(
	SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/thirdParty/include
)

link_directories(
	SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/thirdParty/lib
)

#本工程所有的cpp文件编译链接，生成exe
add_executable(Texture_projection "main.cpp" "glad.c")