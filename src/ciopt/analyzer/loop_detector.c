#include "loop_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Known O(N) functions that can make loops O(n^2) when called inside */
static const char *_loop_expensive_o_n[] = {
    "strlen", "strcpy", "strcmp", "strncpy", "strncat",
    "memcpy", "memset", "memmove", "memcmp",
    "strchr", "strrchr", "strstr", "strspn", "strcspn",
    "strpbrk", "strncmp", "strcoll", "strxfrm",
    NULL
};

/* Check if a function call is a known O(N) operation */
static bool _is_expensive_call(const char *func_name)
{
    if (!func_name) return false;
    for (int i = 0; _loop_expensive_o_n[i]; i++) {
        if (strcmp(func_name, _loop_expensive_o_n[i]) == 0)
            return true;
    }
    return false;
}

LoopDetail *loop_detail_create(LoopKind kind, int lineno)
{
    LoopDetail *loop = (LoopDetail *)calloc(1, sizeof(LoopDetail));
    if (!loop) return NULL;

    loop->kind = kind;
    loop->lineno = lineno;
    loop->end_lineno = lineno;
    loop->depth = 1;
    loop->parent = NULL;
    loop->children = NULL;
    loop->children_count = 0;
    loop->children_capacity = 0;
    loop->variables = NULL;
    loop->variables_count = 0;
    loop->variables_capacity = 0;
    loop->contains_break = false;
    loop->contains_continue = false;
    loop->contains_return = false;
    loop->has_invariant_code = false;
    loop->invariant_lines = NULL;
    loop->invariant_count = 0;
    loop->has_expensive_operation = false;
    loop->expensive_operations = NULL;
    loop->expensive_count = 0;
    loop->expensive_capacity = 0;
    loop->node = NULL;

    return loop;
}

void loop_detail_free(LoopDetail *loop)
{
    if (!loop) return;

    /* Free children recursively */
    for (size_t i = 0; i < loop->children_count; i++) {
        loop_detail_free(loop->children[i]);
    }
    free(loop->children);

    /* Free variables */
    for (size_t i = 0; i < loop->variables_count; i++) {
        free(loop->variables[i].name);
        free(loop->variables[i].iterable);
        free(loop->variables[i].variable_bound);
    }
    free(loop->variables);

    /* Free expensive operations */
    for (size_t i = 0; i < loop->expensive_count; i++) {
        free(loop->expensive_operations[i]);
    }
    free(loop->expensive_operations);

    free(loop->invariant_lines);
    free(loop);
}

int loop_detail_add_child(LoopDetail *parent, LoopDetail *child)
{
    if (!parent || !child) return -1;

    if (parent->children_count >= parent->children_capacity) {
        size_t new_cap = parent->children_capacity ? parent->children_capacity * 2 : 4;
        LoopDetail **new_children = (LoopDetail **)realloc(
            parent->children, new_cap * sizeof(LoopDetail *));
        if (!new_children) return -1;
        parent->children = new_children;
        parent->children_capacity = new_cap;
    }

    parent->children[parent->children_count++] = child;
    child->parent = parent;
    child->depth = parent->depth + 1;

    return 0;
}

int loop_detail_add_variable(LoopDetail *loop, const char *name, int lineno)
{
    if (!loop) return -1;

    if (loop->variables_count >= loop->variables_capacity) {
        size_t new_cap = loop->variables_capacity ? loop->variables_capacity * 2 : 4;
        LoopVariable *new_vars = (LoopVariable *)realloc(
            loop->variables, new_cap * sizeof(LoopVariable));
        if (!new_vars) return -1;
        loop->variables = new_vars;
        loop->variables_capacity = new_cap;
    }

    LoopVariable *var = &loop->variables[loop->variables_count++];
    memset(var, 0, sizeof(LoopVariable));
    var->name = name ? strdup(name) : NULL;
    var->lineno = lineno;
    var->is_range = false;
    var->has_constant_bound = false;
    var->has_variable_bound = false;
    var->is_halving = false;
    var->is_doubling = false;

    return 0;
}

