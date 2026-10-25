
cmake_minimum_required(VERSION 3.20)

project(LOB
    VERSION 0.1.0
    DESCRIPTION "A C++ limit order book"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_library(project_warnings INTERFACE)

target_compile_options(project_warnings
    INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
)

add_library(order_book
    src/order_book.cpp
)

target_include_directories(order_book
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(order_book
    PRIVATE project_warnings
)

add_executable(lob
    src/main.cpp
)

target_link_libraries(lob
    PRIVATE
        order_book
        project_warnings
)

if(BUILD_TESTING)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(googletest)

    add_executable(order_book_tests
        tests/order_book_test.cpp
    )

    target_link_libraries(order_book_tests
        PRIVATE
            order_book
            GTest::gtest_main
            project_warnings
    )

    include(GoogleTest)
    gtest_discover_tests(order_book_tests)
endif()
