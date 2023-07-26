cmake_minimum_required(VERSION 3.16)
project(ex5)

set(CMAKE_CXX_STANDARD 11)

add_executable(ex5
#        main.cpp
        client.h
        globals.h
        server.h
        server.cpp
        client.cpp
        server_main.cpp
#        client_main.cpp
        )
