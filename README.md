# Simple Calculator in C

[![CI](https://github.com/khanhdoan4106/Simple-Calculator-by-C/actions/workflows/ci.yml/badge.svg)](https://github.com/khanhdoan4106/Simple-Calculator-by-C/actions/workflows/ci.yml)

An expression calculator written in C. Type a full expression such as `sqrt(16) + 2 * (3 + 4)^2` and it is parsed with Dijkstra's **Shunting-yard algorithm** and evaluated on a stack. Every math function — square root, exponentials, logarithms, trigonometry — is **implemented from scratch** without `<math.h>`.

**[▶ Live demo](https://khanhdoan4106.github.io/Simple-Calculator-by-C/)** — the same C code compiled to WebAssembly (~12 KB), running in your browser.

## Features

- **Full expressions** with correct precedence and associativity: `+ - * / % ^ !`, unary minus, parentheses
  - `2 ^ 3 ^ 2 = 512` (right-associative), `-2 ^ 2 = -4`, `2 ^ -1 = 0.5`
- **Functions:** `sqrt sin cos tan ln log exp abs` · **constants:** `pi e`
- **Variables:** `r = 5`, then `pi * r ^ 2`; `ans` holds the previous result
- **Precise error reporting** — the error position is marked under the input:
  ```text
  > sqrt(16) + 2 * (3 + 4))
    sqrt(16) + 2 * (3 + 4))
                          ^
    [!] Loi: Thieu hoac thua dau ngoac
  ```
  Detects syntax errors, unbalanced parentheses, unknown names, division/modulo by zero, domain errors (`sqrt(-1)`, `ln(0)`, `tan(pi/2)`), and overflow (`171!`, `10^400`).
- **Calculation history** stored in a linked list, persisted to `~/.calc_history` across sessions
- **Interactive REPL** (`help`, `history`, `vars`, `clear`, `quit`) or one-shot mode: `./calc "2 + 3 * 4"`

## Algorithms

| Component | Technique | Complexity |
|---|---|---|
| Parsing | Tokenizer + Shunting-yard → Reverse Polish Notation, evaluated with a stack | O(n) |
| Integer powers | Exponentiation by squaring | O(log n) multiplications |
| `sqrt(x)` | Newton–Raphson, initial guess from halving the IEEE-754 exponent | ~5 iterations |
| `e^x` | Range reduction `x = k·ln2 + r`, Taylor series on `r`, scale by `2^k` via exponent bits | |
| `ln(x)` | Split `x = m·2^e`, then `ln m = 2·atanh((m−1)/(m+1))` series | |
| `sin`, `cos` | Reduce to `[−π/4, π/4]` + quadrant, Taylor series | |
| Real powers | `a^b = e^(b·ln a)` | |
| History | Singly linked list with tail pointer, O(1) append and O(1) eviction of the oldest entry | |

Accuracy is verified against the C standard library in the test suite: relative error ≤ 1e-15 for `exp`, `ln`, `sqrt`, `sin`, `cos` and ≤ 1e-13 for real powers.

## Project Structure

```text
Simple-Calculator-by-C/
├── src/
│   ├── calc.h / calc.c       # Tokenizer, Shunting-yard, RPN evaluation, variables
│   ├── mathx.h / mathx.c     # Math functions without <math.h>
│   ├── history.h / history.c # Linked-list history + file persistence
│   ├── main.c                # REPL and one-shot command-line mode
│   └── wasm_api.c            # Bridge exported to JavaScript (WebAssembly build only)
├── tests/
│   └── test_calc.c           # Unit tests (math vs libm, parser, errors, limits, history)
├── docs/
│   ├── index.html            # Web demo (GitHub Pages)
│   └── calc.wasm             # Built with `make wasm`
├── .github/workflows/ci.yml  # Build + tests on gcc/clang/macOS, sanitizers, wasm
└── Makefile
```

`calc.c` and `mathx.c` are *freestanding*: they use no C standard library at all (no `malloc`, `stdio`, `string.h`, `math.h`), which is what allows them to compile directly to WebAssembly with plain `clang`.

## Getting Started

Requires a C99 compiler (gcc or clang) and `make`.

```bash
make            # build ./calc
./calc          # interactive mode
./calc "2^10"   # one-shot mode -> 1024
make test       # run unit tests
make wasm       # rebuild docs/calc.wasm (requires clang + wasm-ld)
```

On Windows without `make`:

```bash
gcc -std=c99 -O2 -o calc.exe src/calc.c src/mathx.c src/history.c src/main.c
```

To run the web demo locally, serve the `docs/` folder (e.g. `python -m http.server` inside `docs/`) and open `http://localhost:8000` — browsers do not load `.wasm` files from `file://`.

## Example Session

```text
> 2 + 3 * 4
  = 14
> r = 5
  r = 5
> pi * r^2
  = 78.5398163397
> ans / pi
  = 25
> sqrt(16) + 5!
  = 124
> history
    1. 2 + 3 * 4 = 14
    2. r = 5 = 5
    ...
```
