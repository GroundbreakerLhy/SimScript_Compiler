# SIMSCRIPT II.5 语法规范

## 程序结构

### 基本组成
```simscript
PREAMBLE                        # 声明段
    DEFINE declarations
    ENTITY definitions  
    EVENT definitions
    ROUTINE definitions
END PREAMBLE

MAIN                           # 主程序段
    executable statements
END MAIN
```

### 语句规则
- 每行一条语句
- 续行符: `&`
- 注释符: `''`

## 数据类型

### 基本类型
| 类型 | 声明语法 | 用途 |
|------|----------|------|
| INT | `DEFINE var AS INT` | 32位整数 |
| REAL | `DEFINE var AS REAL` | 双精度浮点 |
| DOUBLE | `DEFINE var AS DOUBLE` | 双精度浮点 |
| TEXT | `DEFINE var AS TEXT` | 字符串 |
| ALPHA | `DEFINE var AS ALPHA` | 字符串 |
| SET | `DEFINE var AS SET` | 集合类型 |

### 复合类型
```simscript
ENTITY entity_name              # 实体定义
    ATTRIBUTES
        field1: type,
        field2: type
END

EVENT event_name                # 事件定义  
    PARAMETERS
        param1: type,
        param2: type
END
```

## 运算符

### 算术运算
| 运算符 | 功能 | 优先级 |
|--------|------|--------|
| `**` | 幂运算 | 1 |
| `*`, `/` | 乘除 | 2 |
| `+`, `-` | 加减 | 3 |

### 比较运算
| 运算符 | 功能 |
|--------|------|
| `=` | 等于 |
| `<>` | 不等于 |
| `<`, `>` | 小于、大于 |
| `<=`, `>=` | 小于等于、大于等于 |

### 逻辑运算
| 运算符 | 功能 |
|--------|------|
| `AND` | 逻辑与 |
| `OR` | 逻辑或 |
| `NOT` | 逻辑非 |

### 集合运算
| 运算符 | 功能 |
|--------|------|
| `IN` | 元素属于集合 |
| `FIRST.IN` | 获取首元素 |
| `LAST.IN` | 获取尾元素 |

## 控制流语句

### 条件语句
```simscript
IF condition THEN
    statements
ELSEIF condition THEN
    statements  
ELSE
    statements
ALWAYS
```

### 循环语句
```simscript
# 计数循环
FOR variable = start TO end [STEP step] DO
    statements
LOOP

# 条件循环
WHILE condition DO
    statements
LOOP

# 集合遍历
FOR EACH element IN set DO
    statements
LOOP
```

## 函数定义

### 函数语法
```simscript
ROUTINE function_name(param1: type, param2: type) = return_type
    statements
    RETURN expression
END
```

### 调用语法
```simscript
LET result = function_name(arg1, arg2)
```

## 变量操作

### 声明与赋值
```simscript
DEFINE variable AS type         # 声明
LET variable = expression       # 赋值
```

### 作用域规则
- 全局变量: PREAMBLE中声明
- 局部变量: 函数内声明
- 参数变量: 函数参数列表

## 输入输出

### 屏幕输出
```simscript
WRITE expression TO SCREEN
```

### 文件操作
```simscript
OPEN "filename" AS FILE number
WRITE expression TO FILE "filename"
READ variable FROM FILE number
CLOSE FILE number
```

## 仿真控制

### 仿真语句
```simscript
START SIMULATION                # 初始化仿真
SCHEDULE event AT time          # 调度事件
SCHEDULE event AT time WITH parameters
ADVANCE TIME BY delta           # 推进时间
```

### 时间管理
```simscript
LET current_time = TIME.V       # 获取当前时间
```

## 资源管理

### 资源定义
```simscript
RESOURCE resource_name
    CAPACITY: number
END
```

### 资源操作
```simscript
ACQUIRE units OF resource
USE resource FOR duration
RELEASE resource
```

## 错误处理

### 异常捕获
```simscript
MONITOR
    statements
WHEN ERROR
    error_handling_statements
END
```

## 统计收集

### 统计定义
```simscript
DEFINE statistics AS TALLY
    FOR target_variable
    REPORT MEAN, MAX, MIN
END
```

### 数据收集
```simscript
OBSERVE statistics WITH value
```

## 语法约束

### 命名规则
- 标识符: 字母开头，可包含字母、数字、下划线
- 关键字: 大小写不敏感
- 字符串: 双引号包围

### 类型规则  
- 强类型检查
- 禁止隐式类型转换
- 集合操作需类型匹配

### 作用域规则
- 变量默认局部作用域
- 全局变量需在PREAMBLE声明
- 函数参数局部可见
