# 获取 Application 目录中所有 cpp 文件
file(GLOB Application
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

# 添加 Include 目录为头文件路径
# 这样整个项目的其他源文件都能通过 #include 访问头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Include)

# 添加 IMGUI 和其他第三方库的头文件路径（如果需要）
include_directories(
    SYSTEM ../thirdParty/include/IMGUI
    SYSTEM ../thirdParty/include
)

# 定义 app 静态库或动态库
add_library(app ${Application})
