# CMakeList.txt : CMake project for space, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_library (algo_linear "algoLinear.h" "algoLinear.cpp" "algo_dumbo.h" "algo_dumbo.cpp" "algo_dumbo_impl.h" "algo_dumbo_impl.cpp")
target_link_libraries (algo_linear chess common)


