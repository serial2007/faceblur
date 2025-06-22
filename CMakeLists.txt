cmake_minimum_required(VERSION 3.10)
project(VideoCaptureDemo)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
# 查找 OpenCV
find_package(OpenCV REQUIRED)
find_package(X11 REQUIRED)

# 添加可执行文件
add_executable(faceblur video.cpp blur.cpp capture.cpp)

# 链接 OpenCV 库
target_link_libraries(faceblur PRIVATE ${OpenCV_LIBS} ${X11_LIBRARIES})
