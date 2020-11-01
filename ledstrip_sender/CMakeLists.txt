cmake_minimum_required(VERSION 3.5)

project(ledstrip_sender LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic")

### source files
set( SOURCE_FILES
     "src/main.cpp"
     )

set( INCLUDE_FILES
    )

set( LIBRARIES
    )

### main program
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${INCLUDE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRARIES})
