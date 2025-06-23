# SIMSCRIPT II.5 编译器开发文档

## 项目概述

这是一个基于 SIMSCRIPT II.5 语言的编译器前端实现，使用 Flex/Bison 自动生成前端，C/C++ 混合开发，能够将 SIMSCRIPT 源代码编译为 LLVM IR。

**注意**: 所有 SIMSCRIPT 源文件使用 `.sim` 扩展名。

## 编译器架构

### 1. 词法分析器 (Lexer)
- **位置**: `src/frontend/lexer.l`
- **工具**: Flex 自动生成
- **功能**: 将源代码转换为 Token 流
- **特性**: 
  - 支持 SIMSCRIPT 关键字、标识符、字面量
  - 行内注释支持 (`''`)
  - 位置跟踪 (行号、列号)

### 2. 语法分析器 (Parser)
- **位置**: `src/frontend/parser.y`
- **工具**: Bison 自动生成
- **功能**: 将 Token 流转换为抽象语法树 (AST)
- **特性**:
  - LALR(1) 语法分析
  - 错误恢复机制
  - 直接构建 AST

### 3. 抽象语法树 (AST)
- **位置**: `src/frontend/ast.h`, `src/frontend/ast.c`
- **语言**: C
- **功能**: 表示程序的语法结构
- **特性**:
  - 工厂函数模式
  - 内存管理
  - 调试输出
  - 遍历接口

### 4. 代码生成器 (Code Generator)
- **位置**: `src/codegen/`
- **功能**: 生成 LLVM IR 代码
- **位置**: `src/codegen/codegen.h`, `src/codegen/codegen.cpp`  
- **语言**: C++
- **功能**: 遍历 AST 并生成 LLVM IR
- **特性**:
  - 基于 LLVM C API
  - 支持基本表达式和语句
  - printf 函数调用生成

## 当前支持的语言特性

### 已实现
- ✅ **基本数据类型**: INT, REAL, DOUBLE, TEXT, ALPHA, SET
- ✅ **数学表达式**: +, -, *, /, ** 运算符
- ✅ **比较运算符**: =, <>, <, >, <=, >=
- ✅ **逻辑运算符**: AND, OR, NOT
- ✅ **输出语句**: `WRITE expression TO SCREEN`
- ✅ **注释**: 行内注释 `''`
- ✅ **程序结构**: PREAMBLE, MAIN 代码块
- ✅ **变量声明**: `DEFINE name AS type`，支持初始化
- ✅ **变量赋值**: `LET variable = expression`，支持类型推断
- ✅ **控制结构**: IF-THEN-ELSE, FOR, WHILE 循环
- ✅ **函数定义**: ROUTINE 定义和调用，支持参数和返回值
- ✅ **实体定义**: ENTITY 和 EVENT 声明，支持属性和参数
- ✅ **字面量**: 整数、浮点数、字符串
- ✅ **符号表**: 变量作用域管理，函数参数处理

### 部分实现
- 🔄 **集合操作**: SET 类型基础框架（语法解析完成，运行时操作待实现）

### 计划实现
- � **模拟功能**: 事件调度、时间管理
- 📋 **高级集合操作**: 并集、交集、差集的完整实现
- 📋 **错误处理**: 更完善的错误检测和报告
- 📋 **优化**: 代码优化和性能改进

## 语言示例

### 基本程序
```simscript
MAIN
    WRITE 42 TO SCREEN
END MAIN
```

### 函数定义和调用
```simscript
PREAMBLE
    ROUTINE add_numbers(a: INT, b: INT) = INT
        RETURN a + b
    END
END PREAMBLE

MAIN
    LET result = add_numbers(10, 20)
    WRITE result TO SCREEN
END MAIN
```

### 控制结构
```simscript
MAIN
    LET x = 10
    
    IF x > 5 THEN
        WRITE 1 TO SCREEN
    ELSE
        WRITE 0 TO SCREEN
    ALWAYS
    
    FOR i = 1 TO 5 DO
        WRITE i TO SCREEN
    LOOP
    
    LET count = 3
    WHILE count > 0 DO
        WRITE count TO SCREEN
        LET count = count - 1
    LOOP
END MAIN
```

### 实体和事件定义
```simscript
PREAMBLE
    ENTITY Student
        ATTRIBUTES
            id: INT,
            name: TEXT,
            gpa: REAL
    END
    
    EVENT StudentEnrolled
        PARAMETERS
            student_id: INT,
            course_code: TEXT
    END
END PREAMBLE

MAIN
    LET student_count = 100
    WRITE student_count TO SCREEN
END MAIN
```

### 数学运算
```simscript  
MAIN
    WRITE 10 + 5 TO SCREEN
    WRITE 20 * 3 TO SCREEN
    WRITE 100 / 4 TO SCREEN
    WRITE 2 ** 3 TO SCREEN
END MAIN
```

### 带变量声明
```simscript
PREAMBLE
    DEFINE count AS INT
    DEFINE total AS REAL
END PREAMBLE

MAIN
    WRITE 123 TO SCREEN
END MAIN

```

## 编译流程

1. **预处理**: 读取源文件
2. **词法分析**: 生成 Token 序列
3. **语法分析**: 构建 AST
4. **语义分析**: 类型检查和符号解析
5. **代码生成**: 生成 LLVM IR
6. **后端编译**: 使用 LLVM 生成机器码

## 使用示例

### 基本编译
```bash
./simscript_compiler input.sim -o output.ll
```

### 调试选项
```bash
# 仅词法分析
./simscript_compiler input.sim --lex-only

# 仅语法分析
./simscript_compiler input.sim --parse-only

# 仅语义分析
./simscript_compiler input.sim --semantic-only

# 打印生成的 IR
./simscript_compiler input.sim --print-ir
```

### 完整编译流程
```bash
# 生成 LLVM IR
./simscript_compiler input.sim -o output.ll

# 生成汇编代码
llc output.ll -o output.s

# 生成可执行文件
gcc output.s -o executable
```

## 错误处理

编译器提供详细的错误信息，包括：
- 词法错误：非法字符、未终止的字符串等
- 语法错误：语法规则违反、缺失的分隔符等
- 语义错误：类型不匹配、未定义的标识符等
- 代码生成错误：LLVM IR 生成失败等

## 扩展性

编译器使用访问者模式设计，便于添加新的分析阶段或代码生成目标：

1. **添加新的 AST 节点**：在 `ast.h` 中定义新节点
2. **扩展访问者接口**：在 `ASTVisitor` 中添加新方法
3. **实现访问方法**：在各个分析器中实现新节点的处理
