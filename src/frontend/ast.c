#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 创建新的 AST 节点 */
static ASTNode* create_node(NodeType type) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "内存分配失败\n");
        exit(1);
    }
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    return node;
}

/* 创建程序节点 */
ASTNode* create_program_node(ASTNode* preamble, ASTNode* main) {
    ASTNode* node = create_node(NODE_PROGRAM);
    node->data.program.preamble = preamble;
    node->data.program.main = main;
    return node;
}

/* 创建语句列表节点 */
ASTNode* create_statement_list_node() {
    ASTNode* node = create_node(NODE_STATEMENT_LIST);
    node->data.statement_list.items = NULL;
    node->data.statement_list.count = 0;
    node->data.statement_list.capacity = 0;
    return node;
}

/* 创建变量声明节点 */
ASTNode* create_variable_declaration_node(char* name, DataType type, ASTNode* initializer) {
    ASTNode* node = create_node(NODE_VARIABLE_DECLARATION);
    node->data.variable_declaration.name = strdup(name);
    node->data.variable_declaration.type = type;
    node->data.variable_declaration.initializer = initializer;
    return node;
}

/* 创建实体声明节点 */
ASTNode* create_entity_declaration_node(char* name, ASTNode* attributes) {
    ASTNode* node = create_node(NODE_ENTITY_DECLARATION);
    node->data.entity_declaration.name = strdup(name);
    node->data.entity_declaration.attributes = attributes;
    return node;
}

/* 创建事件声明节点 */
ASTNode* create_event_declaration_node(char* name, ASTNode* parameters) {
    ASTNode* node = create_node(NODE_EVENT_DECLARATION);
    node->data.event_declaration.name = strdup(name);
    node->data.event_declaration.parameters = parameters;
    return node;
}

/* 创建函数声明节点 */
ASTNode* create_function_declaration_node(char* name, ASTNode* parameters, DataType return_type, ASTNode* body) {
    ASTNode* node = create_node(NODE_FUNCTION_DECLARATION);
    node->data.function_declaration.name = strdup(name);
    node->data.function_declaration.parameters = parameters;
    node->data.function_declaration.return_type = return_type;
    node->data.function_declaration.body = body;
    return node;
}

/* 创建赋值节点 */
ASTNode* create_assignment_node(char* target, ASTNode* value) {
    ASTNode* node = create_node(NODE_ASSIGNMENT);
    node->data.assignment.target = strdup(target);
    node->data.assignment.value = value;
    return node;
}

/* 创建 if 节点 */
ASTNode* create_if_node(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch) {
    ASTNode* node = create_node(NODE_IF);
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_branch = then_branch;
    node->data.if_stmt.else_branch = else_branch;
    return node;
}

/* 组合 elseif 链 */
ASTNode* combine_elseif_chain(ASTNode* first_if, ASTNode* second_if) {
    if (!first_if) return second_if;
    if (!second_if) return first_if;
    
    /* 找到第一个if节点的最后一个else分支 */
    ASTNode* current = first_if;
    while (current->data.if_stmt.else_branch && 
           current->data.if_stmt.else_branch->type == NODE_IF) {
        current = current->data.if_stmt.else_branch;
    }
    
    /* 将second_if连接到链的末尾 */
    current->data.if_stmt.else_branch = second_if;
    return first_if;
}

/* 创建 while 节点 */
ASTNode* create_while_node(ASTNode* condition, ASTNode* body) {
    ASTNode* node = create_node(NODE_WHILE);
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;
    return node;
}

/* 创建 for 节点 */
ASTNode* create_for_node(char* variable, ASTNode* start, ASTNode* end, ASTNode* step, ASTNode* body) {
    ASTNode* node = create_node(NODE_FOR);
    node->data.for_stmt.variable = strdup(variable);
    node->data.for_stmt.start = start;
    node->data.for_stmt.end = end;
    node->data.for_stmt.step = step;
    node->data.for_stmt.body = body;
    return node;
}

/* 创建 for each 节点 */
ASTNode* create_for_each_node(char* variable, ASTNode* set, ASTNode* body) {
    ASTNode* node = create_node(NODE_FOR_EACH);
    node->data.for_each_stmt.variable = strdup(variable);
    node->data.for_each_stmt.set = set;
    node->data.for_each_stmt.body = body;
    return node;
}

