
# HDCPP – Header‑Only Hyperdimensional Computing Library

> **Warning:** This library is a personal side project and **is not production ready**. It lacks comprehensive testing, has limited error handling, and may contain bugs. Use at your own risk.

`hdcpp` is a fast, safe, and portable C++17/20 library for **hyperdimensional computing (HDC)**, also known as vector symbolic architectures (VSA). It provides efficient operations on binary hypervectors of static (compile‑time) or dynamic (runtime) dimension.

## Features

- **Header‑only** – no build, no external dependencies beyond the C++ standard library.
- **Two vector types**:
  - `static_hypervector<Dimension, BlockType>` – dimension fixed at compile time, stored in `std::array`.
  - `dynamic_hypervector<BlockType>` – dimension determined at runtime, stored in `std::vector`.
- **Core HDC operations**:
  - **Binding** (XOR) – `bind()`, `bind_inplace()`
  - **Bundling** (majority vote) – `bundle()`, `bundle_inplace()`
  - **Permutation** (cyclic shift) – `permute()`, `permute_inplace()`
- **Similarity measures**:
  - Hamming distance (`hamming_distance()`)
  - Cosine similarity via dot product (`dot_product()`)
- **Efficient storage**: bit‑packed into blocks of `uint64_t` (configurable to any unsigned integer type).
- **Safety**: bounds checking in debug builds, exception on dimension mismatch (dynamic vectors).
- **Portable popcount**: uses C++20 `std::popcount` or compiler intrinsics.

## Quick Start

### Include the library

```cpp
#include "hdc/hypervector.hpp"
using namespace hdc;
```

### Create hypervectors

```cpp
// Static 10’000‑dimensional vectors (compile‑time)
static_hypervector<10000> a, b, c;

// Fill with random bits (you supply the randomness)
for (size_t i = 0; i < a.dimension; ++i) {
    a.set_bit(i, rand() % 2);
    b.set_bit(i, rand() % 2);
}

// Dynamic vectors with runtime dimension
dynamic_hypervector<> d(5000);   // zero‑initialised
dynamic_hypervector<> e(5000, true); // all‑ones
```

### Basic operations

```cpp
// Binding (XOR)
auto ab = bind(a, b);          // returns new vector
a.bind_inplace(b);             // in‑place modification

// Bundling (majority of three)
auto bundled = bundle(a, b, c);

// Permutation (cyclic shift left by 3)
auto perm = permute(a, 3);

// Similarity
size_t hd = a.hamming_distance(b);
int64_t dot = a.dot_product(b);   // range [-dim, dim]
double cosine = static_cast<double>(dot) / a.dimension;
```

### Example: Recognising similar patterns

```cpp
static_hypervector<1000> prototype, query;
// ... initialise prototype and query ...
if (prototype.hamming_distance(query) < 100) {
    std::cout << "Match!\n";
}
```

## Performance Notes

- All operations are **bit‑parallel** over 64‑bit blocks. Binding (XOR) runs at ≈0.5 cycles per bit on modern CPUs.
- The library never allocates memory in `static_hypervector` operations.
- **No use of `std::vector<bool>`** – no proxy iterator pitfalls.
- For maximum speed, compile with `-O3 -march=native` (enables `popcnt` instruction).

## Multi‑Platform Caveats

