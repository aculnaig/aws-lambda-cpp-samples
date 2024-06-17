cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)

project(zimg-api LANGUAGES CXX)

find_package(AWSSDK COMPONENTS core REQUIRED)
find_package(aws-lambda-runtime REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZIMG REQUIRED IMPORTED_TARGET zimg)

add_executable(${PROJECT_NAME} "main.cpp")

find_package(AWSSDK COMPONENTS core REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC AWS::aws-lambda-runtime ${AWSSDK_LINK_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::ZIMG)

target_compile_options(${PROJECT_NAME} PRIVATE
    "-Wall"
    "-Wextra"
    "-Wconversion"
    "-Wshadow"
    "-Wno-sign-conversion"
    "-mavx2")

target_compile_features(${PROJECT_NAME} PRIVATE "cxx_std_11")

aws_lambda_package_target(${PROJECT_NAME})
