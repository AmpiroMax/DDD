cmake_minimum_required(VERSION 3.20)
project(DDDGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared libs" ON)

# Dependencies
find_package(SFML 2.5 REQUIRED COMPONENTS system window graphics)
find_package(Box2D REQUIRED)
find_package(nlohmann_json REQUIRED)

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(GLOB_RECURSE SOURCES
    ${SRC_ROOT}/*.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_ROOT})

target_link_libraries(${PROJECT_NAME}
    sfml-system
    sfml-window
    sfml-graphics
    Box2D
    nlohmann_json::nlohmann_json
)

if(APPLE)
    # On macOS, ensure we use modern rpaths for SFML frameworks if present
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_RPATH ON
    )
endif()