LoopAnalysis *loop_analysis_create(const char *function_name)
{
    LoopAnalysis *analysis = (LoopAnalysis *)calloc(1, sizeof(LoopAnalysis));
    if (!analysis) return NULL;

    analysis->function_name = function_name ? strdup(function_name) : NULL;
    analysis->loops = NULL;
    analysis->loops_count = 0;
    analysis->loops_capacity = 0;
    analysis->max_depth = 0;
    analysis->total_loops = 0;

    return analysis;
}

void loop_analysis_free(LoopAnalysis *analysis)
{
    if (!analysis) return;

    for (size_t i = 0; i < analysis->loops_count; i++) {
        if (analysis->loops[i]->parent == NULL) {
            loop_detail_free(analysis->loops[i]);
        }
    }
    free(analysis->loops);
    free(analysis->function_name);
    free(analysis);
}

static bool _is_binary_search_update(CioptNode *node)
{
    /* Check for patterns like: low = mid + 1, high = mid - 1, low = mid, high = mid */
    if (!node || node->type != CIOPT_NODE_ASSIGNMENT) return false;

    CioptNode *target = node->data.assignment.target;
    CioptNode *value = node->data.assignment.value;

    if (!target || !value) return false;

    /* Target should be an identifier (like low, high, left, right) */
    if (target->type != CIOPT_NODE_IDENTIFIER) return false;

    const char *target_name = target->data.identifier.name;
    if (!target_name) return false;

    /* Check if target name suggests a bound variable */
    bool is_bound_var = (strstr(target_name, "low") != NULL ||
                         strstr(target_name, "high") != NULL ||
                         strstr(target_name, "left") != NULL ||
                         strstr(target_name, "right") != NULL ||
                         strstr(target_name, "start") != NULL ||
                         strstr(target_name, "end") != NULL ||
                         strcmp(target_name, "l") == 0 ||
                         strcmp(target_name, "r") == 0 ||
                         strcmp(target_name, "lo") == 0 ||
                         strcmp(target_name, "hi") == 0);

    if (!is_bound_var) return false;

    /* Check if value is based on midpoint identifier or similar */
    if (value->type == CIOPT_NODE_IDENTIFIER) {
        const char *val_name = value->data.identifier.name;
        if (val_name &&
            (strcmp(val_name, "mid") == 0 ||
             strcmp(val_name, "mid1") == 0 ||
             strcmp(val_name, "mid2") == 0 ||
             strcmp(val_name, "m") == 0 ||
             strcmp(val_name, "p") == 0 ||
             strcmp(val_name, "pos") == 0)) {
            return true;
        }
    }

    /* Check for mid + 1, mid - 1 patterns */
    if (value->type == CIOPT_NODE_BINARY_OP) {
        CioptNode *left = value->data.binary_op.left;
        CioptNode *right = value->data.binary_op.right;

        /* Check left side for midpoint variables */
        if (left && left->type == CIOPT_NODE_IDENTIFIER &&
            left->data.identifier.name) {
            const char *left_name = left->data.identifier.name;
            if (strcmp(left_name, "mid") == 0 ||
                strcmp(left_name, "mid1") == 0 ||
                strcmp(left_name, "mid2") == 0 ||
                strcmp(left_name, "m") == 0 ||
                strcmp(left_name, "p") == 0 ||
                strcmp(left_name, "pos") == 0) {
                if (right && right->type == CIOPT_NODE_INT_LITERAL) {
                    return true; /* mid +/- constant */
                }
            }
        }

        /* Check right side for midpoint variables */
        if (right && right->type == CIOPT_NODE_IDENTIFIER &&
            right->data.identifier.name) {
            const char *right_name = right->data.identifier.name;
            if (strcmp(right_name, "mid") == 0 ||
                strcmp(right_name, "mid1") == 0 ||
                strcmp(right_name, "mid2") == 0 ||
                strcmp(right_name, "m") == 0 ||
                strcmp(right_name, "p") == 0 ||
                strcmp(right_name, "pos") == 0) {
                if (left && left->type == CIOPT_NODE_INT_LITERAL) {
                    return true; /* constant +/- mid */
                }
            }
        }
    }

    return false;
}

