# ToyForth

ToyForth is a custom, stack-based concatenative programming language and interpreter written entirely in C. It is designed as an educational "toy compiler" project to explore core computer science concepts such as dynamic memory management, parsing, execution contexts, and language design.

## 🎯 Project Goals

*   **Understand Language Architecture:** Build a fully functional interpreter from scratch without relying on external libraries (like Lex/Yacc).
*   **Memory Management:** Implement a robust Reference Counting system (`retain`/`release`) in C to handle dynamic memory allocation safely and prevent memory leaks.
*   **Concatenative Paradigm:** Explore a stack-based execution model where functions (words) implicitly pass data through a global stack.
*   **Clean Design:** Utilize modern C design patterns, such as Tagged Unions for dynamic typing and a unified execution engine for both native C callbacks and user-defined code.

## 💻 Syntax and Features

ToyForth uses postfix notation (Reverse Polish Notation), where operands precede their operators. The language heavily utilizes lists `[ ... ]` to encapsulate code blocks for control flow and function definitions.

### Basic Data Types
*   **Integers:** `10`, `-5`
*   **Strings:** `"hello"`
*   **Booleans:** Handled natively or as integers (`0` is False, non-zero is True)
*   **Lists:** `[ 1 2 + ]` (an array of objects/tokens)

### Standard Library
*   **Math & Logic:** `+`, `-`, `*`, `/`, `%`, `=`, `>`, `<`, `>=`, `<=`
*   **Stack Manipulation:** `dup` (duplicates the top element)
*   **I/O:** `print` (prints the top integer), `sprint` (prints the entire stack state)
*   **Execution:** `exec` (pops a list from the stack and executes its contents)

### Control Flow (IF Statement)
Unlike traditional Forth, ToyForth uses a more functional approach for conditionals. The `if` keyword expects a boolean (or integer) and two lists on the stack.
```text
10 5 > [ "10 is greater" print ] [ "5 is greater" print ] if

```

### User-Defined Functions (def)

Functions are defined by pushing a list of instructions, followed by a string representing the function name, and calling `def`.

```text
[ dup * ] "square" def
5 square sprint  // Output: [25]
```

## ⚙️ Under the Hood (Design Choices)

### 1. Unified Execution Engine

Both the main program and user-defined functions are compiled into `tfobj` Lists. The core engine (`executeList`) iterates through these lists. If it encounters a literal (number, string, list), it pushes it to the stack. If it encounters a Symbol, it looks it up in the Function Table and executes it. This perfectly encapsulates the DRY (Don't Repeat Yourself) principle.

### 2. Tagged Unions for Dynamic Typing

Every element in ToyForth is a `tfobj` (ToyForth Object). To save memory, the underlying data structure uses a C `union` combined with a `type` tag (e.g., `TFOBJ_TYPE_INT`, `TFOBJ_TYPE_LIST`).

### 3. Reference Counting (retain / release)

Memory management is handled manually via Reference Counting.

- When an object is added to a list or the stack, retain() is called.
- When an object is popped and consumed by a native function, release() is called.
- If the reference count reaches 0, deleteObject() safely frees the memory (and recursively frees inner elements if it's a list).

### 4. Immutability in Math Operations

To avoid "Shared State" side effects, native mathematical functions treat objects as immutable. For example, the `+` operator pops two integers, calls `release()` on both, computes the sum, and pushes a brand *new* integer object to the stack. This makes stack manipulation functions like `dup` incredibly lightweight and safe, as they only need to increment the reference count rather than cloning memory.

### 5. Function Table (Native vs. User)

The execution context (`tfctx`) holds a dynamic array of registered functions. Each `FuncEntry` uses a union to store either:

1. A function pointer to native C code (e.g., basicMathFunctions).
2. A pointer to a tfobj List containing user-defined ToyForth code.

## 🚀 How to Build and Run

A simple `Makefile` is used to compile the project.

Bash```
# Compile the executable
make toyforth.exe

# Examples
in the repository there is an example of a factorial function, that works by recursion. Change the number in the program


# Run a ToyForth script
./toyforth.exe my_program.tf

