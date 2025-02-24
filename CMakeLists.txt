cmake_minimum_required(VERSION 3.12)

# 本工程的名字
project(Texture_projection)

# 本工程支持的C++版本
set(CMAKE_CXX_STANDARD 17)

# 查找 Python 3
find_package(Python3 REQUIRED COMPONENTS Development Interpreter)
include_directories(${Python3_INCLUDE_DIRS})

# 查找 Pybind11
#set(pybind11_DIR "D:/Application/Lib/site-packages/pybind11/share/cmake/pybind11")
#find_package(pybind11 REQUIRED)
#include_directories(${pybind11_INCLUDE_DIRS})

# 打印 Python 相关信息
message(STATUS "Python3_EXECUTABLE: ${Python3_EXECUTABLE}")
message(STATUS "Python3_INCLUDE_DIRS: ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3_LIBRARIES: ${Python3_LIBRARIES}")

# 添加子目录
add_subdirectory(Application)
add_subdirectory(IMGUI)

include_directories(
    SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/thirdParty/include
)

link_directories(
    SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/thirdParty/lib
)

# 本工程所有的cpp文件编译链接，生成exe
set(
    SRCS
    glad.c
    main.cpp
)

add_executable(Texture_projection ${SRCS})

target_link_libraries(Texture_projection glfw3 imgui app ${Python3_LIBRARIES})