static bool _has_binary_search_pattern(CioptNode *node)
{
    if (!node) return false;

    /* Look for mid = (low + high) / 2 pattern or mid1/mid2 for ternary search */
    bool has_mid_calc = false;
    bool has_bound_update = false;

    if (node->type == CIOPT_NODE_ASSIGNMENT) {
        CioptNode *target = node->data.assignment.target;
        CioptNode *value = node->data.assignment.value;

        /* Check for mid = (low + high) / 2, mid = (left + right) >> 1,
           or mid1/mid2 = ... / 3 patterns, also m, p, pos as midpoint vars */
        if (target && target->type == CIOPT_NODE_IDENTIFIER &&
            target->data.identifier.name) {
            const char *target_name = target->data.identifier.name;
            bool is_mid_var = (strcmp(target_name, "mid") == 0 ||
                               strcmp(target_name, "mid1") == 0 ||
                               strcmp(target_name, "mid2") == 0 ||
                               strcmp(target_name, "m") == 0 ||
                               strcmp(target_name, "p") == 0 ||
                               strcmp(target_name, "pos") == 0);

            if (is_mid_var && value && value->type == CIOPT_NODE_BINARY_OP) {
                const char *op = value->data.binary_op.op;
                if (op && (strcmp(op, "/") == 0 || strcmp(op, ">>") == 0)) {
                    has_mid_calc = true;
                }
            }
        }

        /* Check for bound updates like low = mid + 1 or high = mid - 1 */
        if (_is_binary_search_update(node)) {
            has_bound_update = true;
        }
    }

    if (node->type == CIOPT_NODE_BLOCK) {
        for (size_t i = 0; i < node->data.block.stmts.count; i++) {
            if (_has_binary_search_pattern(node->data.block.stmts.nodes[i])) {
                return true;
            }
        }
    } else if (node->type == CIOPT_NODE_EXPR_STMT) {
        return _has_binary_search_pattern(node->data.expr_stmt.expr);
    } else if (node->type == CIOPT_NODE_IF) {
        if (_has_binary_search_pattern(node->data.if_stmt.then_body)) return true;
        if (_has_binary_search_pattern(node->data.if_stmt.else_body)) return true;

        /* Also check the condition for comparisons with arr[mid] */
        if (node->data.if_stmt.condition) {
            CioptNode *cond = node->data.if_stmt.condition;
            if (cond->type == CIOPT_NODE_BINARY_OP) {
                /* Check if condition involves array access with mid (subscript expression) */
                CioptNode *left = cond->data.binary_op.left;
                CioptNode *right = cond->data.binary_op.right;

                /* Look for arr[mid], arr[mid1], arr[mid2], arr[m], arr[p], arr[pos] pattern */
                if (left && left->type == CIOPT_NODE_SUBSCRIPT &&
                    left->data.subscript.index &&
                    left->data.subscript.index->type == CIOPT_NODE_IDENTIFIER &&
                    left->data.subscript.index->data.identifier.name) {
                    const char *idx_name = left->data.subscript.index->data.identifier.name;
                    if (strcmp(idx_name, "mid") == 0 ||
                        strcmp(idx_name, "mid1") == 0 ||
                        strcmp(idx_name, "mid2") == 0 ||
                        strcmp(idx_name, "m") == 0 ||
                        strcmp(idx_name, "p") == 0 ||
                        strcmp(idx_name, "pos") == 0) {
                        has_mid_calc = true;
                    }
                }
                if (right && right->type == CIOPT_NODE_SUBSCRIPT &&
                    right->data.subscript.index &&
                    right->data.subscript.index->type == CIOPT_NODE_IDENTIFIER &&
                    right->data.subscript.index->data.identifier.name) {
                    const char *idx_name = right->data.subscript.index->data.identifier.name;
                    if (strcmp(idx_name, "mid") == 0 ||
                        strcmp(idx_name, "mid1") == 0 ||
                        strcmp(idx_name, "mid2") == 0 ||
                        strcmp(idx_name, "m") == 0 ||
                        strcmp(idx_name, "p") == 0 ||
                        strcmp(idx_name, "pos") == 0) {
                        has_mid_calc = true;
                    }
                }
            }
        }

        /* If we have both mid calculation context and bound updates, it's binary search */
        if (has_mid_calc && has_bound_update) return true;
    }

    return has_mid_calc || has_bound_update;
}

