cmake_minimum_required(VERSION 3.10)
# Enable Hot Reload for MSVC compilers if supported.
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if (POLICY CMP0141)
      cmake_policy(SET CMP0141 NEW)
      set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
    endif()
endif()

project(VideoCaptureDemo)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-finput-charset=UTF-8)
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/source-charset:utf-8)
endif()

find_package(OpenCV REQUIRED)
if (UNIX)
    find_package(X11 REQUIRED)
endif()
add_executable(faceblur video.cpp blur.cpp capture.cpp)

target_link_libraries(faceblur PRIVATE ${OpenCV_LIBS} ${X11_LIBRARIES})

message(STATUS "OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS = ${OpenCV_LIBS}")