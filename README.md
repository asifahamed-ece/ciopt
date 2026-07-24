<p align="center">
  <img src="https://img.shields.io/badge/version-0.1.0-blue" alt="Version" />
  <img src="https://img.shields.io/badge/c-11-00599C?logo=c" alt="C Standard" />
  <img src="https://img.shields.io/badge/license-MIT-green" alt="License" />
  <img src="https://img.shields.io/badge/Made%20for-Vibe%20Coders-blueviolet" alt="Made for Vibe Coders" />
</p>

# CiOpt — AI-Powered C Code Complexity & Optimization Engine

> *"Analyze. Detect. Accelerate."*

**Developed by [Asif Ahamed](https://github.com/asif-ahamed)**

CiOpt is a **compiler-inspired static analysis tool** for C that automatically estimates Big-O complexity, detects performance bottlenecks, finds anti-patterns, and suggests optimizations — **all without compiling or running your code**.

```
Source Code → Tree-sitter CST → AST Wrapper → Analysis → Report
```

## Built for Vibe Coders

CiOpt is **purpose-built for the vibe coding workflow**. If you're a developer who uses AI assistants (ChatGPT, Claude, Copilot, Gemini, Cursor, etc.) to write C code, CiOpt is your **quality gate**.

### The Problem

When you vibe-code — letting AI generate large chunks of C code — you move fast, but you can't always tell if the generated code is **performant**. AI models can produce working code that silently contains `O(n²)` loops, unnecessary recursion, or buffer management issues that will **break at scale**.

### The Solution

CiOpt analyzes the AI-generated code and produces a **structured report** that you can feed right back to your AI assistant to fix the issues — creating a **self-improving feedback loop**:

```
You ──prompt──▶ AI writes C code
                   │
                   ▼
            CiOpt analyzes it
                   │
                   ▼
     Report (bottlenecks, anti-patterns, fixes)
                   │
                   ▼
    Feed report back to AI ◀── "Fix these issues"
                   │
                   ▼
           AI improves code
```

## Features

| Feature | Description |
|---------|-------------|
| **Big-O Complexity Detection** | Automatically estimates time complexity for every function — O(1), O(log n), O(n), O(n log n), O(n²), O(n³), O(2ⁿ), and more |
| **Loop Analysis** | Detects for/while/do-while loops, nesting depth, halving patterns (binary search), loop-invariant code |
| **Recursion Detection** | Finds recursive functions, missing base cases, tail-recursion candidates, memoization opportunities |
| **Anti-Pattern Detection** | strcat in loops, manual realloc in loops, missing null checks, unsafe string functions |
| **Dead Code Detection** | Unreachable code after return/break/goto, unused variables, uncalled functions |
| **Data Structure Analysis** | Linked list vs array, repeated linear searches, malloc/free in hot paths |
| **Rich Reports** | Beautiful terminal output (ANSI colors), standalone HTML reports, machine-readable JSON |
| **Vibe-Coder Friendly** | JSON/terminal output designed to be pasted directly into AI assistants for auto-fixing |
| **Zero Compilation Needed** | Pure static analysis — no compilation, no execution, no side effects |
| **CI/CD Ready** | Exit codes and JSON output for automated quality gates in pipelines |

## Requirements

- **C11 compiler** (GCC 5+, Clang 3.6+, MSVC 2015+)
- **Make** (GNU Make, NMake, or CMake)
- **Tree-sitter** (bundled automatically by setup script)

## Quick Start

```bash
# Build CiOpt
make

# Analyze a single C file
./ciopt analyze program.c

# Analyze with verbose explanations
./ciopt analyze program.c -v

# Analyze an entire project
./ciopt analyze src/

# Generate an HTML report
./ciopt analyze src/ --format html -o report.html

# Get JSON output (for CI/CD or AI assistants)
./ciopt analyze program.c --format json
```

## CLI Reference

```
ciopt analyze [OPTIONS] PATH

Options:
  --format, -f    terminal|html|json   Output format (default: terminal)
  --output, -o    FILE                 Output file path (for html/json)
  --verbose, -v                        Show detailed complexity explanations
  --threshold     O(n)|O(n²)|O(n³)    Complexity threshold (default: O(n²))
```

## License

MIT © 2026 Asif Ahamed

<p align="center">
  <b>CiOpt</b> — The C companion to FiOpt. Stop shipping slow code.<br>
  <sub>Built for the vibe coding community</sub>
</p>