static bool _is_variable_halving(CioptNode *assign_node, const char *loop_var_name)
{
    if (!assign_node || !loop_var_name) return false;

    if (assign_node->type != CIOPT_NODE_ASSIGNMENT) return false;

    CioptNode *target = assign_node->data.assignment.target;
    CioptNode *value = assign_node->data.assignment.value;

    if (!target || !value) return false;

    if (target->type != CIOPT_NODE_IDENTIFIER) return false;
    const char *target_name = target->data.identifier.name;
    if (!target_name || strcmp(target_name, loop_var_name) != 0) return false;

    if (value->type == CIOPT_NODE_BINARY_OP) {
        CioptNode *bin = value;
        if (!bin->data.binary_op.op) return false;

        if (strcmp(bin->data.binary_op.op, "/") == 0 ||
            strcmp(bin->data.binary_op.op, ">>") == 0) {
            CioptNode *left = bin->data.binary_op.left;
            CioptNode *right = bin->data.binary_op.right;

            if (left && left->type == CIOPT_NODE_IDENTIFIER) {
                const char *left_name = left->data.identifier.name;
                if (left_name && strcmp(left_name, loop_var_name) == 0) {
                    if (right && right->type == CIOPT_NODE_INT_LITERAL) {
                        int val = right->data.int_literal.value;
                        if (val > 1) return true;
                    }
                }
            }
        }
    }

    return false;
}

static bool _has_halving_expr_in_block(CioptNode *node, const char *loop_var_name)
{
    if (!node) return false;

    if (node->type == CIOPT_NODE_ASSIGNMENT) {
        if (node->data.assignment.op) {
            if (strcmp(node->data.assignment.op, "/=") == 0 ||
                strcmp(node->data.assignment.op, ">>=") == 0)
                return true;
        }
        if (node->data.assignment.value &&
            node->data.assignment.value->type == CIOPT_NODE_BINARY_OP) {
            CioptNode *bin = node->data.assignment.value;
            if (bin->data.binary_op.op &&
                (strcmp(bin->data.binary_op.op, "/") == 0 ||
                 strcmp(bin->data.binary_op.op, ">>") == 0)) {
                if (loop_var_name && _is_variable_halving(node, loop_var_name))
                    return true;
                return true;
            }
        }
    }

    if (node->type == CIOPT_NODE_BLOCK) {
        for (size_t i = 0; i < node->data.block.stmts.count; i++) {
            if (_has_halving_expr_in_block(node->data.block.stmts.nodes[i], loop_var_name)) return true;
        }
    } else if (node->type == CIOPT_NODE_EXPR_STMT) {
        return _has_halving_expr_in_block(node->data.expr_stmt.expr, loop_var_name);
    } else if (node->type == CIOPT_NODE_IF) {
        if (_has_halving_expr_in_block(node->data.if_stmt.then_body, loop_var_name)) return true;
        if (_has_halving_expr_in_block(node->data.if_stmt.else_body, loop_var_name)) return true;
    }

    return false;
}

static bool _has_halving_expr(CioptNode *node)
{
    return _has_halving_expr_in_block(node, NULL);
}

