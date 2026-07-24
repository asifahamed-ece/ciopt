#include "ts_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Tree-sitter API */
#include <tree_sitter/api.h>

/* Tree-sitter C grammar parser (generated) */
extern const TSLanguage *tree_sitter_c(void);

/* Forward declarations for CST walker */
static CioptNode *_walk_ts_node(TSNode node, const char *source);
static CioptNode *_walk_ts_children(TSNode node, const char *source);
static const char *_ts_node_type(TSNode node);

/* Parse C source code into our AST */
CioptNode *ts_parse_c(const char *source, const char *filename)
{
    (void)filename;

    if (!source) return NULL;

    /* Create parser */
    TSParser *parser = ts_parser_new();
    if (!parser) return NULL;

    /* Set the C language */
    ts_parser_set_language(parser, tree_sitter_c());

    /* Parse the source */
    TSTree *tree = ts_parser_parse_string(parser, NULL, source, strlen(source));
    if (!tree) {
        ts_parser_delete(parser);
        return NULL;
    }

    /* Get root node */
    TSNode root = ts_tree_root_node(tree);

    /* Walk the CST and build our AST */
    CioptNode *program = ciopt_node_create(CIOPT_NODE_PROGRAM, 1);
    if (!program) {
        ts_tree_delete(tree);
        ts_parser_delete(parser);
        return NULL;
    }

    /* Walk top-level children */
    uint32_t child_count = ts_node_named_child_count(root);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_named_child(root, i);
        CioptNode *child_node = _walk_ts_node(child, source);
        if (child_node) {
            ciopt_node_list_add(&program->data.block.stmts, child_node);
        }
    }

    /* Cleanup */
    ts_tree_delete(tree);
    ts_parser_delete(parser);

    return program;
}

/* Get the Tree-sitter C language definition */
const void *ts_c_language(void)
{
    return tree_sitter_c();
}

/* =========================================================================
 * CST to AST conversion helpers
 * ========================================================================= */

static const char *_ts_node_type(TSNode node)
{
    return ts_node_type(node);
}

static int _ts_start_line(TSNode node)
{
    TSPoint start = ts_node_start_point(node);
    return (int)start.row + 1; /* Tree-sitter uses 0-based lines */
}

static int _ts_end_line(TSNode node)
{
    TSPoint end = ts_node_end_point(node);
    return (int)end.row + 1;
}

static int _ts_start_col(TSNode node)
{
    TSPoint start = ts_node_start_point(node);
    return (int)start.column;
}

static char *_ts_node_text(TSNode node, const char *source)
{
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    size_t len = (size_t)(end - start);
    char *text = (char *)malloc(len + 1);
    if (!text) return NULL;
    memcpy(text, source + start, len);
    text[len] = '\0';
    return text;
}

