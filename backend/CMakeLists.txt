cmake_minimum_required(VERSION 3.16)
project(mini_js_runtime_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mini_js_runtime_backend
  main.cpp
  server_minimal.cpp
  runtime.cpp
)


target_include_directories(mini_js_runtime_backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
  target_compile_options(mini_js_runtime_backend PRIVATE /W4)
else()
  # httplib bundled here expects a reasonably complete Win32 + POSIX/C++ runtime.
  # This project is built with MinGW (GCC 6.3) on Windows.
  #
  # Force pthread/thread support and ensure Windows versioned APIs are visible.
  target_compile_options(mini_js_runtime_backend PRIVATE
    -Wall -Wextra -Wpedantic
    -pthread
    -DWIN32_LEAN_AND_MEAN
    -D_WIN32_WINNT=0x0A00

  )
endif()




# Windows + MSVC:
# - We use WinSock2 for sockets and standard C++17.
# - Links are set automatically by the toolchain, but keep this for completeness.
target_link_libraries(mini_js_runtime_backend PRIVATE ws2_32)


