cmake_minimum_required(VERSION 3.27)
project(display_opt C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(TARGET_NAME display_opt)

add_executable(${TARGET_NAME}
               src/main.c
               src/video_adapter.h
               src/video_adapter.c
               src/dpi.h
               src/dpi.c)

target_compile_definitions(${TARGET_NAME} PUBLIC _WIN32_WINNT=0x0A00)
target_link_libraries(${TARGET_NAME} -static) # TLS library static linking