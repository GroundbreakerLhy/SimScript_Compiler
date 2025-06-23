以下是 SIMSCRIPT 语言的详细语法规则，基于官方文档、专利说明和技术规范的系统整理：

### 一、基础语法结构
1. **程序组成**：
   ```simscript
   PREAMBLE '' 声明部分
     DEFINE ...
     ENTITY ...
     EVENT ...
   END PREAMBLE

   MAIN '' 主程序
     START SIMULATION
       INITIALIZE ...
       SCHEDULE ...
     END
   END MAIN
   ```

2. **语句分隔符**：
   - 每行一条语句
   - 跨行语句使用续行符 `&`：
     ```simscript
     LET total = item_count * &
                 unit_price
     ```

### 二、数据类型与声明
1. **基本类型声明**：
   ```simscript
   DEFINE var_name AS INT
   DEFINE var_name AS REAL
   DEFINE var_name AS DOUBLE
   DEFINE var_name AS TEXT
   DEFINE var_name AS ALPHA
   ```

2. **复合类型声明**：
   - 实体声明：
     ```simscript
     ENTITY Customer
       ATTRIBUTES
         id: INT,
         arrival_time: REAL,
         priority: INT
     END
     ```
   - 事件声明：
     ```simscript
     EVENT Arrival
       PARAMETERS
         customer: Customer
     END
     ```

3. **集合类型**：
   ```simscript
   DEFINE WaitingQueue AS SET
     ORDER: FIFO '' 或 LIFO/PRIORITY
     OF Customer
   ```

### 三、运算符与表达式
| 类别     | 运算符                          | 示例                     |
| -------- | ------------------------------- | ------------------------ |
| 算术运算 | `+`, `-`, `*`, `/`, `**`        | `LET y = x**2 + 3*z`     |
| 比较运算 | `=`, `<>`, `<`, `>`, `<=`, `>=` | `IF count > max THEN`    |
| 逻辑运算 | `AND`, `OR`, `NOT`              | `IF (x>0) AND (y<10)`    |
| 集合运算 | `IN`, `FIRST.IN`, `LAST.IN`     | `LET c = FIRST.IN Queue` |

### 四、控制流语句
1. **条件语句**：
   ```simscript
   IF condition THEN
     statements
   ELSEIF condition THEN
     statements
   ELSE
     statements
   ALWAYS '' 结束块
   ```

2. **循环语句**：
   - 计数循环：
     ```simscript
     FOR i = start TO end [STEP step] DO
       statements
     LOOP
     ```
   - 条件循环：
     ```simscript
     WHILE condition DO
       statements
     LOOP
     ```
   - 集合遍历：
     ```simscript
     FOR EACH customer IN WaitingQueue DO
       WRITE customer.id TO SCREEN
     LOOP
     ```

### 五、函数与进程
1. **函数定义**：
   ```simscript
   ROUTINE CalculateTotal(items) = REAL
     LET total = 0.0
     FOR EACH item IN items DO
       ADD item.price TO total
     LOOP
     RETURN total
   END
   ```

2. **进程控制**：
   ```simscript
   PROCESS CustomerProcess
     CREATE Customer WITH arrival_time = TIME.V
     FILE Customer INTO WaitingQueue
     WAIT UNTIL resource IS AVAILABLE
     USE resource FOR service_time
     DESTROY Customer
   END
   ```

### 六、事件调度
```simscript
EVENT_HANDLER Arrival(customer)
  '' 事件处理逻辑
  SCHEDULE Departure AT TIME.V + service_time
END

'' 调度事件
SCHEDULE Arrival AT 0.0 WITH customer = new_customer
```

### 七、输入输出操作
1. **文件操作**：
   ```simscript
   OPEN "data.txt" AS FILE 1 FOR READING
   READ value FROM FILE 1
   CLOSE FILE 1
   ```

2. **控制台输出**：
   ```simscript
   WRITE "Simulation time: ", TIME.V TO SCREEN
   WRITE "Results" TO FILE "output.csv"
   ```

### 八、时间管理
```simscript
LET current_time = TIME.V '' 获取当前仿真时间
ADVANCE TIME BY delta_t '' 推进仿真时间
```

### 九、资源管理
```simscript
RESOURCE Server
  CAPACITY: 3
END

'' 使用资源
ACQUIRE 1 UNIT OF Server
USE Server FOR service_time
RELEASE Server
```

### 十、错误处理
```simscript
MONITOR
  statements '' 可能出错的代码
WHEN ERROR
  WRITE "Error code: ", ERROR.CODE TO SCREEN
END
```

### 十一、统计收集
```simscript
DEFINE QueueStats AS TALLY
  FOR WaitingQueue
  REPORT MEAN, MAX
END

'' 记录统计
OBSERVE QueueStats WITH LENGTH(WaitingQueue)
```

### 语法约束规则：
1. **作用域规则**：
   - 变量默认局部于当前例程/进程
   - 全局变量需在 `PREAMBLE` 中声明

2. **类型强制**：
   - 禁止隐式类型转换
   - 集合操作需类型匹配

3. **事件限制**：
   - 事件处理中不能使用 `DELAY`
   - 进程中可以调度事件

4. **命名规则**：
   - 大小写不敏感
   - 不能使用保留字（SCHEDULE, EVENT, PROCESS等）

5. **仿真控制**：
   - 必须在 `START SIMULATION` 块内使用时间推进
   - 资源操作必须在进程上下文中

### 特殊语法元素：
1. **系统变量**：
   - `TIME.V`：当前仿真时间
   - `EVENT.CAUSE`：触发当前事件的原因
   - `RESOURCE.IDLE`：空闲资源数量

2. **快捷操作**：
   ```simscript
   INCREMENT counter BY 1
   DEQUEUE customer FROM WaitingQueue
   ```

3. **并行语法扩展**：
   ```simscript
   PARALLEL FOR i = 1 TO n DO
     statements
   SYNC '' 同步点
   ```

此语法规则集覆盖 SIMSCRIPT III 的核心规范，实际实现可能因编译器版本有所不同。
