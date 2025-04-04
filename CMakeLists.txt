cmake_minimum_required(VERSION 3.30)

project(c-d3d11 VERSION 1.0 LANGUAGES C)
# configure_file(njin.h.in njin.h)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
# set(UNITY_BUILD True)

# add_subdirectory(extern)

include_directories(src)
# include_directories(extern/haikal/src/meta/gen)
include_directories(extern/haikal/src/meta_arena/gen)
# include_directories(extern/bstring/bstring)

# target_include_directories(${PROJECT_NAME} PRIVATE extern/cglm/include)
include_directories(extern/cglm/include)

set(SOURCE 
    src/main.c
    # extern/bstring/bstring/bstrlib.c
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCE})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(${PROJECT_NAME} winmm.lib)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources)
