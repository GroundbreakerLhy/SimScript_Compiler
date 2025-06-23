cmake_minimum_required(VERSION 3.16)
project(SimScriptCompiler VERSION 1.0.0 LANGUAGES C CXX)

# 设置 C/C++ 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Flex 和 Bison
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# 查找 LLVM
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# 添加 LLVM 的头文件路径和编译定义
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# 查找需要的 LLVM 组件
llvm_map_components_to_libnames(llvm_libs support core irreader analysis)

# 设置编译器标志
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/frontend)
include_directories(${CMAKE_BINARY_DIR}/src/frontend)

# 使用 Flex 和 Bison 生成词法分析器和语法分析器
FLEX_TARGET(SimScriptLexer src/frontend/lexer.l ${CMAKE_BINARY_DIR}/src/frontend/lexer.c)
BISON_TARGET(SimScriptParser src/frontend/parser.y ${CMAKE_BINARY_DIR}/src/frontend/parser.c)
ADD_FLEX_BISON_DEPENDENCY(SimScriptLexer SimScriptParser)

# 源文件
set(FRONTEND_SOURCES
    ${FLEX_SimScriptLexer_OUTPUTS}
    ${BISON_SimScriptParser_OUTPUTS}
    src/frontend/ast.c
    src/frontend/symbol_table.c
)

set(CODEGEN_SOURCES
    src/codegen/codegen.cpp
)

set(MAIN_SOURCES
    src/main.cpp
)

# 创建可执行文件
add_executable(simscript_compiler
    ${FRONTEND_SOURCES}
    ${CODEGEN_SOURCES}
    ${MAIN_SOURCES}
)

# 链接 LLVM 库
target_link_libraries(simscript_compiler ${llvm_libs})

# 安装目标
install(TARGETS simscript_compiler DESTINATION bin)
