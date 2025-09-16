#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* 数据类型枚举 */
typedef enum {
    TYPE_INT,
    TYPE_REAL,
    TYPE_DOUBLE,
    TYPE_TEXT,
    TYPE_ALPHA,
    TYPE_SET,
    TYPE_VOID
} DataType;

/* 二元运算符 */
typedef enum {
    BINOP_ADD,
    BINOP_SUB,
    BINOP_MUL,
    BINOP_DIV,
    BINOP_POW,
    BINOP_EQ,
    BINOP_NE,
    BINOP_LT,
    BINOP_GT,
    BINOP_LE,
    BINOP_GE,
    BINOP_AND,
    BINOP_OR
} BinaryOperator;

/* 一元运算符 */
typedef enum {
    UNOP_NOT,
    UNOP_MINUS
} UnaryOperator;

/* 集合操作类型 */
typedef enum {
    SET_OP_UNION,        /* + */
    SET_OP_INTERSECTION, /* * */
    SET_OP_DIFFERENCE,   /* - */
    SET_OP_CONTAINS,     /* IN */
    SET_OP_ADD_ELEMENT,  /* FILE element IN set */
    SET_OP_REMOVE_ELEMENT /* REMOVE element FROM set */
} SetOperationType;

/* AST 节点类型 */
typedef enum {
    NODE_PROGRAM,
    NODE_STATEMENT_LIST,
    NODE_VARIABLE_DECLARATION,
    NODE_ENTITY_DECLARATION,
    NODE_EVENT_DECLARATION,
    NODE_FUNCTION_DECLARATION,
    NODE_ASSIGNMENT,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_FOR_EACH,
    NODE_RETURN,
    NODE_WRITE,
    NODE_WRITE_TO_FILE,
    NODE_OPEN_FILE,
    NODE_CLOSE_FILE,
    NODE_READ_FROM_FILE,
    NODE_START_SIMULATION,
    NODE_SCHEDULE,
    NODE_ADVANCE_TIME,
    NODE_BINARY_EXPRESSION,
    NODE_UNARY_EXPRESSION,
    NODE_INTEGER_LITERAL,
    NODE_FLOAT_LITERAL,
    NODE_STRING_LITERAL,
    NODE_IDENTIFIER,
    NODE_FUNCTION_CALL,
    NODE_ATTRIBUTE_LIST,
    NODE_ATTRIBUTE,
    NODE_PARAMETER_LIST,
    NODE_PARAMETER,
    NODE_EXPRESSION_LIST,
    NODE_SET_CREATION,
    NODE_SET_OPERATION,
    NODE_CLASS_DECLARATION,
    NODE_METHOD_DECLARATION,
    NODE_OBJECT_CREATION,
    NODE_METHOD_CALL,
    NODE_PARALLEL,
    NODE_PARALLEL_SECTIONS,
    NODE_SECTION_LIST,
    NODE_CRITICAL,
    NODE_BARRIER,
    NODE_MASTER,
    NODE_SINGLE,
    NODE_THREADPRIVATE,
    NODE_STDLIB_FUNCTION_CALL
} NodeType;

/* 前向声明 */
typedef struct ASTNode ASTNode;

/* AST 节点结构 */
struct ASTNode {
    NodeType type;
    int line;
    int column;
    
    union {
        struct {
            ASTNode* preamble;
            ASTNode* main;
        } program;
        
        struct {
            ASTNode** items;
            int count;
            int capacity;
        } statement_list;
        
        struct {
            char* name;
            DataType type;
            ASTNode* initializer;
        } variable_declaration;
        
        struct {
            char* name;
            ASTNode* attributes;
        } entity_declaration;
        
        struct {
            char* name;
            ASTNode* parameters;
        } event_declaration;
        
        struct {
            char* name;
            ASTNode* parameters;
            DataType return_type;
            ASTNode* body;
        } function_declaration;
        
        struct {
            char* target;
            ASTNode* value;
        } assignment;
        
        struct {
            ASTNode* condition;
            ASTNode* then_branch;
            ASTNode* else_branch;
        } if_stmt;
        
        struct {
            ASTNode* condition;
            ASTNode* body;
        } while_stmt;
        
        struct {
            char* variable;
            ASTNode* start;
            ASTNode* end;
            ASTNode* step;
            ASTNode* body;
        } for_stmt;
        