See the [API documentation](#caveats) for a detailed discussion of endianness, popcount fallbacks, alignment, thread safety, and MSVC / 32‑bit platform considerations.

## Requirements

- C++17 or later (C++20 for `std::popcount`; fallback provided).
- A compiler supporting `__builtin_popcountll` (GCC, Clang) or C++20.

## License

MIT License – see repository.

## Contributing

Issues and pull requests are welcome. Please follow the existing style and add tests for new features.

---

# API Documentation

## Namespace `hdc`

All types and functions are inside the `hdc` namespace.

## Class Template `static_hypervector<size_t Dimension, typename BlockType = uint64_t>`

A hypervector whose dimension is known at compile time. Stored as a fixed‑size array of `BlockType`.

### Type Traits

| Member | Value |
|--------|-------|
| `dimension` | `Dimension` (static) |
| `block_count` | number of blocks needed |
| `block_type` | `BlockType` |

### Constructors

- `constexpr static_hypervector() noexcept` – zero‑initialised.
- `explicit constexpr static_hypervector(bool bit) noexcept` – all bits set to `bit`.
- `constexpr static_hypervector(std::initializer_list<BlockType> blocks)` – set raw blocks (low‑order bits first). Missing blocks are zero.

### Bit Access

- `bool get_bit(size_t pos) const` – read bit. Bounds checked in debug builds.
- `void set_bit(size_t pos, bool value)` – write bit. Bounds checked in debug builds.

### Block Access (advanced)

- `constexpr BlockType block(size_t idx) const noexcept`
- `constexpr void set_block(size_t idx, BlockType value) noexcept`

### In‑place Operations

- `constexpr static_hypervector& bind_inplace(const static_hypervector& other) noexcept` – XOR.
- `static_hypervector& bundle_inplace(const static_hypervector& a, const static_hypervector& b) noexcept` – majority of `*this`, `a`, `b`. Result stored in `*this`.
- `static_hypervector& permute_inplace(size_t shift) noexcept` – cyclic left shift by `shift` bits.

### Similarity

- `size_t hamming_distance(const static_hypervector& other) const noexcept`
- `int64_t dot_product(const static_hypervector& other) const noexcept` – treats bits as bipolar: 0 → -1, 1 → +1. Returns sum of products (range `[-Dimension, Dimension]`).

### Comparison

- `bool operator==(const static_hypervector&) const noexcept`
- `bool operator!=(const static_hypervector&) const noexcept`

### Output

- `friend std::ostream& operator<<(std::ostream& os, const static_hypervector& hv)` – prints a binary string (MSB first? Actually LSB first due to bit order inside block – see note).

> **Bit order:** Bit `0` is the least significant bit of the first block. `get_bit(0)` returns that LSB. The `operator<<` prints from bit `0` to bit `Dimension-1`, so the printed string’s leftmost character corresponds to bit `0`. This is arbitrary but consistent.

## Class Template `dynamic_hypervector<typename BlockType = uint64_t>`

A hypervector whose dimension is set at runtime. Uses `std::vector<BlockType>` as storage.

### Constructors

- `explicit dynamic_hypervector(size_t dimension)` – zero‑initialised.
- `dynamic_hypervector(size_t dimension, bool bit)` – all bits set to `bit`.
- Copy/move constructors and assignment – defaulted.

### Accessors

- `size_t dimension() const noexcept`
- `const std::vector<BlockType>& blocks() const noexcept` – raw block vector.
- `BlockType block(size_t idx) const` – bounds‑checked (throws `std::out_of_range`).
- `bool get_bit(size_t pos) const` – bounds‑checked (throws).
- `void set_bit(size_t pos, bool value)` – bounds‑checked (throws).

### In‑place Operations (throw `std::invalid_argument` on dimension mismatch)

- `dynamic_hypervector& bind_inplace(const dynamic_hypervector& other)`
- `dynamic_hypervector& bundle_inplace(const dynamic_hypervector& a, const dynamic_hypervector& b)`
- `dynamic_hypervector& permute_inplace(size_t shift)`

### Similarity (throw on dimension mismatch)

- `size_t hamming_distance(const dynamic_hypervector& other) const`
- `int64_t dot_product(const dynamic_hypervector& other) const`

### Comparison & Output

- Same as `static_hypervector`.

## Free Functions

These return new hypervectors (copy + operation):

- `template<typename Hypervec> Hypervec bind(const Hypervec& a, const Hypervec& b)`
- `template<typename Hypervec> Hypervec bundle(const Hypervec& a, const Hypervec& b, const Hypervec& c)`
- `template<typename Hypervec> Hypervec permute(const Hypervec& hv, size_t shift)`

The `Hypervec` type can be either `static_hypervector` or `dynamic_hypervector`. The returned type is the same as the input.

---

## Caveats for Multi‑Platform Usage

| Issue | Implication | Workaround / Guarantee |
|-------|-------------|------------------------|
| **Endianness** | The library interprets the lowest addressed byte as the least significant bits. On a big‑endian machine, raw block dumps will differ. | All internal operations are endian‑agnostic because they never rely on byte order. Serialising blocks to a file or network requires converting to a fixed endianness (e.g., little‑endian) using `std::byteswap` or similar. |
| **Popcount** | If not using C++20, the library expects GCC/Clang intrinsics. MSVC will fail. | Add an `#ifdef _MSC_VER` inside `popcount_impl` to call `__popcnt64`. |
| **Alignment** | `std::array` and `std::vector` may not align blocks to 64‑byte cache lines. | For large hypervectors, you can replace `std::array` with `alignas(64) std::array` or use a custom allocator for `std::vector`. Not required for correctness. |
| **Thread safety** | Concurrent reads are safe; concurrent writes to the same hypervector are not. | Users must provide external locking (e.g., `std::mutex`) if modifying the same hypervector from multiple threads. |
| **Exception behaviour** | Debug builds throw `std::out_of_range`. Release builds skip bounds checks. | Compile with `-DNDEBUG` to disable bounds checks and exceptions in bit access. For dynamic operations (`bind_inplace` on mismatched dimensions), exceptions are always thrown. Define `HDC_NO_EXCEPTIONS` to replace them with `std::abort()`. |
| **32‑bit platforms** | `uint64_t` operations may be slower. | Instantiate templates with `uint32_t` as `BlockType`. Example: `static_hypervector<10000, uint32_t>`. |
| **MSVC `constexpr` limitations** | Some `constexpr` expressions involving bit shifts may fail on older MSVC. | The library uses `constexpr` only where guaranteed to work. If you encounter errors, remove `constexpr` from the affected methods (they are `noexcept` anyway). |

---

## Example: Complete Program

```cpp
#include "hdc/hypervector.hpp"
#include <iostream>
#include <random>

int main() {
    using namespace hdc;
    constexpr size_t D = 10000;

    // Create three random static hypervectors
    static_hypervector<D> a, b, c;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1);
    for (size_t i = 0; i < D; ++i) {
        a.set_bit(i, dist(rng));
        b.set_bit(i, dist(rng));
        c.set_bit(i, dist(rng));
    }

    // Bundle a and b with c
    auto bundled = bundle(a, b, c);

    // Bind the bundled vector with a permutation of a
    auto perm_a = permute(a, 100);
    auto result = bind(bundled, perm_a);

    // Compare with original a
    double similarity = static_cast<double>(result.dot_product(a)) / D;
    std::cout << "Cosine similarity: " << similarity << std::endl;

    return 0;
}
```

---

## Building and Testing

No build required – just include the header. To run a quick test, compile with:

```bash
g++ -std=c++17 -O3 -march=native -DNDEBUG my_program.cpp -o my_program
```

### CMake Integration

```bash
mkdir build && cd build
cmake ..
cmake --install .  # optional, installs to /usr/local by default
```

### Conan Integration

```bash
conan create .
```

To use in your project, add to your `conanfile.txt`:

```text
[requires]
hdcpp/1.0.0

[generators]
CMakeDeps
CMakeToolchain
```

---
