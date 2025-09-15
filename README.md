# SIMSCRIPT II.5 编译器

基于 SIMSCRIPT II.5 语言规范的编译器前端实现，采用 Flex/Bison 工具链构建词法语法分析器，生成 LLVM IR 中间代码。

## 技术架构

| 组件 | 实现方案 | 功能 |
|------|----------|------|
| 词法分析 | Flex 2.6+ | Token流生成 |
| 语法分析 | Bison 3.0+ | AST构建 |
| 代码生成 | LLVM 14.0+ | IR代码生成 |
| 构建系统 | CMake 3.16+ | 自动化构建 |

## 语言特性支持

### 核心语法
- 数据类型：INT, REAL, DOUBLE, TEXT, ALPHA, SET
- 程序结构：PREAMBLE声明段, MAIN主程序段
- 变量操作：DEFINE声明, LET赋值
- 控制流：IF/ELSEIF/ELSE, WHILE, FOR, FOR EACH

### 高级特性
- 函数定义：ROUTINE 定义与调用，参数传递
- 数据结构：ENTITY 实体, EVENT 事件
- 表达式：算术运算、逻辑运算、比较运算
- 输入输出：WRITE TO SCREEN, 文件 I/O 语法
- 仿真支持：START SIMULATION, SCHEDULE 事件调度

## 项目结构

```
src/
├── frontend/              # 编译器前端
│   ├── lexer.l           # Flex词法规则
│   ├── parser.y          # Bison语法规则
│   ├── ast.[h|c]         # 抽象语法树
│   └── symbol_table.[h|c] # 符号表管理
├── codegen/              # 代码生成后端
│   ├── codegen.h         # 代码生成接口
│   └── codegen.cpp       # LLVM IR生成
└── main.cpp              # 编译器入口

tests/basic/              # 测试用例集合
docs/                     # 技术文档
CMakeLists.txt           # 构建配置
build.sh                 # 构建脚本
test.sh                  # 测试脚本
```

## 构建与安装

### 系统要求
```bash
# Ubuntu/Debian
sudo apt-get install flex bison llvm-14-dev cmake build-essential

# macOS
brew install flex bison llvm@16 cmake
```

### 编译流程
```bash
./build.sh
```

### 手动构建
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 使用说明

### 基本编译
```bash
./build/simscript_compiler input.sim -o output.ll
llc output.ll -o output.s
gcc -no-pie output.s -o executable
./executable
```

### 调试选项
```bash
./build/simscript_compiler input.sim --print-ast    # 输出语法树
./build/simscript_compiler input.sim --print-ir     # 输出LLVM IR
```

### 测试验证
```bash
./test.sh    # 运行完整测试套件
```

## 实现状态

| 功能模块 | 状态 | 备注 |
|----------|------|------|
| 词法语法分析 | ✓ | 完整支持SIMSCRIPT语法 |
| 类型系统 | ✓ | 支持基本类型与自动推断 |
| 控制流语句 | ✓ | IF/WHILE/FOR/ELSEIF |
| 函数系统 | ✓ | 定义、调用、参数传递 |
| 实体事件 | ✓ | 语法解析完成 |
| 仿真控制 | ✓ | 语法解析完成 |
| 文件I/O | ✓ | 语法解析完成 |
| 运行时库 | ○ | 仿真引擎待实现 |

## 技术文档

- [实现状态详细说明](docs/IMPLEMENTATION_STATUS.md)
- [语法规范](docs/syntax.md)
