# ToyForth

ToyForth is a minimal, concatenative, turing complete stack-based toy programming language and interpreter written in C. It is designed as an educational project to explore memory management (reference counting), compiler architecture, and the mechanics of stack-based execution.

Unlike traditional Forth, ToyForth heavily relies on lists `[ ... ]` and a purely functional approach to defining structures and user-functions, resembling a mix between Forth and PostScript.

## 🎯 Project Goals

*   **Understand Language Architecture:** Build a fully functional interpreter from scratch without relying on external libraries (like Lex/Yacc).
*   **Memory Management:** Implement a robust Reference Counting system (`retain`/`release`) in C to handle dynamic memory allocation safely and prevent memory leaks.
*   **Concatenative Paradigm:** Explore a stack-based execution model where functions (words) implicitly pass data through a global stack.
*   **Clean Design:** Utilize modern C design patterns, such as Tagged Unions for dynamic typing and a unified execution engine for both native C callbacks and user-defined code.

## 💻 Syntax and Features

ToyForth uses postfix notation (Reverse Polish Notation), where operands precede their operators. The language heavily utilizes lists `[ ... ]` to encapsulate code blocks for control flow and function definitions.

## Features

* **Stack-Based Execution**: All operations pop arguments from a global stack and push their results back onto it.
* **Reference Counting**: Robust memory management under the hood for dynamic objects (strings, lists).
* **Dynamic Typing**: Supports Integers, Booleans, Strings, Lists, and executable Symbols.
* **Custom User Functions**: Define your own operations at runtime using lists.

## Building and Running

You can compile the interpreter using `gcc` or via the provided Makefile:

```bash
make toyforth.exe

```

To run a ToyForth program, pass the source file as an argument:

```bash
./toyforth.exe my_program.tf
```

## Language Syntax & Primitives

ToyForth uses **Postfix (Reverse Polish) Notation**. You push operands onto the stack first, followed by the operator.

### Data Types

- Integers: ```42, -10```
- Strings: ```"Hello World"```
- Booleans: Produced by comparison operators. (Internally True / False), behave same way as integers
- Lists: ```[ 1 2 + ] ```(Used for code blocks and data structures).
- Symbols: +, dup, print (Executable words).

### Basic Math & Comparison

The interpreter supports standard integer arithmetic and comparison.

- Math: +, -, \*, /, %Example:``` 10 5 - ```(Leaves 5 on the stack).
- Comparison: =, >, <, >=, <=Example:``` 10 10 = ```(Leaves True on the stack). Note: string comparison and addition is also suppoted!

### Stack Manipulation & I/O

- dup: Duplicates the top element of the stack.
- print: Pops and prints the top integer object.
- sprint: Prints the entire current state of the stack (useful for debugging).
- exec: Pops a list from the stack and executes its contents immediately.

## Control Flow (if)

The `if` statement in ToyForth expects three elements on the stack in the following order:

1. A boolean condition (or integer).
2. The list to execute if the condition is False.
3. The list to execute if the condition is True (Top of the stack).

**Syntax:**

```plaintext
<condition> [ false_branch ] [ true_branch ] if

```

**Example:**

```plaintext
10 5 > [ "Less" sprint ] [ "Greater" sprint ] if

```

## Iterarion structure
Both ```for``` and ```while``` structure exist in the lenguage

- for structure takes as argument an integer n and a list, which will be iterated n times, so the statement look something like ```<number> [...code...] for ```
**example:**

```plaintext
2 10 [ 2 *] for
```
- While structure is inspired by other lenguage such as postScript, and it need a condition list and a iteration list. The function first execute the condition list, then pop the last element from the stack, and see his logical value, and if it is true the iteration list get executed and so on```[...code...] [...condition...] while```
**example:**

```plaintext
2 [3 *] [dup 100 <] while sprint
```
## Defining User Functions (def)

To keep the parser clean, ToyForth defines functions purely via stack operations instead of special keywords like `:` and `;`. You push a list containing the function's code, followed by a string for its name, and call `def`.

**Syntax:**

```
[ body ] "function_name" def

```
Recursion is also supported: 

**Example: A Recursive Factorial Function**

```plaintext
[ 
  dup 1 = 
  [ dup 1 - factorial ]  # False branch: n * factorial(n-1)
  [ 1 ]                    # True branch: return 1
  if 
*
] "factorial" def
```
You can also find an implementation of this function in the file at factorial.tf

Note: Inline comment are supported with '#'
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



## Architecture Notes

- Memory Safety: The core uses a Tagged Union (tfobj) with a strict reference-counting mechanism (retain / release) to prevent memory leaks during recursive list execution.
- Immutability: Native math operations do not mutate objects in-place; they consume the operands and push newly allocated result objects to maintain a safe global state.


## 🚀 How to Build and Run

A simple `Makefile` is used to compile the project.


# Compile the executable

```bash
make toyforth.exe
```
# Examples
in the repository there is an example of a factorial function (as discussed before), that works by recursion. 


# Run a ToyForth script

```bash
./toyforth.exe my_program.tf
```