/* 创建 return 节点 */
ASTNode* create_return_node(ASTNode* value) {
    ASTNode* node = create_node(NODE_RETURN);
    node->data.return_stmt.value = value;
    return node;
}

/* 创建 write 节点 */
ASTNode* create_write_node(ASTNode* expression) {
    ASTNode* node = create_node(NODE_WRITE);
    node->data.write_stmt.expression = expression;
    return node;
}

/* 创建 write to file 节点 */
ASTNode* create_write_to_file_node(ASTNode* expression, char* filename) {
    ASTNode* node = create_node(NODE_WRITE_TO_FILE);
    node->data.write_to_file_stmt.expression = expression;
    node->data.write_to_file_stmt.filename = strdup(filename);
    return node;
}

/* 创建 open file 节点 */
ASTNode* create_open_file_node(char* filename, int file_id) {
    ASTNode* node = create_node(NODE_OPEN_FILE);
    node->data.open_file_stmt.filename = strdup(filename);
    node->data.open_file_stmt.file_id = file_id;
    return node;
}

/* 创建 close file 节点 */
ASTNode* create_close_file_node(int file_id) {
    ASTNode* node = create_node(NODE_CLOSE_FILE);
    node->data.close_file_stmt.file_id = file_id;
    return node;
}

/* 创建 read from file 节点 */
ASTNode* create_read_from_file_node(char* variable, int file_id) {
    ASTNode* node = create_node(NODE_READ_FROM_FILE);
    node->data.read_from_file_stmt.variable = strdup(variable);
    node->data.read_from_file_stmt.file_id = file_id;
    return node;
}

/* 创建 start simulation 节点 */
ASTNode* create_start_simulation_node() {
    ASTNode* node = create_node(NODE_START_SIMULATION);
    return node;
}

/* 创建 schedule 节点 */
ASTNode* create_schedule_node(char* event_name, ASTNode* time, ASTNode* parameters) {
    ASTNode* node = create_node(NODE_SCHEDULE);
    node->data.schedule_stmt.event_name = strdup(event_name);
    node->data.schedule_stmt.time = time;
    node->data.schedule_stmt.parameters = parameters;
    return node;
}

/* 创建 advance time 节点 */
ASTNode* create_advance_time_node(ASTNode* delta_time) {
    ASTNode* node = create_node(NODE_ADVANCE_TIME);
    node->data.advance_time_stmt.delta_time = delta_time;
    return node;
}

/* 创建二元表达式节点 */
ASTNode* create_binary_expression_node(ASTNode* left, BinaryOperator op, ASTNode* right) {
    ASTNode* node = create_node(NODE_BINARY_EXPRESSION);
    node->data.binary_expression.left = left;
    node->data.binary_expression.op = op;
    node->data.binary_expression.right = right;
    return node;
}

/* 创建一元表达式节点 */
ASTNode* create_unary_expression_node(UnaryOperator op, ASTNode* operand) {
    ASTNode* node = create_node(NODE_UNARY_EXPRESSION);
    node->data.unary_expression.op = op;
    node->data.unary_expression.operand = operand;
    return node;
}

/* 创建整数字面量节点 */
ASTNode* create_integer_literal_node(int value) {
    ASTNode* node = create_node(NODE_INTEGER_LITERAL);
    node->data.integer_literal.value = value;
    return node;
}

/* 创建浮点数字面量节点 */
ASTNode* create_float_literal_node(double value) {
    ASTNode* node = create_node(NODE_FLOAT_LITERAL);
    node->data.float_literal.value = value;
    return node;
}

/* 创建字符串字面量节点 */
ASTNode* create_string_literal_node(char* value) {
    ASTNode* node = create_node(NODE_STRING_LITERAL);
    node->data.string_literal.value = strdup(value);
    return node;
}

/* 创建标识符节点 */
ASTNode* create_identifier_node(char* name) {
    ASTNode* node = create_node(NODE_IDENTIFIER);
    node->data.identifier.name = strdup(name);
    return node;
}

/* 创建函数调用节点 */
ASTNode* create_function_call_node(char* name, ASTNode* arguments) {
    ASTNode* node = create_node(NODE_FUNCTION_CALL);
    node->data.function_call.name = strdup(name);
    node->data.function_call.arguments = arguments;
    return node;
}

