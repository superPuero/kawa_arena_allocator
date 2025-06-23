# 🏟 kawa::arena\_allocator

![language](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![status](https://img.shields.io/badge/stability-stable-brightgreen)
![license](https://img.shields.io/badge/license-MIT-green.svg)

 **kawa::arena\_allocator** is a **single-header**, *zero-overhead* arena (stack) allocator for modern C++.
It delivers lightning-fast **push / pop** semantics, automatic pointer
alignment, and a handly RAII **scoped** helper – no exceptions, no surprises.

---

## 🔧 Building & Integrating

1. Add `arena_allocator.h` to your include path.
2. Compile with **`-std=c++17`** (or `/std:c++17` on MSVC).

There are no other dependencies.

---

## ✨ Features

* **O(1) push / pop** for trivially predictable performance.
* **Alignment‑aware**: uses `std::align` internally – no UB.
* **`scoped` RAII helper** – automatic rollback of nested allocations.
* **Debug‑friendly**: assertions & platform‑specific `debugbreak()` in `_DEBUG` builds.
* Works with **C++17** and later (GCC / Clang / MSVC).

---

## 🚀 Quick Start

```cpp
#include "arena_allocator.h"

constexpr std::size_t BYTES   = 1024;   // 1 KiB
constexpr std::size_t ENTRIES = 32;     // 32 push ops max

kawa::arena_allocator arena{BYTES, ENTRIES};

auto* i  = arena.push<int>();                    // typed push (default ctor)
auto* v  = arena.push<std::vector<int>>(10, 42); // ctor with (10, 42) as args
void* raw = arena.push(64);                      // untyped push (64 bytes)

arena.pop(); // ← raw block (no dtor)
arena.pop(); // ← vector (dtor is called because vector is not trivially-destructable!)
```

### Scoped guard usage

```cpp
{
    kawa::arena_allocator::scoped scope = arena.scope();

    auto* big = scope.push<double[128]>();
    scope.push(256);          // scratch buffer

    // automatically rolled back when guard goes out of scope (calls destructors too)
} // ← all allocations in scope are popped here
```

---

## 📝 API Overview

| Member                                          | Notes                                                    |
| ----------------------------------------------- | ---------------------------------------------------------|
| `arena_allocator(size_t bytes, size_t entries)` | create arena                                             |
| `T* push<T>(Args&&...)`                         | reserve & in place construction of T with provided args  |
| `void* push(size_t bytes)`                      | raw memory block                                         |
| `void pop()`                                    | Pops the last push (LIFO) and calls destructor if needed |
| `arena_appocator::scoped scope()`               | returns RAII guard for automatic roll-back               |
| `size_t capacity() const`                       | total bytes available                                    |
| `size_t occupied() const`                       | current bytes in use                                     |

> **Note**: `pop()` calls the destructor of non‑trivially destructible types.  
> For raw memory allocations (`push(size)`), no destructor is called.

---

```
 ┌──────────── arena_allocator ────────────┐
 │  _data                                 │
 │ ▼                                      │
 │ [ used ][ free ................. ]     │  ← raw buffer (LIFO)
 │          ▲                             │
 │      _current                          │
 │                                        │
 │  _entries → [16][32][8] …              │  ← stride stack
 └─────────────────────────────────────────┘
```

---

