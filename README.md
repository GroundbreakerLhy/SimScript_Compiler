# SIMSCRIPT II.5 编译器

基于 SIMSCRIPT II.5 语言规范的完整编译器

## 技术架构

| 组件 | 实现方案 | 功能 |
|------|----------|------|
| 词法分析 | Flex 2.6+ | Token流生成 |
| 语法分析 | Bison 3.0+ | AST构建 |
| 代码生成 | LLVM 14.0+ | IR代码生成 |
| 目标代码生成 | LLVM Target | 本地代码生成 |
| 链接器集成 | GCC Linker | 可执行文件链接 |
| JIT执行引擎 | LLVM ORC JIT | 实时代码执行 |
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
- 标准库: 随机数生成、统计函数、数据结构、时间模拟


## 项目结构

```
src/
├── main.cpp              # 编译器入口和命令行处理
├── frontend/             # 编译器前端
│   ├── lexer.l          # Flex词法规则
│   ├── parser.y         # Bison语法规则
│   ├── ast.[h|c]        # 抽象语法树
│   └── symbol_table.[h|c] # 符号表管理
├── codegen/              # 代码生成后端
│   ├── codegen.h        # 代码生成接口
│   └── codegen.cpp      # LLVM IR和本地代码生成
├── debug/                # 调试和JIT执行
│   ├── debug.[h|c]      # 调试接口
│   ├── debug_runtime.[h|c] # 运行时调试支持
│   ├── graph.[h|c]      # 可视化调试
│   └── graph_runtime.c  # 图形运行时
└── stdlib/               # 标准库实现
    ├── data_structures/ # 数据结构
    ├── math/            # 数学函数
    └── time_simulation/ # 时间模拟

docs/                    # 技术文档
CMakeLists.txt          # 构建配置
```

### 系统要求
```bash
# Ubuntu/Debian
sudo apt-get install flex bison llvm-14-dev cmake build-essential clang

# macOS
brew install flex bison llvm@16 cmake libomp && export OpenMP_ROOT=/opt/homebrew/opt/libomp
```

### 编译流程
```bash
mkdir -p build && cd build && cmake .. && make -j$(getconf _NPROCESSORS_ONLN)
```

## 技术文档

- [功能特性说明](docs/FEATURES.md)
- [语法规范](docs/syntax.md)
