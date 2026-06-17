# Unit Tests

## Framework: doctest

Use the single-header version of [doctest](https://github.com/doctest/doctest) (fastest compile, lightweight, nearly identical API to Catch2).

## Location

```
tests/
├── doctest.h              # bundled single header
├── CMakeLists.txt          # test target
└── test_hypervector.cpp    # all tests
```

## Test Coverage

### 1. Constructors

| Test | Variant | Details |
|------|---------|---------|
| Default construction | static + dynamic | All bits should be 0 |
| Fill true | static + dynamic | All bits should be 1 |
| Fill false | static + dynamic | All bits should be 0 |
| Initializer list | static only | Blocks set correctly, remainder zero |
| Copy construction | static + dynamic | Bits identical, independent storage |
| Move construction | dynamic | Source left empty |

### 2. Bit Access

| Test | Details |
|------|---------|
| get_bit / set_bit round-trip | Set each bit in [0, dim), verify with get_bit |
| Boundary positions | pos=0, pos=dim-1, middle |
| set_bit overwrite | Set bit 0→1→0→1, verify each transition |
| Out-of-range (debug only) | pos >= dim triggers `std::out_of_range` |
| Dynamic dimension | `dimension()` returns constructor argument |

### 3. Binding (XOR)

| Test | Details |
|------|---------|
| Self-binding | `a ^ a == 0` |
| Zero identity | `a ^ 0 == a` |
| Commutativity | `a ^ b == b ^ a` |
| Associativity | `(a ^ b) ^ c == a ^ (b ^ c)` |
| Free function | `bind(a, b)` returns new vector, originals unchanged |
| Chaining | `a.bind_inplace(b).bind_inplace(c) == a ^ b ^ c` |

### 4. Bundling (Majority Vote)

| Test | Details |
|------|---------|
| All equal | `bundle(a, a, a) == a` |
| Two equal | `bundle(a, a, b) == a` (majority is `a`) |
| All different (50% pop) | Averaging property — each bit matches majority |
| Free function | `bundle(a, b, c)` returns new, originals unchanged |
| Truth table | For all 8 combos of 3 bits, verify majority result |

### 5. Permutation (Cyclic Shift)

| Test | Details |
|------|---------|
| Zero shift | `permute(a, 0) == a` |
| Full cycle | `permute(a, dim) == a` |
| Composition | `permute(permute(a, s1), s2) == permute(a, (s1+s2)%dim)` |
| Inverse | `permute(permute(a, s), dim-s) == a` |
| One-bit rotation | Bootstrap: set bit 0, permute by 1, verify bit 1 is now 1 |
| Free function | `permute(a, s)` returns new, original unchanged |

### 6. Similarity Measures

| Test | Details |
|------|---------|
| Hamming self | `hamming_distance(a, a) == 0` |
| Hamming complement | `hamming_distance(a, ~a) == dim` |
| Hamming symmetric | `hd(a,b) == hd(b,a)` |
| Dot product self | `dot_product(a, a) == dim` |
| Dot product complement | `dot_product(a, ~a) == -dim` |
| Dot product random | Range check: `abs(dot_product(a,b)) <= dim` |
| Dot product symmetric | `dot(a,b) == dot(b,a)` |

### 7. Dynamic-Only: Dimension Mismatch Exceptions

All operations throw `std::invalid_argument` when dimensions differ:

- `bind_inplace`
- `bundle_inplace`
- `permute_inplace` (no — shift only, doesn't check)
- `hamming_distance`
- `dot_product`

### 8. Edge Cases

| Test | Details |
|------|---------|
| Dimension = 1 | Smallest valid static vector |
| Dimension not multiple of 64 | Trailing bits always zero despite set_bit attempts |
| Dimension = 2 | Tests boundary of `(dim % bits_per_block) == 2` |
| BlockType = uint32_t | Static + dynamic with 32-bit blocks |
| BlockType = uint16_t | Static + dynamic with 16-bit blocks |
| BlockType = uint8_t | Static + dynamic with 8-bit blocks |

### 9. Constexpr Evaluation (static only)

Verify that operations compile and run at constexpr context:

```cpp
constexpr static_hypervector<64> a(true);
constexpr static_hypervector<64> b(false);
constexpr auto c = bind(a, b);
constexpr auto d = permute(a, 3);
static_assert(c.hamming_distance(b) == 64);
```

### 10. Output / Comparison

| Test | Details |
|------|---------|
| `operator==` | Vectors with same bits are equal |
| `operator==` | Vectors with different bits are not equal |
| `operator<<` | Output string length matches dimension |

## CMake Integration

```cmake
# tests/CMakeLists.txt
enable_testing()

# Fetch doctest single header
add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_hdcpp test_hypervector.cpp)
target_link_libraries(test_hdcpp PRIVATE hdcpp doctest)
target_compile_definitions(test_hdcpp PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)

add_test(NAME hdcpp COMMAND test_hdcpp)
```

Register in root `CMakeLists.txt`:

```cmake
option(hdcpp_BUILD_TESTS "Build unit tests" OFF)
if(hdcpp_BUILD_TESTS)
    add_subdirectory(tests)
endif()
```

## Running Tests

```bash
cmake -B build -Dhdcpp_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Future Expansion

- Split into `test_static.cpp` and `test_dynamic.cpp` when tests grow large
- Add property-based tests with random vectors (fixed-seed RNG for reproducibility)
- Add CI workflow (GitHub Actions) to run tests on push/PR
- Add sanitizer builds (ASan, UBSan) for debug
