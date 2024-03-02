cmake_minimum_required(VERSION 3.22)
project(stdpar_ccl)

find_package(OpenCV REQUIRED)

include_directories(src/)

add_executable(${PROJECT_NAME} src/main.cpp src/CCL.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBRARIES})

set(ACCEL_TARGET gpu)
target_compile_definitions(${PROJECT_NAME} PUBLIC COMPILE_FOR_GPU)
target_compile_options(${PROJECT_NAME} PRIVATE -Minfo -fast -march=native -Mllvm-fast -stdpar=${ACCEL_TARGET})
target_link_options(${PROJECT_NAME} PRIVATE -Minfo -fast -march=native -Mllvm-fast -stdpar=${ACCEL_TARGET})
