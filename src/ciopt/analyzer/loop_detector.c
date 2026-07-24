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
    for (size_t i = 0; i < analysis->loops_count; i++)
        loop_detail_free(analysis->loops[i]);
    free(analysis->loops);
    free(analysis);
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

        /* Push onto stack and recurse */
        current_stack[stack_depth] = loop;
        if (node->type == CIOPT_NODE_FOR) {
            _walk_node(node->data.for_loop.body, current_stack, stack_depth + 1, analysis);
            _walk_node(node->data.for_loop.init, current_stack, stack_depth + 1, analysis);
            _walk_node(node->data.for_loop.condition, current_stack, stack_depth + 1, analysis);
            _walk_node(node->data.for_loop.update, current_stack, stack_depth + 1, analysis);
        } else {
            _walk_node(node->data.loop.body, current_stack, stack_depth + 1, analysis);
            _walk_node(node->data.loop.condition, current_stack, stack_depth + 1, analysis);
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

    /* Allocate stack for recursion tracking (max 64 depth) */
    LoopDetail *stack[64];
    _walk_node(func_node, stack, 0, analysis);

    return analysis;
}

ComplexityClass estimate_loop_iterations(LoopDetail *loop)
{
    if (!loop) return COMPLEXITY_O_N;

    if (loop->kind == LOOP_WHILE || loop->kind == LOOP_DO_WHILE) {
        if (loop->variables_count > 0) {
            LoopVariable *v = &loop->variables[0];
            if (v->is_halving) return COMPLEXITY_O_LOG_N;
            if (v->is_doubling) return COMPLEXITY_O_LOG_N;
        }
        return COMPLEXITY_O_N;
    }

    /* For loop */
    if (loop->variables_count > 0) {
        LoopVariable *v = &loop->variables[0];
        if (v->has_constant_bound) return COMPLEXITY_O_1;
        if (v->is_halving) return COMPLEXITY_O_LOG_N;
        if (v->is_doubling) return COMPLEXITY_O_LOG_N;
        /* Default: O(n) for variable bound */
        return COMPLEXITY_O_N;
    }

    return COMPLEXITY_O_N;
}

bool check_halving_pattern(CioptNode *node)
{
    if (!node) return false;

    /* Check for patterns like: n /= 2 or n = n / 2 inside the loop body */
    CioptNode *body = NULL;
    if (node->type == CIOPT_NODE_WHILE) body = node->data.loop.body;
    else if (node->type == CIOPT_NODE_FOR) body = node->data.for_loop.body;
    if (!body) return false;

    /* Walk body for halving assignments */
    /* This is a simplified check - looks for n = n / 2 or n /= 2 in the body */
    for (size_t i = 0; i < body->data.block.stmts.count; i++) {
        CioptNode *stmt = body->data.block.stmts.nodes[i];
        if (stmt->type == CIOPT_NODE_EXPR_STMT && stmt->data.expr_stmt.expr) {
            CioptNode *expr = stmt->data.expr_stmt.expr;
            if (expr->type == CIOPT_NODE_ASSIGNMENT) {
                if (expr->data.assignment.op &&
                    strcmp(expr->data.assignment.op, "/=") == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool check_doubling_pattern(CioptNode *node)
{
    if (!node) return false;

    CioptNode *body = NULL;
    if (node->type == CIOPT_NODE_WHILE) body = node->data.loop.body;
    else if (node->type == CIOPT_NODE_FOR) body = node->data.for_loop.body;
    if (!body) return false;

    for (size_t i = 0; i < body->data.block.stmts.count; i++) {
        CioptNode *stmt = body->data.block.stmts.nodes[i];
        if (stmt->type == CIOPT_NODE_EXPR_STMT && stmt->data.expr_stmt.expr) {
            CioptNode *expr = stmt->data.expr_stmt.expr;
            if (expr->type == CIOPT_NODE_ASSIGNMENT) {
                if (expr->data.assignment.op &&
                    strcmp(expr->data.assignment.op, "*=") == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}