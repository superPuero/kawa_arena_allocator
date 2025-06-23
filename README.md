# kawa::arena\_allocator

![language](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![status](https://img.shields.io/badge/stability-stable-brightgreen)
![license](https://img.shields.io/badge/license-MIT-green.svg)

A **single-header**, *zero-overhead* arena (stack) allocator for modern C++.
It delivers lightning-fast **push / pop** semantics, automatic pointer
alignment, and an optional RAII **scoped** helper that unwinds all local
allocations when leaving the scope – no exceptions, no surprises.

---

## 🔧 Building & Integrating

1. Add `arena_allocator.h` to your include path.
2. Compile with **`-std=c++17`** (or `/std:c++17` on MSVC).

There are no other dependencies.

---

## ✨ Features

* **O(1) push / pop** for trivially predictable performance.
* **Alignment-aware**: uses `std::align` internally – no UB.
* **`scoped` RAII helper** – automatic rollback of nested allocations.
* **Debug-friendly**: assertions & platform-specific `debugbreak()` in
  `_DEBUG` builds.
* **Header-only** – just drop `arena_allocator.h` into your project.
* Works with **C++17** and later (GCC / Clang / MSVC).

---

## 🚀 Quick Start

```cpp
#include "arena_allocator.h"

constexpr std::size_t BYTES   = 1024;   // 1 KiB
constexpr std::size_t ENTRIES = 32;     // 32 push ops max

kawa::arena_allocator arena{BYTES, ENTRIES};

auto* i  = arena.push<int>();           // typed push (no ctor)
auto* v  = arena.push_and_construct<std::vector<int>>(10, 42);
void* raw = arena.push(64);             // untyped push (64 bytes)

arena.pop(); // ← raw block
arena.pop(); // ← vector (remember to call dtor if non-trivial!)
```

### Scoped usage

```cpp
{
    kawa::arena_allocator::scoped scope{arena};

    auto* big = scope.push<double[128]>();
    scope.push(256);          // scratch buffer

    // automatically rolled back when leaving the block
} // ← all allocations in scope are popped here
```

---

## 📝 API Overview

| Member                                          | Notes                               |
| ----------------------------------------------- | ----------------------------------- |
| `arena_allocator(size_t bytes, size_t entries)` | create arena                        |
| `T* push<T>()`                                  | reserve `sizeof(T)` bytes (no ctor) |
| `T* push_and_construct<T>(Args&&...)`           | reserve & in-place construct        |
| `void* push(size_t bytes)`                      | raw memory block                    |
| `void pop()`                                    | pop last push (LIFO)                |
| `size_t capacity() const`                       | total bytes available               |
| `size_t occupied() const`                       | current bytes in use                |
| **`scoped` helper**                             | see quick start above               |

> **Important**: `pop()` **does not** run destructors. For non-trivial
> types, call the destructor manually before popping.

---

## ⚙️ Design Highlights

* Stores raw buffer + entry stack side-by-side for cache friendliness.
* Records the exact byte stride of every push, ensuring perfect pop.
* Move-constructible – you can transfer an arena without copying memory.
* Alignment gaps are tracked in the stride, so memory is never leaked.

```
 ┌──────────── arena_allocator ────────────┐
 │  _data                                 │
 │ ▼                                      │
 │ [ used ][ free ................. ]     │  ← raw buffer (LIFO)
 │          ▲                             │
 │      _current                          │
 │                                        |
```
