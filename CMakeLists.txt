project(aisdecoder)
cmake_minimum_required(VERSION 2.8)
INCLUDE(CheckIncludeFiles)

set(STATIC_LIB "${PROJECT_NAME}_static_lib")
set(DYNAMIC_LIB "${PROJECT_NAME}_dynamic_lib")

macro(get_WIN32_WINNT version)
    if (WIN32 AND CMAKE_SYSTEM_VERSION)
        set(ver ${CMAKE_SYSTEM_VERSION})
        string(REPLACE "." "" ver ${ver})
        string(REGEX REPLACE "([0-9])" "0\\1" ver ${ver})
        set(${version} "0x${ver}")
    endif()
endmacro()

if(NOT WIN32)
    #CHECK_INCLUDE_FILES(alsa/asoundlib.h HAVE_ALSA)
    #CHECK_INCLUDE_FILES(pulse/pulseaudio.h HAVE_PULSEAUDIO)
else()
    get_WIN32_WINNT(ver)
    if(${CMAKE_SYSTEM_VERSION} LESS 5.1)
        MESSAGE(FATAL_ERROR "Windows XP or higher is required")
    endif()
    add_definitions(-D_WIN32_WINNT=${ver})
endif()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(NOT $ENV{CMAKE_BUILD_TYPE} STREQUAL "")
    set(CMAKE_BUILD_TYPE $ENV{CMAKE_BUILD_TYPE})
endif()

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)
MESSAGE(STATUS "Build configuration: ${CMAKE_BUILD_TYPE}")

set(CMAKE_C_FLAGS_DEBUG "-g -Wall")
set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall")

if(${CMAKE_BUILD_TYPE} STREQUAL "RELEASE")
    set(CMAKE_EXE_LINKER_FLAGS "-s")
endif()




include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/lib)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_LIST)
set(SRC_LIST_LIB "${CMAKE_CURRENT_SOURCE_DIR}/src/sounddecoder.c")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/lib SRC_LIST_LIB)

if(NOT WIN32)
    if(HAVE_PULSEAUDIO)
        aux_source_directory(src/lib/pulseaudio SRC_LIST_LIB)
        set(L_PULSE pulse-simple)
    else()
        message(WARNING "You do not have the development files for pulseaudio (DISABLE).")
    endif()

    if(HAVE_ALSA)
        aux_source_directory(src/lib/alsaaudio SRC_LIST_LIB)
        set(L_ALSA asound)
    else()
        message(WARNING "You do not have the development files for ASLA (DISABLE).")
    endif()
else()
    aux_source_directory(src/lib/winmmaudio SRC_LIST_LIB)
endif()

add_library(${DYNAMIC_LIB} SHARED ${SRC_LIST_LIB})
set_target_properties(${DYNAMIC_LIB} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

add_library(${STATIC_LIB} STATIC ${SRC_LIST_LIB})
set_target_properties(${STATIC_LIB} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

if(WIN32)
    target_link_libraries(${DYNAMIC_LIB} winmm)
else()
    set(MYLIB_VERSION_MAJOR 1)
    set(MYLIB_VERSION_MINOR 0)
    set(MYLIB_VERSION_PATCH 0)
    set(MYLIB_VERSION_STRING ${MYLIB_VERSION_MAJOR}.${MYLIB_VERSION_MINOR}.${MYLIB_VERSION_PATCH})
    set_target_properties(${DYNAMIC_LIB}
                          PROPERTIES
                          VERSION ${MYLIB_VERSION_STRING}
                          SOVERSION ${MYLIB_VERSION_MAJOR})
endif()

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${STATIC_LIB})

if(NOT WIN32)
    target_link_libraries(${PROJECT_NAME} ${L_ALSA} ${L_PULSE})
else()
    target_link_libraries(${PROJECT_NAME} winmm ws2_32)
endif()
