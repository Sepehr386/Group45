cmake_minimum_required(VERSION 3.26)
project(14041016)

set(CMAKE_CXX_STANDARD 17)
set(MINGW_PATH "C:/MinGW")

include_directories(${MINGW_PATH}/include)
link_directories(${MINGW_PATH}/lib)

add_executable(14041016
        Hw8_Q3_404102245.cpp
        myheader.h
        main.cpp
)

target_link_libraries(14041016
        mingw32
        SDL2main
        SDL2
        SDL2_image
        SDL2_ttf
        SDL2_mixer
)
