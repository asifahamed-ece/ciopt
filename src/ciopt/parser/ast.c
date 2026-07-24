#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *_type_strings[] = {
    "program", "function_def", "function_decl", "variable_decl", "struct_decl",
    "typedef",
    "block", "if", "switch", "case", "for", "while", "do_while",
    "break", "continue", "return", "goto", "label", "expr_stmt",
    "binary_op", "unary_op", "call", "identifier",
    "int_literal", "float_literal", "string_literal", "char_literal",
    "subscript", "member_access", "assignment", "ternary",
    "sizeof", "cast", "comma",
    "array_decl", "pointer_decl", "init_list", "param_list",
    "null"
};

CioptNode *ciopt_node_create(CioptNodeType type, int lineno)
{
    CioptNode *node = (CioptNode *)calloc(1, sizeof(CioptNode));
    if (!node) return NULL;

    node->type = type;
    node->lineno = lineno;
    node->end_lineno = lineno;
    node->col_offset = 0;
    return node;
}

int ciopt_node_list_add(CioptNodeList *list, CioptNode *node)
{
    if (!list || !node) return -1;

    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2 : 8;
        CioptNode **new_nodes = (CioptNode **)realloc(list->nodes, new_cap * sizeof(CioptNode *));
        if (!new_nodes) return -1;
        list->nodes = new_nodes;
        list->capacity = new_cap;
    }

    list->nodes[list->count++] = node;
    return 0;
}

static void _free_string_fields(CioptNode *node)
{
    switch (node->type) {
        case CIOPT_NODE_FUNCTION_DEF:
            free(node->data.func_def.name);
            break;
        case CIOPT_NODE_FUNCTION_DECL:
            free(node->data.func_decl.name);
            break;
        case CIOPT_NODE_VARIABLE_DECL:
            free(node->data.var_decl.name);
            break;
        case CIOPT_NODE_STRUCT_DECL:
            free(node->data.struct_decl.name);
            break;
        case CIOPT_NODE_GOTO:
            free(node->data.goto_stmt.name);
            break;
        case CIOPT_NODE_LABEL:
            free(node->data.label.name);
            break;
        case CIOPT_NODE_IDENTIFIER:
            free(node->data.identifier.name);
            break;
        case CIOPT_NODE_STRING_LITERAL:
            free(node->data.string_literal.value);
            break;
        case CIOPT_NODE_MEMBER_ACCESS:
            free(node->data.member_access.member);
            break;
        default:
            break;
    }
}

static void _free_node_list(CioptNodeList *list)
{
    if (!list) return;
    for (size_t i = 0; i < list->count; i++)
        ciopt_node_free(list->nodes[i]);
    free(list->nodes);
}

void ciopt_node_free(CioptNode *node)
{
    if (!node) return;

    /* Recursively free children based on node type */
    switch (node->type) {
        case CIOPT_NODE_FUNCTION_DEF:
            ciopt_node_free(node->data.func_def.body);
            ciopt_node_free(node->data.func_def.params);
            ciopt_node_free(node->data.func_def.return_type);
            break;
        case CIOPT_NODE_FUNCTION_DECL:
            ciopt_node_free(node->data.func_decl.body);
            ciopt_node_free(node->data.func_decl.params);
            break;
        case CIOPT_NODE_VARIABLE_DECL:
            ciopt_node_free(node->data.var_decl.type);
            ciopt_node_free(node->data.var_decl.init);
            break;
        case CIOPT_NODE_STRUCT_DECL:
            _free_node_list(&node->data.struct_decl.members);
            break;
        case CIOPT_NODE_BLOCK:
            _free_node_list(&node->data.block.stmts);
            break;
        case CIOPT_NODE_IF:
            ciopt_node_free(node->data.if_stmt.condition);
            ciopt_node_free(node->data.if_stmt.then_body);
            ciopt_node_free(node->data.if_stmt.else_body);
            break;
        case CIOPT_NODE_SWITCH:
            ciopt_node_free(node->data.switch_stmt.value);
            ciopt_node_free(node->data.switch_stmt.body);
            break;
        case CIOPT_NODE_CASE:
            ciopt_node_free(node->data.case_stmt.value);
            ciopt_node_free(node->data.case_stmt.body);
            break;
        case CIOPT_NODE_FOR:
            ciopt_node_free(node->data.for_loop.init);
            ciopt_node_free(node->data.for_loop.condition);
            ciopt_node_free(node->data.for_loop.update);
            ciopt_node_free(node->data.for_loop.body);
            break;
        case CIOPT_NODE_WHILE:
        case CIOPT_NODE_DO_WHILE:
            ciopt_node_free(node->data.loop.condition);
            ciopt_node_free(node->data.loop.body);
            break;
        case CIOPT_NODE_RETURN:
            ciopt_node_free(node->data.return_stmt.value);
            break;
        case CIOPT_NODE_EXPR_STMT:
            ciopt_node_free(node->data.expr_stmt.expr);
            break;
        case CIOPT_NODE_BINARY_OP:
            ciopt_node_free(node->data.binary_op.left);
            ciopt_node_free(node->data.binary_op.right);
            break;
        case CIOPT_NODE_UNARY_OP:
            ciopt_node_free(node->data.unary_op.operand);
            break;
        case CIOPT_NODE_CALL:
            _free_node_list(&node->data.call.args);
            ciopt_node_free(node->data.call.callee);
            break;
        case CIOPT_NODE_SUBSCRIPT:
            ciopt_node_free(node->data.subscript.object);
            ciopt_node_free(node->data.subscript.index);
            break;
        case CIOPT_NODE_MEMBER_ACCESS:
            ciopt_node_free(node->data.member_access.object);
            break;
        case CIOPT_NODE_ASSIGNMENT:
            ciopt_node_free(node->data.assignment.target);
            ciopt_node_free(node->data.assignment.value);
            break;
        case CIOPT_NODE_TERNARY:
            ciopt_node_free(node->data.ternary.condition);
            ciopt_node_free(node->data.ternary.then_expr);
            ciopt_node_free(node->data.ternary.else_expr);
            break;
        case CIOPT_NODE_CAST:
            ciopt_node_free(node->data.cast.expr);
            ciopt_node_free(node->data.cast.type);
            break;
        case CIOPT_NODE_COMMA:
            _free_node_list(&node->data.comma.exprs);
            break;
        case CIOPT_NODE_PARAM_LIST:
            _free_node_list(&node->data.block.stmts); /* reuse stmts list */
            break;
        default:
            break;
    }

    _free_string_fields(node);
    free(node);
}

void ciopt_node_list_free(CioptNodeList *list)
{
    if (!list) return;
    free(list->nodes);
    list->nodes = NULL;
    list->count = 0;
    list->capacity = 0;
}

const char *ciopt_node_type_string(CioptNodeType type)
{
    if (type >= 0 && type < CIOPT_NODE_NULL)
        return _type_strings[type];
    return "unknown";
}

bool ciopt_node_is_loop(CioptNodeType type)
{
    return type == CIOPT_NODE_FOR ||
           type == CIOPT_NODE_WHILE ||
           type == CIOPT_NODE_DO_WHILE;
}

bool ciopt_node_is_jump(CioptNodeType type)
{
    return type == CIOPT_NODE_BREAK ||
           type == CIOPT_NODE_CONTINUE ||
           type == CIOPT_NODE_RETURN ||
           type == CIOPT_NODE_GOTO;
}