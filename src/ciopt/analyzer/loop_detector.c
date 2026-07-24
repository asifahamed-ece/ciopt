#include "loop_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LoopDetail *loop_detail_create(LoopKind kind, int lineno)
{
    LoopDetail *loop = (LoopDetail *)calloc(1, sizeof(LoopDetail));
    if (!loop) return NULL;
    loop->kind = kind;
    loop->lineno = lineno;
    loop->end_lineno = lineno;
    loop->depth = 1;
    return loop;
}

int loop_detail_add_child(LoopDetail *parent, LoopDetail *child)
{
    if (!parent || !child) return -1;
    if (parent->children_count >= parent->children_capacity) {
        size_t new_cap = parent->children_capacity ? parent->children_capacity * 2 : 4;
        LoopDetail **new_ch = (LoopDetail **)realloc(parent->children, new_cap * sizeof(LoopDetail *));
        if (!new_ch) return -1;
        parent->children = new_ch;
        parent->children_capacity = new_cap;
    }
    parent->children[parent->children_count++] = child;
    child->parent = parent;
    child->depth = parent->depth + 1;
    return 0;
}

int loop_detail_add_variable(LoopDetail *loop, const char *name, int lineno)
{
    if (!loop || !name) return -1;
    if (loop->variables_count >= loop->variables_capacity) {
        size_t new_cap = loop->variables_capacity ? loop->variables_capacity * 2 : 4;
        LoopVariable *new_v = (LoopVariable *)realloc(loop->variables, new_cap * sizeof(LoopVariable));
        if (!new_v) return -1;
        loop->variables = new_v;
        loop->variables_capacity = new_cap;
    }
    LoopVariable *v = &loop->variables[loop->variables_count++];
    memset(v, 0, sizeof(LoopVariable));
    v->name = strdup(name);
    v->lineno = lineno;
    return 0;
}

void loop_detail_free(LoopDetail *loop)
{
    if (!loop) return;
    for (size_t i = 0; i < loop->children_count; i++)
        loop_detail_free(loop->children[i]);
    free(loop->children);
    for (size_t i = 0; i < loop->variables_count; i++) {
        free(loop->variables[i].name);
        free(loop->variables[i].variable_bound);
        free(loop->variables[i].iterable);
    }
    free(loop->variables);
    free(loop->invariant_lines);
    for (size_t i = 0; i < loop->expensive_count; i++)
        free(loop->expensive_operations[i]);
    free(loop->expensive_operations);
    free(loop);
}

LoopAnalysis *loop_analysis_create(const char *function_name)
{
    LoopAnalysis *a = (LoopAnalysis *)calloc(1, sizeof(LoopAnalysis));
    if (!a) return NULL;
    a->function_name = strdup(function_name);
    a->max_depth = 0;
    a->total_loops = 0;
    return a;
}

void loop_analysis_free(LoopAnalysis *analysis)
{
    if (!analysis) return;
    free(analysis->function_name);
    /* Only free top-level loops directly; loop_detail_free handles children recursively */
    for (size_t i = 0; i < analysis->loops_count; i++) {
        if (analysis->loops[i]->depth == 1 || analysis->loops[i]->parent == NULL)
            loop_detail_free(analysis->loops[i]);
    }
    free(analysis->loops);
    free(analysis);
}

static bool _has_halving_expr(CioptNode *node)
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
                 strcmp(bin->data.binary_op.op, ">>") == 0))
                return true;
        }
    }

    if (node->type == CIOPT_NODE_BLOCK) {
        for (size_t i = 0; i < node->data.block.stmts.count; i++) {
            if (_has_halving_expr(node->data.block.stmts.nodes[i])) return true;
        }
    } else if (node->type == CIOPT_NODE_EXPR_STMT) {
        return _has_halving_expr(node->data.expr_stmt.expr);
    } else if (node->type == CIOPT_NODE_IF) {
        if (_has_halving_expr(node->data.if_stmt.then_body)) return true;
        if (_has_halving_expr(node->data.if_stmt.else_body)) return true;
    }

    return false;
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

    CioptNode *body = NULL;
    if (node->type == CIOPT_NODE_WHILE || node->type == CIOPT_NODE_DO_WHILE) body = node->data.loop.body;
    else if (node->type == CIOPT_NODE_FOR) body = node->data.for_loop.body;
    
    return _has_halving_expr(body);
}

bool check_doubling_pattern(CioptNode *node)
{
    if (!node) return false;

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
        if (node->type == CIOPT_NODE_FOR) {
            _walk_node(node->data.for_loop.body, current_stack, stack_depth + 1, analysis);
        } else {
            _walk_node(node->data.loop.body, current_stack, stack_depth + 1, analysis);
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