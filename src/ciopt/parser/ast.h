#ifndef CIOPT_AST_H
#define CIOPT_AST_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * AST node types for C code representation.
 *
 * This is a simplified AST that captures the constructs needed for
 * complexity analysis. It is populated by the Tree-sitter CST walker.
 *============================================================================*/

/* AST node types */
typedef enum {
    /* Top-level */
    CIOPT_NODE_PROGRAM,
    CIOPT_NODE_FUNCTION_DEF,
    CIOPT_NODE_FUNCTION_DECL,
    CIOPT_NODE_VARIABLE_DECL,
    CIOPT_NODE_STRUCT_DECL,
    CIOPT_NODE_TYPEDEF,

    /* Statements */
    CIOPT_NODE_BLOCK,
    CIOPT_NODE_IF,
    CIOPT_NODE_SWITCH,
    CIOPT_NODE_CASE,
    CIOPT_NODE_FOR,
    CIOPT_NODE_WHILE,
    CIOPT_NODE_DO_WHILE,
    CIOPT_NODE_BREAK,
    CIOPT_NODE_CONTINUE,
    CIOPT_NODE_RETURN,
    CIOPT_NODE_GOTO,
    CIOPT_NODE_LABEL,
    CIOPT_NODE_EXPR_STMT,

    /* Expressions */
    CIOPT_NODE_BINARY_OP,
    CIOPT_NODE_UNARY_OP,
    CIOPT_NODE_CALL,
    CIOPT_NODE_IDENTIFIER,
    CIOPT_NODE_INT_LITERAL,
    CIOPT_NODE_FLOAT_LITERAL,
    CIOPT_NODE_STRING_LITERAL,
    CIOPT_NODE_CHAR_LITERAL,
    CIOPT_NODE_SUBSCRIPT,
    CIOPT_NODE_MEMBER_ACCESS,
    CIOPT_NODE_ASSIGNMENT,
    CIOPT_NODE_TERNARY,
    CIOPT_NODE_SIZEOF,
    CIOPT_NODE_CAST,
    CIOPT_NODE_COMMA,

    /* Declarations */
    CIOPT_NODE_ARRAY_DECL,
    CIOPT_NODE_POINTER_DECL,
    CIOPT_NODE_INIT_LIST,
    CIOPT_NODE_PARAM_LIST,

    CIOPT_NODE_NULL
} CioptNodeType;

/* Forward declaration */
typedef struct CioptNode CioptNode;

/* Child node list */
typedef struct {
    CioptNode **nodes;
    size_t count;
    size_t capacity;
} CioptNodeList;

/* AST node */
struct CioptNode {
    CioptNodeType type;
    int lineno;
    int end_lineno;
    int col_offset;

    union {
        struct {
            char *name;
            CioptNode *body;
            CioptNode *params;   /* parameter list */
            CioptNode *return_type;
        } func_def;

        struct {
            char *name;
            CioptNode *body;
            CioptNode *params;
        } func_decl;

        struct {
            char *name;
            CioptNode *type;
            CioptNode *init;
        } var_decl;

        struct {
            char *name;
            CioptNodeList members;
        } struct_decl;

        struct {
            CioptNodeList stmts;
        } block;

        struct {
            CioptNode *condition;
            CioptNode *then_body;
            CioptNode *else_body;
        } if_stmt;

        struct {
            CioptNode *condition;
            CioptNode *body;
        } loop;  /* for, while, do-while */

        struct {
            CioptNode *init;
            CioptNode *condition;
            CioptNode *update;
            CioptNode *body;
        } for_loop;

        struct {
            CioptNode *value;
            CioptNode *body;
        } switch_stmt;

        struct {
            CioptNode *value;
            CioptNode *body;
        } case_stmt;

        struct {
            CioptNode *value;
        } return_stmt;

        struct {
            char *name;
        } goto_stmt;

        struct {
            char *name;
        } label;

        struct {
            CioptNode *expr;
        } expr_stmt;

        struct {
            CioptNode *left;
            CioptNode *right;
            const char *op;  /* "+", "-", "*", "/", "%", "==", etc. */
        } binary_op;

        struct {
            CioptNode *operand;
            const char *op;  /* "!", "~", "-", "+", "&", "*", "++", "--" */
            bool prefix;
        } unary_op;

        struct {
            char *name;      /* function name */
            CioptNode *callee; /* for complex calls like ptr->func() */
            CioptNodeList args;
        } call;

        struct {
            char *name;
        } identifier;

        struct {
            long long value;
        } int_literal;

        struct {
            double value;
        } float_literal;

        struct {
            char *value;
        } string_literal;

        struct {
            char value;
        } char_literal;

        struct {
            CioptNode *object;
            CioptNode *index;
        } subscript;

        struct {
            CioptNode *object;
            char *member;
            bool arrow;  /* true for ->, false for . */
        } member_access;

        struct {
            CioptNode *target;
            CioptNode *value;
            const char *op;  /* "=", "+=", "-=", etc. */
        } assignment;

        struct {
            CioptNode *condition;
            CioptNode *then_expr;
            CioptNode *else_expr;
        } ternary;

        struct {
            CioptNode *expr;
            CioptNode *type;
        } cast;

        struct {
            CioptNodeList exprs;
        } comma;
    } data;
};

/* Create a new AST node of given type. Returns NULL on failure. */
CioptNode *ciopt_node_create(CioptNodeType type, int lineno);

/* Add a child to a node list. Returns 0 on success, -1 on failure. */
int ciopt_node_list_add(CioptNodeList *list, CioptNode *node);

/* Free an AST node and all its children recursively. */
void ciopt_node_free(CioptNode *node);

/* Free a node list (does not free the nodes themselves). */
void ciopt_node_list_free(CioptNodeList *list);

/* Get string representation of node type for debugging. */
const char *ciopt_node_type_string(CioptNodeType type);

/* Check if a node is a loop type (for, while, do-while). */
bool ciopt_node_is_loop(CioptNodeType type);

/* Check if a node is a jump statement (break, continue, return, goto). */
bool ciopt_node_is_jump(CioptNodeType type);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_AST_H */