# Working with hdcpp

Header-only C++ library for hyperdimensional computing (HDC). All types/functions in `hdc` namespace.

## Build & Test Commands

| Command | Description |
|---------|-------------|
| `g++ -std=c++17 -O3 -march=native -DNDEBUG file.cpp -o file` | Compile a test file |
| `cmake -B build && cmake --build build` | CMake configuration/build |
| `conan create .` | Create Conan package |

**No tests exist yet** - the library is not production-ready (per README). Add tests in `examples/` or a new `tests/` folder.

## Code Organization

```
hdcpp/
├── include/hdcpp/
│   └── hypervector.hpp    # All code - static_hypervector and dynamic_hypervector
├── examples/
│   └── simple_example.cpp
├── CMakeLists.txt         # Header-only install target
└── conanfile.py         # Conan packaging (header-only)
```

## Architecture

Two template classes in `hdc` namespace:

- `static_hypervector<Dimension, BlockType>` - compile-time dimension, uses `std::array`
- `dynamic_hypervector<BlockType>` - runtime dimension, uses `std::vector`

Both store bits packed into blocks (default `uint64_t`). Block type can be any unsigned integer for different performance trade-offs.

### Core Operations

| Operation | Method | Free Function |
|-----------|--------|---------------|
| Binding | `bind_inplace(other)` | `bind(a, b)` |
| Bundling | `bundle_inplace(a, b)` (uses *this as 3rd) | `bundle(a, b, c)` (3 vectors) |
| Permutation | `permute_inplace(shift)` | `permute(hv, shift)` |

All `*_inplace` methods return `*this` for chaining.

### Similarity Measures

- `hamming_distance(other)` - count of differing bits
- `dot_product(other)` - bipolar interpretation: 0→-1, 1→+1; range [-dim, dim]

## Gotchas & Non-obvious Patterns

1. **Bit order**: Bit 0 is LSB of first block. `get_bit(0)` returns that LSB. Printed string has leftmost char = bit 0.

2. **Bundling requires 3 vectors**: `bundle_inplace(a, b)` computes majority of `*this`, `a`, and `b`. The free function `bundle(a, b, c)` bundles three separate vectors. This is typical HD computing bundling.

3. **Bounds checking is debug-only**: `get_bit`/`set_bit` only throw in debug builds (`#ifndef NDEBUG`). Release builds skip checks.

4. **Popcount fallback**: Uses `__builtin_popcountll` for GCC/Clang. For C++20, uses `std::popcount`. MSVC needs custom implementation.

5. **Trailing bits sanitization**: When dimension isn't a multiple of block size, trailing bits in last block are zeroed out.

6. **Static vector operations are constexpr**: Most operations on `static_hypervector` are `constexpr` and `noexcept`.

7. **Dimension mismatch exceptions**: Dynamic vector operations throw `std::invalid_argument` if dimensions don't match.

## Adding New Features

- Follow existing style: `#pragma`-style section dividers, `noexcept` where applicable
- Place all code in `include/hdcpp/` for header-only distribution
- Use `std::array` for static, `std::vector` for dynamic variants
- Add comprehensive bounds checking for dynamic vectors; use debug-only checks for static