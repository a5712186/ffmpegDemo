# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

include_directories(
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/include #h文件目录
    ${CMAKE_SOURCE_DIR}/libs/include
    ${CMAKE_SOURCE_DIR}/src/main/cpp/include/${ANDROID_ABI}
)



add_library( jni-lib                        #c/cpp代码将要编译成为so库的名称，java代码加载库文件要用这个名称
             SHARED
             src/main/cpp/native-lib.cpp  #cpp代码文件路径 这里可以随意添加c、c++文件
             src/main/cpp/audio-lib.cpp
             src/main/cpp/FFmpegMusic.cpp
             src/main/cpp/FFmpegVideoMusic.cpp
             src/main/cpp/FFmpegVideo.cpp
             src/main/cpp/native-lib-video.cpp
             )
add_library(
            avcodec
            SHARED
            IMPORTED
            )

add_library(
            avfilter
            SHARED
            IMPORTED
             )


add_library(
            avformat
            SHARED
            IMPORTED
            )


add_library(
            avutil
            SHARED
            IMPORTED
            )

add_library(
            swresample
            SHARED
            IMPORTED
            )

add_library(
            swscale
            SHARED
            IMPORTED
            )


add_library(
            fdk-aac
            SHARED
            IMPORTED
            )

set_target_properties(
    avcodec
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libavcodec.so
    )

set_target_properties(
        avfilter
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libavfilter.so
        )

set_target_properties(
            avformat
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libavformat.so
            )

set_target_properties(
            avutil
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libavutil.so
            )

set_target_properties(
            swresample
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libswresample.so
             )

set_target_properties(
            swscale
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libswscale.so
             )

set_target_properties(
            fdk-aac
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libfdk-aac.so
             )
find_library( # Sets the name of the path variable.
              log-lib
              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )
target_link_libraries( # Specifies the target library.
                       jni-lib
                       android # Add this.
                       fdk-aac
                       avcodec
                       avfilter
                       avformat
                       avutil
                       swresample
                       swscale
                     ${log-lib}#这个是打印jni调试log要用到的库文件这里添加进来，最后打印视频时长就是用这个库打印
                     OpenSLES
                     )