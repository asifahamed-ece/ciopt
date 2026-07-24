#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/ciopt/api.h"
#include "../src/ciopt/config.h"
#include "../src/ciopt/parser/ts_parser.h"
#include "../src/ciopt/parser/ast.h"
#include "../src/ciopt/analyzer/complexity_estimator.h"
#include "../src/ciopt/analyzer/loop_detector.h"
#include "../src/ciopt/analyzer/recursion_detector.h"
#include "../src/ciopt/reporting/report.h"
#include "../src/ciopt/reporting/json_reporter.h"
#include "../src/ciopt/reporting/html_reporter.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, expr) do { \
    if (!(expr)) { \
        printf("FAIL: %s\n", name); \
        tests_failed++; \
    } else { \
        printf("PASS: %s\n", name); \
        tests_passed++; \
    } \
} while(0)

static void test_constant_loop(void)
{
    const char *code = 
        "int const_loop(void) {\n"
        "    int sum = 0;\n"
        "    for (int i = 0; i < 10; i++) {\n"
        "        sum += i;\n"
        "    }\n"
        "    return sum;\n"
        "}\n";

    AnalysisConfig *config = config_create_default();
    AnalysisReport *report = ciopt_analyze_source(code, "test_const.c", config);

    TEST("report non-null for constant loop", report != NULL);
    TEST("1 file analyzed", report && report->files_count == 1);
    TEST("1 function analyzed", report && report->files[0]->functions_count == 1);
    
    if (report && report->files_count > 0 && report->files[0]->functions_count > 0) {
        FunctionReport *fr = report->files[0]->functions[0];
        TEST("constant loop complexity is O(1)", 
             fr->complexity && fr->complexity->estimated_complexity == COMPLEXITY_O_1);
    }

    analysis_report_free(report);
    config_free(config);
}

static void test_linear_loop(void)
{
    const char *code = 
        "int sum_arr(int *arr, int n) {\n"
        "    int sum = 0;\n"
        "    for (int i = 0; i < n; i++) {\n"
        "        sum += arr[i];\n"
        "    }\n"
        "    return sum;\n"
        "}\n";

    AnalysisConfig *config = config_create_default();
    AnalysisReport *report = ciopt_analyze_source(code, "test_linear.c", config);

    TEST("linear loop report non-null", report != NULL);
    if (report && report->files_count > 0 && report->files[0]->functions_count > 0) {
        FunctionReport *fr = report->files[0]->functions[0];
        TEST("linear loop complexity is O(n)", 
             fr->complexity && fr->complexity->estimated_complexity == COMPLEXITY_O_N);
    }

    analysis_report_free(report);
    config_free(config);
}

static void test_halving_loop(void)
{
    const char *code = 
        "int binary_search_like(int n) {\n"
        "    int count = 0;\n"
        "    while (n > 1) {\n"
        "        n /= 2;\n"
        "        count++;\n"
        "    }\n"
        "    return count;\n"
        "}\n";

    AnalysisConfig *config = config_create_default();
    AnalysisReport *report = ciopt_analyze_source(code, "test_log.c", config);

    TEST("halving loop report non-null", report != NULL);
    if (report && report->files_count > 0 && report->files[0]->functions_count > 0) {
        FunctionReport *fr = report->files[0]->functions[0];
        TEST("halving loop complexity is O(log n)", 
             fr->complexity && fr->complexity->estimated_complexity == COMPLEXITY_O_LOG_N);
    }

    analysis_report_free(report);
    config_free(config);
}

static void test_nested_loops(void)
{
    const char *code = 
        "void matrix_mult(int n) {\n"
        "    for (int i = 0; i < n; i++) {\n"
        "        for (int j = 0; j < n; j++) {\n"
        "            for (int k = 0; k < n; k++) {\n"
        "                int x = i * j * k;\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n";

    AnalysisConfig *config = config_create_default();
    AnalysisReport *report = ciopt_analyze_source(code, "test_nested.c", config);

    TEST("nested loop report non-null", report != NULL);
    if (report && report->files_count > 0 && report->files[0]->functions_count > 0) {
        FunctionReport *fr = report->files[0]->functions[0];
        TEST("triple nested loop complexity is O(n^3)", 
             fr->complexity && fr->complexity->estimated_complexity == COMPLEXITY_O_N_CUBED);
    }

    analysis_report_free(report);
    config_free(config);
}