static bool _has_doubling_expr(CioptNode *node)
{
    if (!node) return false;

    if (node->type == CIOPT_NODE_ASSIGNMENT) {
        if (node->data.assignment.op) {
            if (strcmp(node->data.assignment.op, "*=") == 0 ||
                strcmp(node->data.assignment.op, "<<=") == 0)
                return true;
        }
        if (node->data.assignment.value &&
            node->data.assignment.value->type == CIOPT_NODE_BINARY_OP) {
            CioptNode *bin = node->data.assignment.value;
            if (bin->data.binary_op.op &&
                (strcmp(bin->data.binary_op.op, "*") == 0 ||
                 strcmp(bin->data.binary_op.op, "<<") == 0))
                return true;
        }
    }

    if (node->type == CIOPT_NODE_BLOCK) {
        for (size_t i = 0; i < node->data.block.stmts.count; i++) {
            if (_has_doubling_expr(node->data.block.stmts.nodes[i])) return true;
        }
    } else if (node->type == CIOPT_NODE_EXPR_STMT) {
        return _has_doubling_expr(node->data.expr_stmt.expr);
    } else if (node->type == CIOPT_NODE_IF) {
        if (_has_doubling_expr(node->data.if_stmt.then_body)) return true;
        if (_has_doubling_expr(node->data.if_stmt.else_body)) return true;
    }

    return false;
}

bool check_halving_pattern(CioptNode *node)
{
    if (!node) return false;

    if (node->type == CIOPT_NODE_FOR) {
        if (_has_halving_expr(node->data.for_loop.update)) return true;
    }

    CioptNode *body = NULL;
    if (node->type == CIOPT_NODE_WHILE || node->type == CIOPT_NODE_DO_WHILE) body = node->data.loop.body;
    else if (node->type == CIOPT_NODE_FOR) body = node->data.for_loop.body;

    /* First check for direct halving pattern (n /= 2) */
    if (_has_halving_expr(body)) return true;

    /* Then check for binary search pattern (mid calculation + bound updates) */
    if (_has_binary_search_pattern(body)) return true;

    return false;
}

bool check_doubling_pattern(CioptNode *node)
{
    if (!node) return false;

    if (node->type == CIOPT_NODE_FOR) {
        if (_has_doubling_expr(node->data.for_loop.update)) return true;
    }

    CioptNode *body = NULL;
    if (node->type == CIOPT_NODE_WHILE || node->type == CIOPT_NODE_DO_WHILE) body = node->data.loop.body;
    else if (node->type == CIOPT_NODE_FOR) body = node->data.for_loop.body;

    return _has_doubling_expr(body);
}

