cmake_minimum_required(VERSION 3.20)
project(DDDGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Speed up builds if ccache is available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

option(BUILD_SHARED_LIBS "Build shared libs" ON)

# Prefer Homebrew paths on macOS to avoid unsigned /Library/Frameworks SFML.
if(APPLE)
    # Prefer local built SFML (used in Chest) to avoid unsigned /Library/Frameworks.
    set(SFML_LOCAL_ROOT "/Users/ampiro/programs/libs/build-mac-sfml")
    if(EXISTS "${SFML_LOCAL_ROOT}/SFMLConfig.cmake")
        set(SFML_DIR "${SFML_LOCAL_ROOT}")
        list(PREPEND CMAKE_PREFIX_PATH "${SFML_LOCAL_ROOT}")
        set(SFML_CUSTOM_LIB "${SFML_LOCAL_ROOT}/lib")
    else()
        list(PREPEND CMAKE_PREFIX_PATH /opt/homebrew)
        list(PREPEND CMAKE_FRAMEWORK_PATH /opt/homebrew/Frameworks /Library/Frameworks)
        set(SFML_ROOT /opt/homebrew/opt/sfml)
        set(SFML_DIR "/opt/homebrew/opt/sfml/lib/cmake/SFML")
        set(SFML_CUSTOM_LIB "/opt/homebrew/opt/sfml/lib")
    endIf()
endif()

# Dependencies
find_package(SFML 2.5 REQUIRED COMPONENTS system window graphics)
find_package(Box2D REQUIRED)
find_package(nlohmann_json REQUIRED)

# Normalize Box2D target/import variables across package variants.
set(DDD_BOX2D_TARGET "")
if(TARGET box2d::box2d)
    set(DDD_BOX2D_TARGET box2d::box2d)
elseif(TARGET Box2D::Box2D)
    set(DDD_BOX2D_TARGET Box2D::Box2D)
elseif(DEFINED Box2D_LIBRARIES)
    set(DDD_BOX2D_TARGET ${Box2D_LIBRARIES})
endif()

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
    ${DDD_BOX2D_TARGET}
    nlohmann_json::nlohmann_json
)

if(DEFINED Box2D_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Box2D_INCLUDE_DIRS})
endif()

if(APPLE)
    # Ensure rpaths include Homebrew libs so SFML dylibs load without manual install_name_tool.
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_RPATH ON
        BUILD_WITH_INSTALL_RPATH ON
        INSTALL_RPATH "${SFML_CUSTOM_LIB};/opt/homebrew/lib;/opt/homebrew/opt/sfml/lib;/opt/homebrew/Frameworks"
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        "-Wl,-rpath,${SFML_CUSTOM_LIB}"
        "-Wl,-rpath,/opt/homebrew/lib"
        "-Wl,-rpath,/opt/homebrew/opt/sfml/lib"
        "-Wl,-rpath,/opt/homebrew/Frameworks"
    )
endif()