        struct {
            char* variable;
            ASTNode* set;
            ASTNode* body;
        } for_each_stmt;
        
        struct {
            ASTNode* value;
        } return_stmt;
        
        struct {
            ASTNode* expression;
        } write_stmt;
        
        struct {
            ASTNode* expression;
            char* filename;
        } write_to_file_stmt;
        
        struct {
            char* filename;
            int file_id;
        } open_file_stmt;
        
        struct {
            int file_id;
        } close_file_stmt;
        
        struct {
            char* variable;
            int file_id;
        } read_from_file_stmt;
        
        struct {
            /* 开始仿真语句 */
        } start_simulation_stmt;
        
        struct {
            char* event_name;
            ASTNode* time;
            ASTNode* parameters;
        } schedule_stmt;
        
        struct {
            ASTNode* delta_time;
        } advance_time_stmt;
        
        struct {
            ASTNode* left;
            BinaryOperator op;
            ASTNode* right;
        } binary_expression;
        
        struct {
            UnaryOperator op;
            ASTNode* operand;
        } unary_expression;
        
        struct {
            int value;
        } integer_literal;
        
        struct {
            double value;
        } float_literal;
        
        struct {
            char* value;
        } string_literal;
        
        struct {
            char* name;
        } identifier;
        
        struct {
            char* name;
            ASTNode* arguments;
        } function_call;

        struct {
            char* name;
            ASTNode* arguments;
        } stdlib_function_call;
        
        struct {
            ASTNode** items;
            int count;
            int capacity;
        } list;
        
        struct {
            char* name;
            DataType type;
        } attribute;
        
        struct {
            char* name;
            DataType type;
        } parameter;
        
        struct {
            ASTNode* elements;
        } set_creation;
        
        struct {
            SetOperationType op;
            ASTNode* left;
            ASTNode* right;
        } set_operation;
        
        struct {
            char* name;
            char* parent_class;
            ASTNode* members;
        } class_declaration;
        
        struct {
            char* name;
            ASTNode* parameters;
            DataType return_type;
            ASTNode* body;
            int is_override;
        } method_declaration;
        
        struct {
            char* variable_name;
            char* class_name;
            ASTNode* arguments;
        } object_creation;
        
        struct {
            char* object_name;
            char* method_name;
            ASTNode* arguments;
        } method_call;
        
        struct {
            ASTNode* body;
        } parallel_stmt;
        
        struct {
            ASTNode* sections;
        } parallel_sections_stmt;
        
        struct {
            ASTNode** items;
            int count;
            int capacity;
        } section_list;
        
        struct {
            ASTNode* body;
        } critical_stmt;
        
        struct {
            /* 屏障语句，无额外数据 */
        } barrier_stmt;
        
        struct {
            ASTNode* body;
        } master_stmt;
        
        struct {
            ASTNode* body;
        } single_stmt;
        
        struct {
            char* variable_name;
        } threadprivate_stmt;
    } data;
};

