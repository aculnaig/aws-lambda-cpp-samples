cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)

project(jpeg-api LANGUAGES CXX)

find_package(aws-lambda-runtime REQUIRED)
find_package(AWSSDK COMPONENTS core REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ImageMagick REQUIRED IMPORTED_TARGET
    MagickWand-7.Q16HDRI
)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} PUBLIC AWS::aws-lambda-runtime ${AWSSDK_LINK_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::ImageMagick)

aws_lambda_package_target(${PROJECT_NAME})