/* 创建属性列表节点 */
ASTNode* create_attribute_list_node() {
    ASTNode* node = create_node(NODE_ATTRIBUTE_LIST);
    node->data.list.items = NULL;
    node->data.list.count = 0;
    node->data.list.capacity = 0;
    return node;
}

/* 创建属性节点 */
ASTNode* create_attribute_node(char* name, DataType type) {
    ASTNode* node = create_node(NODE_ATTRIBUTE);
    node->data.attribute.name = strdup(name);
    node->data.attribute.type = type;
    return node;
}

/* 创建参数列表节点 */
ASTNode* create_parameter_list_node() {
    ASTNode* node = create_node(NODE_PARAMETER_LIST);
    node->data.list.items = NULL;
    node->data.list.count = 0;
    node->data.list.capacity = 0;
    return node;
}

/* 创建参数节点 */
ASTNode* create_parameter_node(char* name, DataType type) {
    ASTNode* node = create_node(NODE_PARAMETER);
    node->data.parameter.name = strdup(name);
    node->data.parameter.type = type;
    return node;
}

/* 创建表达式列表节点 */
ASTNode* create_expression_list_node() {
    ASTNode* node = create_node(NODE_EXPRESSION_LIST);
    node->data.list.items = NULL;
    node->data.list.count = 0;
    node->data.list.capacity = 0;
    return node;
}

/* 创建集合创建节点 */
ASTNode* create_set_creation_node(ASTNode* elements) {
    ASTNode* node = create_node(NODE_SET_CREATION);
    node->data.set_creation.elements = elements;
    return node;
}

/* 创建集合操作节点 */
ASTNode* create_set_operation_node(SetOperationType op, ASTNode* left, ASTNode* right) {
    ASTNode* node = create_node(NODE_SET_OPERATION);
    node->data.set_operation.op = op;
    node->data.set_operation.left = left;
    node->data.set_operation.right = right;
    return node;
}

/* 扩展列表容量 */
static void expand_list(ASTNode* list) {
    if (list->data.list.capacity == 0) {
        list->data.list.capacity = 4;
        list->data.list.items = (ASTNode**)malloc(sizeof(ASTNode*) * list->data.list.capacity);
    } else {
        list->data.list.capacity *= 2;
        list->data.list.items = (ASTNode**)realloc(list->data.list.items, 
                                                  sizeof(ASTNode*) * list->data.list.capacity);
    }
    
    if (!list->data.list.items) {
        fprintf(stderr, "内存分配失败\n");
        exit(1);
    }
}

/* 向语句列表添加语句 */
void add_statement_to_list(ASTNode* list, ASTNode* statement) {
    if (list->type != NODE_STATEMENT_LIST && list->type != NODE_SECTION_LIST) return;
    
    if (list->data.list.count >= list->data.list.capacity) {
        expand_list(list);
    }
    
    list->data.list.items[list->data.list.count++] = statement;
}

/* 将statement_list的内容复制到另一个list中 */
void copy_statement_list_to_list(ASTNode* dest_list, ASTNode* src_list) {
    if (!dest_list || !src_list) return;
    if (dest_list->type != NODE_STATEMENT_LIST && dest_list->type != NODE_SECTION_LIST) return;
    if (src_list->type != NODE_STATEMENT_LIST) return;
    
    for (int i = 0; i < src_list->data.list.count; i++) {
        add_statement_to_list(dest_list, src_list->data.list.items[i]);
    }
}

/* 向属性列表添加属性 */
void add_attribute_to_list(ASTNode* list, ASTNode* attribute) {
    if (list->type != NODE_ATTRIBUTE_LIST) return;
    
    if (list->data.list.count >= list->data.list.capacity) {
        expand_list(list);
    }
    
    list->data.list.items[list->data.list.count++] = attribute;
}

/* 向参数列表添加参数 */
void add_parameter_to_list(ASTNode* list, ASTNode* parameter) {
    if (list->type != NODE_PARAMETER_LIST) return;
    
    if (list->data.list.count >= list->data.list.capacity) {
        expand_list(list);
    }
    
    list->data.list.items[list->data.list.count++] = parameter;
}

/* 向表达式列表添加表达式 */
void add_expression_to_list(ASTNode* list, ASTNode* expression) {
    if (list->type != NODE_EXPRESSION_LIST) return;
    
    if (list->data.list.count >= list->data.list.capacity) {
        expand_list(list);
    }
    
    list->data.list.items[list->data.list.count++] = expression;
}

