#ifndef CIOPT_TS_PARSER_H
#define CIOPT_TS_PARSER_H

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * Tree-sitter C parser bridge.
 *
 * Parses C source code using Tree-sitter and converts the CST into
 * our simplified CioptNode AST for analysis.
 *============================================================================*/

/* Parse C source code into our AST.
 * Returns the root CioptNode (CIOPT_NODE_PROGRAM) or NULL on failure.
 * Caller must free with ciopt_node_free(). */
CioptNode *ts_parse_c(const char *source, const char *filename);

/* Get the Tree-sitter C language definition.
 * Used to initialize the parser. */
const void *ts_c_language(void);

#ifdef __cplusplus
}
#endif

#endif /* CIOPT_TS_PARSER_H */