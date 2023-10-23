# CMakeList.txt : CMake project for C1, include source and define
# project specific logic here.
#

add_executable (C1 "src/C1.cpp" "src/expression.cpp")

target_include_directories(C1 PRIVATE include)
target_link_directories(C1 PRIVATE src)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET C1 PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