/* AST 遍历函数 */
void ast_visit(ASTNode* node, ASTVisitor* visitor, void* context) {
    if (!node || !visitor) return;
    
    switch (node->type) {
        case NODE_PROGRAM:
            if (visitor->visit_program) visitor->visit_program(node, context);
            break;
        case NODE_STATEMENT_LIST:
            if (visitor->visit_statement_list) visitor->visit_statement_list(node, context);
            break;
        case NODE_VARIABLE_DECLARATION:
            if (visitor->visit_variable_declaration) visitor->visit_variable_declaration(node, context);
            break;
        case NODE_ENTITY_DECLARATION:
            if (visitor->visit_entity_declaration) visitor->visit_entity_declaration(node, context);
            break;
        case NODE_EVENT_DECLARATION:
            if (visitor->visit_event_declaration) visitor->visit_event_declaration(node, context);
            break;
        case NODE_FUNCTION_DECLARATION:
            if (visitor->visit_function_declaration) visitor->visit_function_declaration(node, context);
            break;
        case NODE_ASSIGNMENT:
            if (visitor->visit_assignment) visitor->visit_assignment(node, context);
            break;
        case NODE_IF:
            if (visitor->visit_if) visitor->visit_if(node, context);
            break;
        case NODE_WHILE:
            if (visitor->visit_while) visitor->visit_while(node, context);
            break;
        case NODE_FOR:
            if (visitor->visit_for) visitor->visit_for(node, context);
            break;
        case NODE_RETURN:
            if (visitor->visit_return) visitor->visit_return(node, context);
            break;
        case NODE_WRITE:
            if (visitor->visit_write) visitor->visit_write(node, context);
            break;
        case NODE_BINARY_EXPRESSION:
            if (visitor->visit_binary_expression) visitor->visit_binary_expression(node, context);
            break;
        case NODE_UNARY_EXPRESSION:
            if (visitor->visit_unary_expression) visitor->visit_unary_expression(node, context);
            break;
        case NODE_INTEGER_LITERAL:
            if (visitor->visit_integer_literal) visitor->visit_integer_literal(node, context);
            break;
        case NODE_FLOAT_LITERAL:
            if (visitor->visit_float_literal) visitor->visit_float_literal(node, context);
            break;
        case NODE_STRING_LITERAL:
            if (visitor->visit_string_literal) visitor->visit_string_literal(node, context);
            break;
        case NODE_IDENTIFIER:
            if (visitor->visit_identifier) visitor->visit_identifier(node, context);
            break;
        case NODE_FUNCTION_CALL:
            if (visitor->visit_function_call) visitor->visit_function_call(node, context);
            break;
        case NODE_SET_CREATION:
            if (visitor->visit_set_creation) visitor->visit_set_creation(node, context);
            break;
        case NODE_SET_OPERATION:
            if (visitor->visit_set_operation) visitor->visit_set_operation(node, context);
            break;
        case NODE_CLASS_DECLARATION:
            if (visitor->visit_class_declaration) visitor->visit_class_declaration(node, context);
            break;
        case NODE_METHOD_DECLARATION:
            if (visitor->visit_method_declaration) visitor->visit_method_declaration(node, context);
            break;
        case NODE_OBJECT_CREATION:
            if (visitor->visit_object_creation) visitor->visit_object_creation(node, context);
            break;
        case NODE_METHOD_CALL:
            if (visitor->visit_method_call) visitor->visit_method_call(node, context);
            break;
        default:
            break;
    }
}