/* Walk a single TSNode and convert to CioptNode */
static CioptNode *_walk_ts_node(TSNode node, const char *source)
{
    const char *type = _ts_node_type(node);
    int lineno = _ts_start_line(node);
    int end_lineno = _ts_end_line(node);

    if (strcmp(type, "function_definition") == 0) {
        CioptNode *func = ciopt_node_create(CIOPT_NODE_FUNCTION_DEF, lineno);
        if (!func) return NULL;
        func->end_lineno = end_lineno;

        /* Extract function name from declarator */
        TSNode declarator = ts_node_child_by_field_name(node, "declarator", 10);
        if (!ts_node_is_null(declarator)) {
            /* Walk down to get the function name */
            TSNode name_node = ts_node_child_by_field_name(declarator, "declarator", 10);
            if (!ts_node_is_null(name_node)) {
                /* Check if name_node itself is an identifier */
                if (strcmp(_ts_node_type(name_node), "identifier") == 0) {
                    func->data.func_def.name = _ts_node_text(name_node, source);
                } else {
                    /* Search for identifier among children */
                    uint32_t nchild = ts_node_named_child_count(name_node);
                    for (uint32_t i = 0; i < nchild; i++) {
                        TSNode child = ts_node_named_child(name_node, i);
                        if (strcmp(_ts_node_type(child), "identifier") == 0) {
                            func->data.func_def.name = _ts_node_text(child, source);
                            break;
                        }
                    }
                }
            }
            if (!func->data.func_def.name && strcmp(_ts_node_type(declarator), "identifier") == 0) {
                func->data.func_def.name = _ts_node_text(declarator, source);
            }
            if (!func->data.func_def.name)
                func->data.func_def.name = strdup("<anonymous>");
        } else {
            func->data.func_def.name = strdup("<anonymous>");
        }

        /* Walk body */
        TSNode body = ts_node_child_by_field_name(node, "body", 4);
        if (!ts_node_is_null(body)) {
            func->data.func_def.body = _walk_ts_node(body, source);
        }

        return func;
    }

    if (strcmp(type, "declaration") == 0) {
        /* Could be variable declaration or function declaration */
        uint32_t nchild = ts_node_named_child_count(node);
        for (uint32_t i = 0; i < nchild; i++) {
            TSNode child = ts_node_named_child(node, i);
            const char *child_type = _ts_node_type(child);
            if (strcmp(child_type, "function_declarator") == 0 ||
                strcmp(child_type, "function_declarator") == 0) {
                /* Function declaration (prototype) */
                CioptNode *func = ciopt_node_create(CIOPT_NODE_FUNCTION_DECL, lineno);
                if (!func) return NULL;
                func->end_lineno = end_lineno;

                /* Extract name */
                uint32_t nc = ts_node_named_child_count(child);
                for (uint32_t j = 0; j < nc; j++) {
                    TSNode c = ts_node_named_child(child, j);
                    if (strcmp(_ts_node_type(c), "identifier") == 0) {
                        func->data.func_decl.name = _ts_node_text(c, source);
                        break;
                    }
                }
                if (!func->data.func_decl.name)
                    func->data.func_decl.name = strdup("<unknown>");
                return func;
            }
        }
        /* Skip other declarations for now */
        return NULL;
    }

    if (strcmp(type, "compound_statement") == 0) {
        CioptNode *block = ciopt_node_create(CIOPT_NODE_BLOCK, lineno);
        if (!block) return NULL;
        block->end_lineno = end_lineno;

        uint32_t nchild = ts_node_named_child_count(node);
        for (uint32_t i = 0; i < nchild; i++) {
            TSNode child = ts_node_named_child(node, i);
            CioptNode *child_node = _walk_ts_node(child, source);
            if (child_node) {
                ciopt_node_list_add(&block->data.block.stmts, child_node);
            }
        }
        return block;
    }

    if (strcmp(type, "else_clause") == 0) {
        /* Unwrap else_clause to get the actual statement inside */
        uint32_t nchild = ts_node_named_child_count(node);
        for (uint32_t i = 0; i < nchild; i++) {
            TSNode child = ts_node_named_child(node, i);
            CioptNode *child_node = _walk_ts_node(child, source);
            if (child_node) {
                return child_node;
            }
        }
        return NULL;
    }

    if (strcmp(type, "if_statement") == 0) {
        CioptNode *if_node = ciopt_node_create(CIOPT_NODE_IF, lineno);
        if (!if_node) return NULL;
        if_node->end_lineno = end_lineno;

        /* Condition */
        TSNode cond = ts_node_child_by_field_name(node, "condition", 9);
        if (!ts_node_is_null(cond)) {
            if_node->data.if_stmt.condition = _walk_ts_node(cond, source);
        }

        /* Consequence */
        TSNode consequence = ts_node_child_by_field_name(node, "consequence", 11);
        if (!ts_node_is_null(consequence)) {
            if_node->data.if_stmt.then_body = _walk_ts_node(consequence, source);
        }

        /* Alternative */
        TSNode alternative = ts_node_child_by_field_name(node, "alternative", 11);
        if (!ts_node_is_null(alternative)) {
            if_node->data.if_stmt.else_body = _walk_ts_node(alternative, source);
        }

        return if_node;
    }

    if (strcmp(type, "for_statement") == 0) {
        CioptNode *for_node = ciopt_node_create(CIOPT_NODE_FOR, lineno);
        if (!for_node) return NULL;
        for_node->end_lineno = end_lineno;

        /* Body */
        TSNode body = ts_node_child_by_field_name(node, "body", 4);
        if (!ts_node_is_null(body)) {
            for_node->data.for_loop.body = _walk_ts_node(body, source);
        }

        return for_node;
    }

    if (strcmp(type, "while_statement") == 0) {
        CioptNode *while_node = ciopt_node_create(CIOPT_NODE_WHILE, lineno);
        if (!while_node) return NULL;
        while_node->end_lineno = end_lineno;

        TSNode body = ts_node_child_by_field_name(node, "body", 4);
        if (!ts_node_is_null(body)) {
            while_node->data.loop.body = _walk_ts_node(body, source);
        }

        return while_node;
    }

    if (strcmp(type, "do_statement") == 0) {
        CioptNode *do_node = ciopt_node_create(CIOPT_NODE_DO_WHILE, lineno);
        if (!do_node) return NULL;
        do_node->end_lineno = end_lineno;

        TSNode body = ts_node_child_by_field_name(node, "body", 4);
        if (!ts_node_is_null(body)) {
            do_node->data.loop.body = _walk_ts_node(body, source);
        }

        return do_node;
    }

    if (strcmp(type, "return_statement") == 0) {
        CioptNode *ret = ciopt_node_create(CIOPT_NODE_RETURN, lineno);
        if (!ret) return NULL;
        ret->end_lineno = end_lineno;

        /* Get return value */
        uint32_t nchild = ts_node_named_child_count(node);
        for (uint32_t i = 0; i < nchild; i++) {
            TSNode child = ts_node_named_child(node, i);
            const char *ct = _ts_node_type(child);
            if (strcmp(ct, "identifier") == 0 || strcmp(ct, "call_expression") == 0 ||
                strcmp(ct, "number_literal") == 0 || strcmp(ct, "string_literal") == 0 ||
                strcmp(ct, "binary_expression") == 0) {
                ret->data.return_stmt.value = _walk_ts_node(child, source);
                break;
            }
        }

        return ret;
    }

    if (strcmp(type, "break_statement") == 0) {
        CioptNode *brk = ciopt_node_create(CIOPT_NODE_BREAK, lineno);
        if (brk) brk->end_lineno = end_lineno;
        return brk;
    }

    if (strcmp(type, "continue_statement") == 0) {
        CioptNode *cont = ciopt_node_create(CIOPT_NODE_CONTINUE, lineno);
        if (cont) cont->end_lineno = end_lineno;
        return cont;
    }

    if (strcmp(type, "expression_statement") == 0) {
        CioptNode *expr_stmt = ciopt_node_create(CIOPT_NODE_EXPR_STMT, lineno);
        if (!expr_stmt) return NULL;
        expr_stmt->end_lineno = end_lineno;

        uint32_t nchild = ts_node_named_child_count(node);
        for (uint32_t i = 0; i < nchild; i++) {
            TSNode child = ts_node_named_child(node, i);
            CioptNode *expr = _walk_ts_node(child, source);
            if (expr) {
                expr_stmt->data.expr_stmt.expr = expr;
                break;
            }
        }

        return expr_stmt;
    }

    if (strcmp(type, "call_expression") == 0) {
        CioptNode *call = ciopt_node_create(CIOPT_NODE_CALL, lineno);
        if (!call) return NULL;
        call->end_lineno = end_lineno;

        /* Function name */
        TSNode func = ts_node_child_by_field_name(node, "function", 8);
        if (!ts_node_is_null(func)) {
            call->data.call.name = _ts_node_text(func, source);
        }

        /* Arguments */
        TSNode args = ts_node_child_by_field_name(node, "arguments", 9);
        if (!ts_node_is_null(args)) {
            uint32_t nargs = ts_node_named_child_count(args);
            for (uint32_t i = 0; i < nargs; i++) {
                TSNode arg = ts_node_named_child(args, i);
                CioptNode *arg_node = _walk_ts_node(arg, source);
                if (arg_node) {
                    ciopt_node_list_add(&call->data.call.args, arg_node);
                }
            }
        }

        return call;
    }

    if (strcmp(type, "identifier") == 0) {
        CioptNode *id = ciopt_node_create(CIOPT_NODE_IDENTIFIER, lineno);
        if (!id) return NULL;
        id->end_lineno = end_lineno;
        id->data.identifier.name = _ts_node_text(node, source);
        return id;
    }

    if (strcmp(type, "number_literal") == 0) {
        char *text = _ts_node_text(node, source);
        if (!text) return NULL;

        CioptNode *num = ciopt_node_create(CIOPT_NODE_INT_LITERAL, lineno);
        if (!num) { free(text); return NULL; }
        num->end_lineno = end_lineno;
        num->data.int_literal.value = strtoll(text, NULL, 0);
        free(text);
        return num;
    }

    if (strcmp(type, "string_literal") == 0) {
        CioptNode *str = ciopt_node_create(CIOPT_NODE_STRING_LITERAL, lineno);
        if (!str) return NULL;
        str->end_lineno = end_lineno;
        str->data.string_literal.value = _ts_node_text(node, source);
        return str;
    }

    if (strcmp(type, "binary_expression") == 0) {
        CioptNode *bin = ciopt_node_create(CIOPT_NODE_BINARY_OP, lineno);
        if (!bin) return NULL;
        bin->end_lineno = end_lineno;

        /* Left */
        TSNode left = ts_node_child_by_field_name(node, "left", 4);
        if (!ts_node_is_null(left))
            bin->data.binary_op.left = _walk_ts_node(left, source);

        /* Right */
        TSNode right = ts_node_child_by_field_name(node, "right", 5);
        if (!ts_node_is_null(right))
            bin->data.binary_op.right = _walk_ts_node(right, source);

        /* Operator */
        TSNode op_node = ts_node_child_by_field_name(node, "operator", 8);
        if (!ts_node_is_null(op_node)) {
            bin->data.binary_op.op = _ts_node_text(op_node, source);
        }

        return bin;
    }

    if (strcmp(type, "assignment_expression") == 0) {
        CioptNode *assign = ciopt_node_create(CIOPT_NODE_ASSIGNMENT, lineno);
        if (!assign) return NULL;
        assign->end_lineno = end_lineno;

        TSNode left = ts_node_child_by_field_name(node, "left", 4);
        if (!ts_node_is_null(left))
            assign->data.assignment.target = _walk_ts_node(left, source);

        TSNode right = ts_node_child_by_field_name(node, "right", 5);
        if (!ts_node_is_null(right))
            assign->data.assignment.value = _walk_ts_node(right, source);

        return assign;
    }

    if (strcmp(type, "unary_expression") == 0) {
        CioptNode *unary = ciopt_node_create(CIOPT_NODE_UNARY_OP, lineno);
        if (!unary) return NULL;
        unary->end_lineno = end_lineno;

        TSNode operand = ts_node_child_by_field_name(node, "argument", 8);
        if (ts_node_is_null(operand))
            operand = ts_node_named_child(node, 0);

        if (!ts_node_is_null(operand))
            unary->data.unary_op.operand = _walk_ts_node(operand, source);

        return unary;
    }

    if (strcmp(type, "subscript_expression") == 0) {
        CioptNode *sub = ciopt_node_create(CIOPT_NODE_SUBSCRIPT, lineno);
        if (!sub) return NULL;
        sub->end_lineno = end_lineno;

        TSNode obj = ts_node_child_by_field_name(node, "object", 6);
        if (!ts_node_is_null(obj))
            sub->data.subscript.object = _walk_ts_node(obj, source);

        TSNode idx = ts_node_child_by_field_name(node, "index", 5);
        if (!ts_node_is_null(idx))
            sub->data.subscript.index = _walk_ts_node(idx, source);

        return sub;
    }

    if (strcmp(type, "field_expression") == 0) {
        CioptNode *member = ciopt_node_create(CIOPT_NODE_MEMBER_ACCESS, lineno);
        if (!member) return NULL;
        member->end_lineno = end_lineno;

        TSNode obj = ts_node_child_by_field_name(node, "object", 6);
        if (!ts_node_is_null(obj))
            member->data.member_access.object = _walk_ts_node(obj, source);

        TSNode field = ts_node_child_by_field_name(node, "field", 5);
        if (!ts_node_is_null(field))
            member->data.member_access.member = _ts_node_text(field, source);

        member->data.member_access.arrow = false;
        return member;
    }

    if (strcmp(type, "pointer_expression") == 0) {
        CioptNode *unary = ciopt_node_create(CIOPT_NODE_UNARY_OP, lineno);
        if (!unary) return NULL;
        unary->end_lineno = end_lineno;
        unary->data.unary_op.op = "*";
        unary->data.unary_op.prefix = true;

        TSNode arg = ts_node_named_child(node, 0);
        if (!ts_node_is_null(arg))
            unary->data.unary_op.operand = _walk_ts_node(arg, source);

        return unary;
    }

    /* Skip other node types for now */
    return NULL;
}