static void _analyze_loop_variables(LoopDetail *loop, CioptNode *node)
{
    if (!loop || !node) return;

    if (node->type == CIOPT_NODE_FOR) {
        CioptNode *init = node->data.for_loop.init;
        CioptNode *cond = node->data.for_loop.condition;
        CioptNode *upd = node->data.for_loop.update;

        const char *var_name = NULL;
        if (init) {
            if (init->type == CIOPT_NODE_VARIABLE_DECL && init->data.var_decl.name) {
                var_name = init->data.var_decl.name;
            } else if (init->type == CIOPT_NODE_ASSIGNMENT && init->data.assignment.target &&
                       init->data.assignment.target->type == CIOPT_NODE_IDENTIFIER) {
                var_name = init->data.assignment.target->data.identifier.name;
            } else if (init->type == CIOPT_NODE_EXPR_STMT && init->data.expr_stmt.expr) {
                CioptNode *e = init->data.expr_stmt.expr;
                if (e->type == CIOPT_NODE_ASSIGNMENT && e->data.assignment.target &&
                    e->data.assignment.target->type == CIOPT_NODE_IDENTIFIER) {
                    var_name = e->data.assignment.target->data.identifier.name;
                }
            }
        }

        if (var_name) {
            loop_detail_add_variable(loop, var_name, node->lineno);
        } else {
            loop_detail_add_variable(loop, "i", node->lineno);
        }

        LoopVariable *v = &loop->variables[0];

        if (cond && cond->type == CIOPT_NODE_BINARY_OP) {
            CioptNode *right = cond->data.binary_op.right;
            if (right) {
                if (right->type == CIOPT_NODE_INT_LITERAL) {
                    v->has_constant_bound = true;
                    v->constant_bound = right->data.int_literal.value;
                } else if (right->type == CIOPT_NODE_IDENTIFIER) {
                    v->has_variable_bound = true;
                    v->variable_bound = strdup(right->data.identifier.name);
                } else {
                    v->has_variable_bound = true;
                }
            }
        } else {
            v->has_variable_bound = true;
        }

        if (upd) {
            if (upd->type == CIOPT_NODE_ASSIGNMENT && upd->data.assignment.op) {
                if (strcmp(upd->data.assignment.op, "*=") == 0 || strcmp(upd->data.assignment.op, "<<=") == 0)
                    v->is_doubling = true;
                if (strcmp(upd->data.assignment.op, "/=") == 0 || strcmp(upd->data.assignment.op, ">>=") == 0)
                    v->is_halving = true;
            }
        }
        if (check_halving_pattern(node)) v->is_halving = true;
        if (check_doubling_pattern(node)) v->is_doubling = true;

    } else if (node->type == CIOPT_NODE_WHILE || node->type == CIOPT_NODE_DO_WHILE) {
        CioptNode *cond = node->data.loop.condition;
        const char *var_name = "n";
        if (cond && cond->type == CIOPT_NODE_BINARY_OP) {
            if (cond->data.binary_op.left && cond->data.binary_op.left->type == CIOPT_NODE_IDENTIFIER) {
                var_name = cond->data.binary_op.left->data.identifier.name;
            }
        }
        loop_detail_add_variable(loop, var_name, node->lineno);
        LoopVariable *v = &loop->variables[0];
        v->has_variable_bound = true;

        if (check_halving_pattern(node)) v->is_halving = true;
        if (check_doubling_pattern(node)) v->is_doubling = true;
    }
}

/* Walk loop body looking for expensive function calls */
static void _find_expensive_calls_in_loop(CioptNode *node, LoopDetail *loop)
{
    if (!node || !loop) return;

    if (node->type == CIOPT_NODE_CALL && node->data.call.name) {
        if (_is_expensive_call(node->data.call.name)) {
            /* Add to expensive operations list */
            if (loop->expensive_count >= loop->expensive_capacity) {
                size_t new_cap = loop->expensive_capacity ? loop->expensive_capacity * 2 : 4;
                char **new_ops = (char **)realloc(loop->expensive_operations,
                                                   new_cap * sizeof(char *));
                if (new_ops) {
                    loop->expensive_operations = new_ops;
                    loop->expensive_capacity = new_cap;
                } else {
                    return; /* realloc failed */
                }
            }

            if (loop->expensive_count < loop->expensive_capacity) {
                char buf[128];
                snprintf(buf, sizeof(buf), "%s() at line %d",
                         node->data.call.name, node->lineno);
                loop->expensive_operations[loop->expensive_count++] = strdup(buf);
                loop->has_expensive_operation = true;
            }
        }
    }

    /* Recurse into children */
    switch (node->type) {
        case CIOPT_NODE_BLOCK:
            for (size_t i = 0; i < node->data.block.stmts.count; i++)
                _find_expensive_calls_in_loop(node->data.block.stmts.nodes[i], loop);
            break;
        case CIOPT_NODE_IF:
            _find_expensive_calls_in_loop(node->data.if_stmt.then_body, loop);
            _find_expensive_calls_in_loop(node->data.if_stmt.else_body, loop);
            break;
        case CIOPT_NODE_FOR:
            _find_expensive_calls_in_loop(node->data.for_loop.body, loop);
            break;
        case CIOPT_NODE_WHILE:
        case CIOPT_NODE_DO_WHILE:
            _find_expensive_calls_in_loop(node->data.loop.body, loop);
            break;
        case CIOPT_NODE_CALL:
            for (size_t i = 0; i < node->data.call.args.count; i++)
                _find_expensive_calls_in_loop(node->data.call.args.nodes[i], loop);
            break;
        default:
            break;
    }
}

