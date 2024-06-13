cmake_minimum_required(VERSION 3.5)

# Set IPP root directory
set(IPP_ROOT_DIR "/opt/intel/oneapi/ipp/2021.11")

# Specify the directory containing the FindIPP.cmake file
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 11)
project(aws-lambda-ipp LANGUAGES CXX)

find_package(aws-lambda-runtime)
find_package(AWSSDK COMPONENTS core REQUIRED)
find_package(IPP REQUIRED)

add_executable(${PROJECT_NAME} "main.cpp")

target_link_libraries(${PROJECT_NAME} PUBLIC AWS::aws-lambda-runtime ${AWSSDK_LINK_LIBRARIES})
if(IPP_FOUND)
  include_directories(${IPP_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PRIVATE ${IPP_LIBRARIES})
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    "-Wall"
    "-Wextra"
    "-Wconversion"
    "-Wshadow"
    "-Wno-sign-conversion")

target_compile_features(${PROJECT_NAME} PRIVATE "cxx_std_11")

aws_lambda_package_target(${PROJECT_NAME})