/* 释放 AST 内存 */
void free_ast(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_PROGRAM:
            free_ast(node->data.program.preamble);
            free_ast(node->data.program.main);
            break;
        case NODE_STATEMENT_LIST:
        case NODE_ATTRIBUTE_LIST:
        case NODE_PARAMETER_LIST:
        case NODE_EXPRESSION_LIST:
            for (int i = 0; i < node->data.list.count; i++) {
                free_ast(node->data.list.items[i]);
            }
            free(node->data.list.items);
            break;
        case NODE_VARIABLE_DECLARATION:
            free(node->data.variable_declaration.name);
            free_ast(node->data.variable_declaration.initializer);
            break;
        case NODE_ENTITY_DECLARATION:
            free(node->data.entity_declaration.name);
            free_ast(node->data.entity_declaration.attributes);
            break;
        case NODE_EVENT_DECLARATION:
            free(node->data.event_declaration.name);
            free_ast(node->data.event_declaration.parameters);
            break;
        case NODE_FUNCTION_DECLARATION:
            free(node->data.function_declaration.name);
            free_ast(node->data.function_declaration.parameters);
            free_ast(node->data.function_declaration.body);
            break;
        case NODE_ASSIGNMENT:
            free(node->data.assignment.target);
            free_ast(node->data.assignment.value);
            break;
        case NODE_IF:
            free_ast(node->data.if_stmt.condition);
            free_ast(node->data.if_stmt.then_branch);
            free_ast(node->data.if_stmt.else_branch);
            break;
        case NODE_WHILE:
            free_ast(node->data.while_stmt.condition);
            free_ast(node->data.while_stmt.body);
            break;
        case NODE_FOR:
            free(node->data.for_stmt.variable);
            free_ast(node->data.for_stmt.start);
            free_ast(node->data.for_stmt.end);
            free_ast(node->data.for_stmt.step);
            free_ast(node->data.for_stmt.body);
            break;
        case NODE_RETURN:
            free_ast(node->data.return_stmt.value);
            break;
        case NODE_WRITE:
            free_ast(node->data.write_stmt.expression);
            break;
        case NODE_BINARY_EXPRESSION:
            free_ast(node->data.binary_expression.left);
            free_ast(node->data.binary_expression.right);
            break;
        case NODE_UNARY_EXPRESSION:
            free_ast(node->data.unary_expression.operand);
            break;
        case NODE_STRING_LITERAL:
            free(node->data.string_literal.value);
            break;
        case NODE_IDENTIFIER:
            free(node->data.identifier.name);
            break;
        case NODE_FUNCTION_CALL:
            free(node->data.function_call.name);
            free_ast(node->data.function_call.arguments);
            break;
        case NODE_ATTRIBUTE:
            free(node->data.attribute.name);
            break;
        case NODE_PARAMETER:
            free(node->data.parameter.name);
            break;
        default:
            break;
    }
    
    free(node);
}

/* 打印 AST（调试用） */
void print_ast_tree(ASTNode* node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    
    switch (node->type) {
        case NODE_PROGRAM:
            printf("Program\n");
            if (node->data.program.preamble) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Preamble:\n");
                print_ast_tree(node->data.program.preamble, indent + 2);
            }
            if (node->data.program.main) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Main:\n");
                print_ast_tree(node->data.program.main, indent + 2);
            }
            break;
        case NODE_INTEGER_LITERAL:
            printf("IntegerLiteral: %d\n", node->data.integer_literal.value);
            break;
        case NODE_FLOAT_LITERAL:
            printf("FloatLiteral: %f\n", node->data.float_literal.value);
            break;
        case NODE_STRING_LITERAL:
            printf("StringLiteral: \"%s\"\n", node->data.string_literal.value);
            break;
        case NODE_IDENTIFIER:
            printf("Identifier: %s\n", node->data.identifier.name);
            break;
        case NODE_VARIABLE_DECLARATION:
            printf("VariableDeclaration: %s\n", node->data.variable_declaration.name);
            if (node->data.variable_declaration.initializer) {
                print_ast_tree(node->data.variable_declaration.initializer, indent + 1);
            }
            break;
        case NODE_ASSIGNMENT:
            printf("Assignment: %s\n", node->data.assignment.target);
            print_ast_tree(node->data.assignment.value, indent + 1);
            break;
        case NODE_BINARY_EXPRESSION:
            printf("BinaryExpression\n");
            print_ast_tree(node->data.binary_expression.left, indent + 1);
            print_ast_tree(node->data.binary_expression.right, indent + 1);
            break;
        case NODE_CLASS_DECLARATION:
            printf("ClassDeclaration: %s", node->data.class_declaration.name);
            if (node->data.class_declaration.parent_class) {
                printf(" inherits %s", node->data.class_declaration.parent_class);
            }
            printf("\n");
            if (node->data.class_declaration.members) {
                print_ast_tree(node->data.class_declaration.members, indent + 1);
            }
            break;
        case NODE_METHOD_DECLARATION:
            printf("MethodDeclaration: %s%s\n", 
                   node->data.method_declaration.is_override ? "override " : "",
                   node->data.method_declaration.name);
            if (node->data.method_declaration.parameters) {
                print_ast_tree(node->data.method_declaration.parameters, indent + 1);
            }
            if (node->data.method_declaration.body) {
                print_ast_tree(node->data.method_declaration.body, indent + 1);
            }
            break;
        case NODE_OBJECT_CREATION:
            printf("ObjectCreation: %s = new %s\n", 
                   node->data.object_creation.variable_name,
                   node->data.object_creation.class_name);
            if (node->data.object_creation.arguments) {
                print_ast_tree(node->data.object_creation.arguments, indent + 1);
            }
            break;
        case NODE_METHOD_CALL:
            printf("MethodCall: %s.%s\n", 
                   node->data.method_call.object_name,
                   node->data.method_call.method_name);
            if (node->data.method_call.arguments) {
                print_ast_tree(node->data.method_call.arguments, indent + 1);
            }
            break;
        default:
            printf("Node type: %d\n", node->type);
            break;
    }
}

