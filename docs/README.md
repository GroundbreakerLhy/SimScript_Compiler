# SIMSCRIPT II.5 编译器./build/simscript_compiler tests/basic/test_simple.sim --print-ir
./build/simscript_compiler tests/basic/test_math.sim --print-ir

这是一个基于 SIMSCRIPT II.5 语言的编译器前端实现，使用 Flex/Bison 自动生成词法和语## 支持的语言特性

- [x] 基本数据类型 (INT, REAL, DOUBLE, TEXT, ALPHA)
- [x] 控制流语句 (IF/WHILE/FOR)
- [x] 变量声明和赋值
- [x] 数学表达式计算 (+, -, *, /)
- [x] 输入输出操作 (WRITE TO SCREEN)
- [x] 注释支持 (行内注释 '' )
- [x] PREAMBLE 和 MAIN 代码块
## 测试

```bash
# 运行基本测试
./build/simscript_compiler tests/basic/test_simple.ss --print-ir
./build/simscript_compiler tests/basic/test_math.ss --print-ir

# 编译并运行生成的代码
llc output.ll -o output.s
gcc -no-pie output.s -o executable
./executable
```
MSCRIPT 源代码编译为 LLVM IR。

## 项目结构

```
SimScript_Compiler/
├── src/                    # 源代码目录
│   ├── frontend/          # 前端组件 (Flex/Bison)
│   │   ├── lexer.l        # Flex 词法分析器规则
│   │   ├── parser.y       # Bison 语法分析器规则
│   │   ├── ast.h          # AST 节点定义
│   │   ├── ast.c          # AST 节点实现
│   │   ├── symbol_table.h # 符号表定义
│   │   └── symbol_table.c # 符号表实现
│   ├── codegen/           # LLVM IR 代码生成器
│   │   ├── codegen.h      # 代码生成器接口
│   │   └── codegen.cpp    # 代码生成器实现
│   └── main.cpp           # 编译器主程序
├── tests/                 # 测试用例 (*.sim)
│   └── basic/             # 基础功能测试
├── docs/                  # 文档
├── CMakeLists.txt         # CMake 构建文件
├── build.sh               # 构建脚本
├── test.sh                # 测试脚本
└── README.md              # 项目说明
```

## 依赖项

- **Flex** 2.6+ (词法分析器生成器)
- **Bison** 3.0+ (语法分析器生成器)
- **LLVM** 16.0+ (用于 IR 生成)
- **CMake** 3.16+ (构建系统)
- **GCC/Clang** (C/C++17 编译器)

## 编译器架构

1. **词法分析** - Flex 自动生成的词法分析器，将源代码转换为 Token 流
2. **语法分析** - Bison 自动生成的语法分析器，构建抽象语法树 (AST)
3. **代码生成** - 遍历 AST 生成 LLVM IR

## 构建方法

### 安装依赖 (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install flex bison llvm-16-dev cmake build-essential
```

### 编译
```bash
chmod +x build.sh
./build.sh
```

或者手动编译：
```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

## 使用方法

```bash
# 编译 SIMSCRIPT 源文件
./simscript_compiler input.sim -o output.ll

# 查看生成的 AST
./simscript_compiler input.sim --print-ast

# 查看生成的 LLVM IR
./simscript_compiler input.sim --print-ir

# 使用 LLVM 工具链生成可执行文件
llc output.ll -o output.s
gcc output.s -o executable
```

## 支持的语言特性

- [x] 基本数据类型 (INT, REAL, DOUBLE, TEXT, ALPHA)
- [x] 实体 (ENTITY) 和事件 (EVENT) 定义
- [x] 控制流语句 (IF/WHILE/FOR)
- [x] 函数和过程定义 (ROUTINE)
- [x] 变量声明和赋值
- [x] 数学表达式计算
- [x] 输入输出操作 (WRITE/READ)
- [ ] 集合操作 (计划中)
- [ ] 仿真特性 (计划中)

## 开发状态

- ✅ Flex 词法分析器
- ✅ Bison 语法分析器  
- ✅ AST 定义和构建
- ✅ LLVM IR 代码生成器
- ✅ 基本语法支持
- 🔄 扩展语法特性
- 📋 测试和文档完善

## 架构优势

- **自动化前端**: 使用 Flex/Bison 自动生成词法和语法分析器，减少手写代码
- **简洁设计**: 精简的 C/C++ 混合架构，易于维护和扩展
- **标准工具链**: 基于成熟的编译器构建工具，稳定可靠
