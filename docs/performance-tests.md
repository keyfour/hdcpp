# Performance Tests

## Approach

Custom microbenchmarks using `std::chrono::high_resolution_clock`. No external dependencies — the library itself is header-only, so benchmarks should be too.

## Design

### Timing Harness

A simple template function that runs an operation N times, measures total wall time, and reports:

- **Total time** (ms)
- **Throughput** (million bits / second)
- **Mean time per call** (ns)
- **Memory bandwidth** (GiB/s) — for operations that read/write data

```cpp
template <typename F>
bench_result measure(size_t iterations, F&& fn);
```

### Structure

```
tests/
└── perf_hypervector.cpp    # single benchmark file
```

Build via a separate CMake target:

```cmake
option(hdcpp_BUILD_PERF "Build performance benchmarks" OFF)
if(hdcpp_BUILD_PERF)
    add_executable(hdcpp_perf perf_hypervector.cpp)
    target_link_libraries(hdcpp_perf PRIVATE hdcpp)
endif()
```

## Benchmark Matrix

### Dimensions

| Size | Purpose |
|------|---------|
| 1,000 | Small — cache resident, measures call overhead |
| 10,000 | Typical HD computing dimension |
| 100,000 | Large — L2/L3 cache pressure |
| 1,000,000 | Very large — memory bandwidth bound |
| 10,000,000 | Extreme — swap / huge page behavior |

### Block Types

| Type | Purpose |
|------|---------|
| `uint64_t` | Default, native word size on 64-bit |
| `uint32_t` | 32-bit platforms, smaller cache footprint |
| `uint16_t` | Extremely narrow — more iterations per op |
| `uint8_t` | Byte-level — tests loop overhead dominance |

### Vector Types

- `static_hypervector<D, BlockType>` — compile-time dimension
- `dynamic_hypervector<BlockType>` — runtime dimension

### Operations

| Operation | Notes |
|-----------|-------|
| `bind_inplace` | Pure XOR over blocks — should be fastest |
| `bundle_inplace` | Majority — three reads, one write per block |
| `permute_inplace` | Cyclic shift — copies then reassigns; O(dim) bit ops |
| `hamming_distance` | Read-only, popcount per block |
| `dot_product` | Read-only, popcount per block + arithmetic |

### Compiler / Flag Combinations

| Configuration | C++ Standard | Flags |
|--------------|-------------|-------|
| GCC -O2 | C++17 | `-O2` |
| GCC -O3 native | C++17 | `-O3 -march=native` |
| GCC -O3 native no-assert | C++17 | `-O3 -march=native -DNDEBUG` |
| Clang -O2 | C++17 | `-O2` |
| Clang -O3 native | C++17 | `-O3 -march=native` |

## Metrics

### Raw Performance

| Metric | Unit | How |
|--------|------|-----|
| Time per operation | ns | `total_time / iterations` |
| Throughput | Mbit/s | `(dimension * iterations) / total_time_s / 1e6` |
| Throughput | Mop/s | `iterations / total_time_s / 1e6` |

### Relative Comparisons

- **Static vs. dynamic overhead**: ratio of dynamic time / static time
- **Block type efficiency**: how throughput scales with block size
- **Compiler flag impact**: speedup of `-O3 -march=native` over `-O2`
- **Dimension scaling**: log-log plot of time vs. dimension

### Cycle Counting (Optional, x86_64 only)

Use `__rdtsc()` to measure cycles per operation when `__x86_64__` is defined:

```
cycles per operation = (tsc_end - tsc_start) / iterations
cycles per bit = cycles per operation / dimension
```

## Expected Results (Reference)

Rough estimates for `uint64_t` blocks on a modern x86_64 (Skylake-ish) at 3 GHz:

| Operation | cycles/bit (O3 native) | Notes |
|-----------|----------------------|-------|
| bind_inplace | ~0.5 | Single XOR per 64-bit word |
| bundle_inplace | ~1.0 | 3 reads + majority logic + write |
| hamming_distance | ~1.0 | XOR + popcount per word |
| dot_product | ~1.5 | XOR + popcount + accumulate |
| permute_inplace | ~3–5 | Copy + bit-by-bit reassign |

## Output Format

CSV with headers for easy plotting:

```
vector_type,block_type,dimension,operation,iterations,total_time_ns,throughput_mbits
static,uint64_t,10000,bind_inplace,100000,1234567,8100.5
```

## Platform Documentation

For each platform tested, record:

- **CPU**: model, frequency, cache sizes (L1/L2/L3)
- **RAM**: type, size, channels
- **OS**: kernel version, distro
- **Compiler**: version, flags used
- **Popcount method**: intrinsic or `std::popcount`

## Future Work

- Automate benchmarks on CI (GitHub Actions Matrix: ubuntu + macOS)
- Track regression over commits (store CSV in repo or artifacts)
- Generate plots (gnuplot or Python script) — `scripts/plot_perf.py`
- Add NUMA-aware allocation tests
- Add aligned allocator tests (`alignas(64)`)