/* 创建类声明节点 */
ASTNode* create_class_declaration_node(char* name, char* parent_class, ASTNode* members) {
    ASTNode* node = create_node(NODE_CLASS_DECLARATION);
    node->data.class_declaration.name = strdup(name);
    node->data.class_declaration.parent_class = parent_class ? strdup(parent_class) : NULL;
    node->data.class_declaration.members = members;
    return node;
}

/* 创建方法声明节点 */
ASTNode* create_method_declaration_node(char* name, ASTNode* parameters, DataType return_type, ASTNode* body, int is_override) {
    ASTNode* node = create_node(NODE_METHOD_DECLARATION);
    node->data.method_declaration.name = strdup(name);
    node->data.method_declaration.parameters = parameters;
    node->data.method_declaration.return_type = return_type;
    node->data.method_declaration.body = body;
    node->data.method_declaration.is_override = is_override;
    return node;
}

/* 创建对象创建节点 */
ASTNode* create_object_creation_node(char* variable_name, char* class_name, ASTNode* arguments) {
    ASTNode* node = create_node(NODE_OBJECT_CREATION);
    node->data.object_creation.variable_name = strdup(variable_name);
    node->data.object_creation.class_name = strdup(class_name);
    node->data.object_creation.arguments = arguments;
    return node;
}

/* 创建方法调用节点 */
ASTNode* create_method_call_node(char* object_name, char* method_name, ASTNode* arguments) {
    ASTNode* node = create_node(NODE_METHOD_CALL);
    node->data.method_call.object_name = strdup(object_name);
    node->data.method_call.method_name = strdup(method_name);
    node->data.method_call.arguments = arguments;
    return node;
}

/* 创建并行节点 */
ASTNode* create_parallel_node(ASTNode* body) {
    ASTNode* node = create_node(NODE_PARALLEL);
    node->data.parallel_stmt.body = body;
    return node;
}

/* 创建并行段节点 */
ASTNode* create_parallel_sections_node(ASTNode* sections) {
    ASTNode* node = create_node(NODE_PARALLEL_SECTIONS);
    node->data.parallel_sections_stmt.sections = sections;
    return node;
}

/* 创建段列表节点 */
ASTNode* create_section_list_node() {
    ASTNode* node = create_node(NODE_SECTION_LIST);
    node->data.section_list.items = NULL;
    node->data.section_list.count = 0;
    node->data.section_list.capacity = 0;
    return node;
}

/* 创建临界区节点 */
ASTNode* create_critical_node(ASTNode* body) {
    ASTNode* node = create_node(NODE_CRITICAL);
    node->data.critical_stmt.body = body;
    return node;
}

/* 创建屏障节点 */
ASTNode* create_barrier_node() {
    ASTNode* node = create_node(NODE_BARRIER);
    return node;
}

/* 创建主线程节点 */
ASTNode* create_master_node(ASTNode* body) {
    ASTNode* node = create_node(NODE_MASTER);
    node->data.master_stmt.body = body;
    return node;
}

/* 创建单线程节点 */
ASTNode* create_single_node(ASTNode* body) {
    ASTNode* node = create_node(NODE_SINGLE);
    node->data.single_stmt.body = body;
    return node;
}

/* 创建线程私有节点 */
ASTNode* create_threadprivate_node(char* variable_name) {
    ASTNode* node = create_node(NODE_THREADPRIVATE);
    node->data.threadprivate_stmt.variable_name = strdup(variable_name);
    return node;
}