/* Internal visitor walker for loop detection */
static void _walk_node(CioptNode *node, LoopDetail **current_stack,
                       size_t stack_depth, LoopAnalysis *analysis)
{
    if (!node) return;

    if (ciopt_node_is_loop(node->type)) {
        LoopKind kind;
        if (node->type == CIOPT_NODE_FOR) kind = LOOP_FOR;
        else if (node->type == CIOPT_NODE_WHILE) kind = LOOP_WHILE;
        else kind = LOOP_DO_WHILE;

        LoopDetail *loop = loop_detail_create(kind, node->lineno);
        loop->end_lineno = node->end_lineno;
        loop->node = node;

        /* Set depth */
        loop->depth = (int)stack_depth + 1;

        _analyze_loop_variables(loop, node);

        /* Add to analysis flat list */
        if (analysis->loops_count >= analysis->loops_capacity) {
            size_t new_cap = analysis->loops_capacity ? analysis->loops_capacity * 2 : 16;
            LoopDetail **new_l = (LoopDetail **)realloc(analysis->loops, new_cap * sizeof(LoopDetail *));
            if (!new_l) { loop_detail_free(loop); return; }
            analysis->loops = new_l;
            analysis->loops_capacity = new_cap;
        }
        analysis->loops[analysis->loops_count++] = loop;
        analysis->total_loops++;

        if (loop->depth > analysis->max_depth)
            analysis->max_depth = loop->depth;

        /* Link to parent */
        if (stack_depth > 0) {
            LoopDetail *parent = current_stack[stack_depth - 1];
            loop_detail_add_child(parent, loop);
        }

        /* Push onto stack and recurse into body */
        current_stack[stack_depth] = loop;
        CioptNode *body = (node->type == CIOPT_NODE_FOR) ?
                          node->data.for_loop.body : node->data.loop.body;
        if (body) {
            _walk_node(body, current_stack, stack_depth + 1, analysis);
            /* After walking the body, check for expensive calls inside this loop */
            _find_expensive_calls_in_loop(body, loop);
        }
    } else {
        /* Recurse into children */
        switch (node->type) {
            case CIOPT_NODE_FUNCTION_DEF:
                _walk_node(node->data.func_def.body, current_stack, stack_depth, analysis);
                break;
            case CIOPT_NODE_BLOCK:
                for (size_t i = 0; i < node->data.block.stmts.count; i++)
                    _walk_node(node->data.block.stmts.nodes[i], current_stack, stack_depth, analysis);
                break;
            case CIOPT_NODE_IF:
                _walk_node(node->data.if_stmt.then_body, current_stack, stack_depth, analysis);
                _walk_node(node->data.if_stmt.else_body, current_stack, stack_depth, analysis);
                break;
            default:
                break;
        }
    }
}

LoopAnalysis *detect_loops(CioptNode *func_node)
{
    if (!func_node) return NULL;

    LoopAnalysis *analysis = loop_analysis_create(
        func_node->type == CIOPT_NODE_FUNCTION_DEF ? func_node->data.func_def.name : "<anonymous>"
    );
    if (!analysis) return NULL;

    LoopDetail *stack[64];
    _walk_node(func_node, stack, 0, analysis);

    return analysis;
}

ComplexityClass estimate_loop_iterations(LoopDetail *loop)
{
    if (!loop) return COMPLEXITY_O_N;

    if (loop->variables_count > 0) {
        LoopVariable *v = &loop->variables[0];
        if (v->is_halving || v->is_doubling) return COMPLEXITY_O_LOG_N;
        if (v->has_constant_bound) return COMPLEXITY_O_1;
        if (v->has_variable_bound) return COMPLEXITY_O_N;
    }

    if (check_halving_pattern(loop->node) || check_doubling_pattern(loop->node))
        return COMPLEXITY_O_LOG_N;

    return COMPLEXITY_O_N;
}