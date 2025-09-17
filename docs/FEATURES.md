# SIMSCRIPT II.5 编译器功能

## 编译器特性

### 核心编译功能
- 编译可执行文件
- 对象文件生成（-c选项）
- JIT执行和调试支持

### 语言特性

#### 数据类型
- INT: 32位整数
- REAL: 双精度浮点数
- DOUBLE: 双精度浮点数
- TEXT: 字符串类型
- ALPHA: 字母数字类型
- SET: 集合类型

#### 程序结构
- PREAMBLE: 声明段
- MAIN: 主程序段
- 变量声明和赋值
- 函数定义和调用

#### 控制流
- IF/ELSEIF/ELSE: 条件分支
- WHILE: 循环结构
- FOR: 计数循环
- FOR EACH: 集合迭代

#### 函数系统
- ROUTINE: 函数定义
- 参数传递和类型检查
- 递归调用支持
- 返回值处理

### 高级特性

#### 面向对象编程
- CLASS: 类定义
- INHERITS: 继承机制
- 方法定义和重写
- 对象创建和方法调用
- THIS和SUPER关键字

#### OpenMP并行化
- PARALLEL DO: 并行循环
- PARALLEL SECTIONS: 并行段
- CRITICAL: 临界区
- BARRIER: 同步屏障
- MASTER/SINGLE: 线程控制
- THREADPRIVATE: 线程私有变量

#### 数据结构和实体
- ENTITY: 实体定义
- EVENT: 事件定义
- 属性和参数声明

### 标准库

#### 数据结构库
- 集合操作: CREATE_SET, ADD, REMOVE, CONTAINS, UNION, INTERSECTION
- 队列操作: CREATE_QUEUE, ENQUEUE, DEQUEUE, PEEK, IS_EMPTY
- 资源管理: CREATE_RESOURCE, REQUEST, RELEASE, GET_UTILIZATION

#### 数学和随机数库
- 随机数生成: RANDOM, UNIFORM, NORMAL, EXPONENTIAL, POISSON, SEED
- 统计函数: MEAN, VARIANCE, STDDEV, MEDIAN, MODE, CORRELATION, PERCENTILE

#### 时间模拟库
- 模拟器控制: SIMULATOR, SCHEDULE_EVENT, ADVANCE_TIME, RUN_SIMULATION
- 事件队列管理: EVENT_QUEUE

### 仿真和I/O

#### 仿真控制
- START SIMULATION: 仿真初始化
- SCHEDULE: 事件调度
- ADVANCE TIME: 时间推进

#### 文件操作
- OPEN/CLOSE: 文件打开关闭
- WRITE TO FILE: 文件写入
- READ FROM FILE: 文件读取

### 技术实现

#### 编译器架构
- 前端: Flex词法分析器 + Bison语法分析器
- 中间表示: LLVM IR生成
- 后端: 本地代码生成和链接器集成
- 调试支持: JIT执行引擎

## 兼容性

- LLVM 14.0+ 支持
- GCC/Clang链接器集成
- POSIX兼容系统
- OpenMP运行时支持