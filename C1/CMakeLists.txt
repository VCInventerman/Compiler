# CMakeList.txt : CMake project for C1, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (C1 "C1.cpp"  "preprocessor.h"  "ast.h" "token.h" "source.h" "util.h" "type.h" "parser.h" "llvmAsm.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET C1 PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
