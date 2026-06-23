cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(XValid LANGUAGES CXX)

set(Boost_NO_BOOST_CMAKE ON CACHE BOOL "Use CMake's FindBoost module for ETISS dependencies")
find_package(ETISS REQUIRED)

if(NOT COMMAND ETISSPlugin)
    if(ETISS_DIR AND EXISTS "${ETISS_DIR}/../cmake/ETISSPlugin.cmake")
        include("${ETISS_DIR}/../cmake/ETISSPlugin.cmake")
    else()
        message(FATAL_ERROR "ETISSPlugin.cmake was not provided by ETISSConfig.cmake. "
                            "Install ETISS or point ETISS_DIR at an ETISS build tree.")
    endif()
endif()

set(XVALID_ETISS_EXTRA_INCLUDE_DIRS)
if(ETISS_DIR AND EXISTS "${ETISS_DIR}/../include/etiss/Plugin.h")
    list(APPEND XVALID_ETISS_EXTRA_INCLUDE_DIRS
        "${ETISS_DIR}/../include"
        "${ETISS_DIR}/../include_c"
        "${ETISS_DIR}/include"
        "${ETISS_DIR}/include_c"
        "${ETISS_DIR}/include/jit"
        "${ETISS_DIR}/../third_party/betterenums"
        "${ETISS_DIR}/../third_party/simpleini"
    )
    link_directories("${ETISS_DIR}/lib")
endif()

add_library(${PROJECT_NAME} SHARED
    src/DataWriteTracer.cpp
    src/ISAExtensionValidator.cpp
    src/InstructionTracer.cpp
    src/TraceFileWriter.cpp
    src/XValidLibrary.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>"
    PRIVATE
        ${ETISS_INCLUDE_DIRS}
        ${XVALID_ETISS_EXTRA_INCLUDE_DIRS}
)

foreach(etiss_include_dir ${ETISS_INCLUDE_DIRS})
    if(IS_DIRECTORY "${etiss_include_dir}/jit")
        target_include_directories(${PROJECT_NAME} PRIVATE "${etiss_include_dir}/jit")
    endif()
endforeach()

if(DEFINED CMAKE_CXX_STANDARD)
    set(XVALID_CXX_STANDARD "${CMAKE_CXX_STANDARD}")
else()
    set(XVALID_CXX_STANDARD 17)
endif()
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_${XVALID_CXX_STANDARD})

ETISSPlugin(${PROJECT_NAME})

install(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.h")