/* AST 节点创建函数 */
ASTNode* create_program_node(ASTNode* preamble, ASTNode* main);
ASTNode* create_statement_list_node();
ASTNode* create_variable_declaration_node(char* name, DataType type, ASTNode* initializer);
ASTNode* create_entity_declaration_node(char* name, ASTNode* attributes);
ASTNode* create_event_declaration_node(char* name, ASTNode* parameters);
ASTNode* create_function_declaration_node(char* name, ASTNode* parameters, DataType return_type, ASTNode* body);
ASTNode* create_assignment_node(char* target, ASTNode* value);
ASTNode* create_if_node(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch);
ASTNode* combine_elseif_chain(ASTNode* first_if, ASTNode* second_if);
ASTNode* create_while_node(ASTNode* condition, ASTNode* body);
ASTNode* create_for_node(char* variable, ASTNode* start, ASTNode* end, ASTNode* step, ASTNode* body);
ASTNode* create_for_each_node(char* variable, ASTNode* set, ASTNode* body);
ASTNode* create_return_node(ASTNode* value);
ASTNode* create_write_node(ASTNode* expression);
ASTNode* create_write_to_file_node(ASTNode* expression, char* filename);
ASTNode* create_open_file_node(char* filename, int file_id);
ASTNode* create_close_file_node(int file_id);
ASTNode* create_read_from_file_node(char* variable, int file_id);
ASTNode* create_start_simulation_node();
ASTNode* create_schedule_node(char* event_name, ASTNode* time, ASTNode* parameters);
ASTNode* create_advance_time_node(ASTNode* delta_time);
ASTNode* create_binary_expression_node(ASTNode* left, BinaryOperator op, ASTNode* right);
ASTNode* create_unary_expression_node(UnaryOperator op, ASTNode* operand);
ASTNode* create_integer_literal_node(int value);
ASTNode* create_float_literal_node(double value);
ASTNode* create_string_literal_node(char* value);
ASTNode* create_identifier_node(char* name);
ASTNode* create_function_call_node(char* name, ASTNode* arguments);
ASTNode* create_stdlib_function_call_node(char* name, ASTNode* arguments);
ASTNode* create_attribute_list_node();
ASTNode* create_attribute_node(char* name, DataType type);
ASTNode* create_parameter_list_node();
ASTNode* create_parameter_node(char* name, DataType type);
ASTNode* create_expression_list_node();
ASTNode* create_set_creation_node(ASTNode* elements);
ASTNode* create_set_operation_node(SetOperationType op, ASTNode* left, ASTNode* right);
ASTNode* create_class_declaration_node(char* name, char* parent_class, ASTNode* members);
ASTNode* create_method_declaration_node(char* name, ASTNode* parameters, DataType return_type, ASTNode* body, int is_override);
ASTNode* create_object_creation_node(char* variable_name, char* class_name, ASTNode* arguments);
ASTNode* create_method_call_node(char* object_name, char* method_name, ASTNode* arguments);
ASTNode* create_parallel_node(ASTNode* body);
ASTNode* create_parallel_sections_node(ASTNode* sections);
ASTNode* create_section_list_node();
ASTNode* create_critical_node(ASTNode* body);
ASTNode* create_barrier_node();
ASTNode* create_master_node(ASTNode* body);
ASTNode* create_single_node(ASTNode* body);
ASTNode* create_threadprivate_node(char* variable_name);

/* 列表操作函数 */
void add_statement_to_list(ASTNode* list, ASTNode* statement);
void add_attribute_to_list(ASTNode* list, ASTNode* attribute);
void add_parameter_to_list(ASTNode* list, ASTNode* parameter);
void add_expression_to_list(ASTNode* list, ASTNode* expression);
void copy_statement_list_to_list(ASTNode* dest_list, ASTNode* src_list);

/* AST 访问者接口 */
typedef struct {
    void (*visit_program)(ASTNode* node, void* context);
    void (*visit_statement_list)(ASTNode* node, void* context);
    void (*visit_variable_declaration)(ASTNode* node, void* context);
    void (*visit_entity_declaration)(ASTNode* node, void* context);
    void (*visit_event_declaration)(ASTNode* node, void* context);
    void (*visit_function_declaration)(ASTNode* node, void* context);
    void (*visit_assignment)(ASTNode* node, void* context);
    void (*visit_if)(ASTNode* node, void* context);
    void (*visit_while)(ASTNode* node, void* context);
    void (*visit_for)(ASTNode* node, void* context);
    void (*visit_return)(ASTNode* node, void* context);
    void (*visit_write)(ASTNode* node, void* context);
    void (*visit_binary_expression)(ASTNode* node, void* context);
    void (*visit_unary_expression)(ASTNode* node, void* context);
    void (*visit_integer_literal)(ASTNode* node, void* context);
    void (*visit_float_literal)(ASTNode* node, void* context);
    void (*visit_string_literal)(ASTNode* node, void* context);
    void (*visit_identifier)(ASTNode* node, void* context);
    void (*visit_function_call)(ASTNode* node, void* context);
    void (*visit_set_creation)(ASTNode* node, void* context);
    void (*visit_set_operation)(ASTNode* node, void* context);
    void (*visit_class_declaration)(ASTNode* node, void* context);
    void (*visit_method_declaration)(ASTNode* node, void* context);
    void (*visit_object_creation)(ASTNode* node, void* context);
    void (*visit_method_call)(ASTNode* node, void* context);
} ASTVisitor;

/* AST 遍历函数 */
void ast_visit(ASTNode* node, ASTVisitor* visitor, void* context);

/* 内存管理 */
void free_ast(ASTNode* node);

/* 调试输出 */
void print_ast_tree(ASTNode* node, int indent);

#ifdef __cplusplus
}
#endif