static void test_recursion(void)
{
    const char *code = 
        "int fibonacci(int n) {\n"
        "    if (n <= 1) return n;\n"
        "    return fibonacci(n - 1) + fibonacci(n - 2);\n"
        "}\n";

    AnalysisConfig *config = config_create_default();
    AnalysisReport *report = ciopt_analyze_source(code, "test_fibo.c", config);

    TEST("recursion report non-null", report != NULL);
    if (report && report->files_count > 0 && report->files[0]->functions_count > 0) {
        FunctionReport *fr = report->files[0]->functions[0];
        TEST("fibonacci complexity is O(2^n)", 
             fr->complexity && fr->complexity->estimated_complexity == COMPLEXITY_O_2_N);
    }

    analysis_report_free(report);
    config_free(config);
}

static void test_anti_patterns(void)
{
    const char *code = 
        "void bad_code(char *str, int n) {\n"
        "    gets(str);\n"
        "    for (int i = 0; i < n; i++) {\n"
        "        strcat(str, \"a\");\n"
        "    }\n"
        "}\n";

    AnalysisConfig *config = config_create_default();
    config->detect_anti_patterns = true;
    AnalysisReport *report = ciopt_analyze_source(code, "test_patterns.c", config);

    TEST("anti pattern report non-null", report != NULL);
    if (report && report->files_count > 0 && report->files[0]->functions_count > 0) {
        FunctionReport *fr = report->files[0]->functions[0];
        TEST("anti-patterns detected", fr->patterns && fr->patterns->count > 0);
    }

    analysis_report_free(report);
    config_free(config);
}

static void test_reporters(void)
{
    const char *code = 
        "int add(int a, int b) { return a + b; }\n";

    AnalysisConfig *config = config_create_default();
    AnalysisReport *report = ciopt_analyze_source(code, "test_rep.c", config);

    char *json = render_json(report);
    TEST("render_json outputs non-NULL string", json != NULL);
    TEST("render_json contains tool name", json && strstr(json, "CiOpt") != NULL);
    free(json);

    analysis_report_free(report);
    config_free(config);
}

static void test_nlogn_nested_loops(void)
{
    const char *code = 
        "void executeNLogN(int n) {\n"
        "    int operationCount = 0;\n"
        "    for (int i = 0; i < n; i++) {\n"
        "        for (int j = n; j > 0; j = j / 2) {\n"
        "            operationCount++;\n"
        "        }\n"
        "    }\n"
        "}\n";

    AnalysisConfig *config = config_create_default();
    AnalysisReport *report = ciopt_analyze_source(code, "test_nlogn_loops.c", config);

    TEST("nlogn nested loop report non-null", report != NULL);
    if (report && report->files_count > 0 && report->files[0]->functions_count > 0) {
        FunctionReport *fr = report->files[0]->functions[0];
        TEST("nested loop complexity is O(n log n)", 
             fr->complexity && fr->complexity->estimated_complexity == COMPLEXITY_O_N_LOG_N);
    }

    analysis_report_free(report);
    config_free(config);
}

static void test_nlogn_loop_with_call(void)
{
    const char *code = 
        "void loop_with_bsearch(int *arr, int n, int *queries, int q) {\n"
        "    for (int i = 0; i < q; i++) {\n"
        "        bsearch(&queries[i], arr, n, sizeof(int), NULL);\n"
        "    }\n"
        "}\n";

    AnalysisConfig *config = config_create_default();
    AnalysisReport *report = ciopt_analyze_source(code, "test_nlogn_call.c", config);

    TEST("nlogn loop with call report non-null", report != NULL);
    if (report && report->files_count > 0 && report->files[0]->functions_count > 0) {
        FunctionReport *fr = report->files[0]->functions[0];
        TEST("loop with bsearch complexity is O(n log n)", 
             fr->complexity && fr->complexity->estimated_complexity == COMPLEXITY_O_N_LOG_N);
    }

    analysis_report_free(report);
    config_free(config);
}

int main(void)
{
    printf("=== Running CiOpt Comprehensive Analyzer Tests ===\n\n");

    test_constant_loop();
    test_linear_loop();
    test_halving_loop();
    test_nested_loops();
    test_recursion();
    test_anti_patterns();
    test_reporters();
    test_nlogn_nested_loops();
    test_nlogn_loop_with_call();

    printf("\n=== Summary: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}