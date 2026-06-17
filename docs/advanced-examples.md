# Advanced Examples

## Overview

Four compilable `.cpp` files in `examples/` demonstrating real HD computing applications. Each includes a `main()` function, uses only the C++ standard library, and can be compiled with the same `g++` command shown in the README.

## Example 1: Text Language Classification

**File:** `examples/lang_classify.cpp`

**Concept:** Encode character bigrams into hypervectors and classify text by language.

**Algorithm:**
1. Create a random *item memory* assigning a hypervector to each lowercase letter (a–z).
2. For each document, encode it by binding adjacent character HV pairs (bigrams), permuted by position, then bundling all bigrams into a single document HV.
3. Build prototype HVs for each language from labeled training documents.
4. Classify an unknown document by Hamming distance to each prototype.

**Demonstrates:**
- Random item memory creation
- Binding (`bind`) for n-gram encoding
- Permutation for position sensitivity
- Bundling (`bundle`) for accumulation
- Similarity (`hamming_distance`) for classification

**Sample data:** Short hard-coded English, French, German sentences (no external files needed).

**Expected output:**
```
Classifying 3 test documents...
  doc1 -> English (correct)
  doc2 -> French  (correct)
  doc3 -> German  (correct)
Accuracy: 100%
```

## Example 2: Associative Memory (Item Memory / Cleanup)

**File:** `examples/assoc_memory.cpp`

**Concept:** Store key-value bindings in a single hypervector, then recover values by querying with a key.

**Algorithm:**
1. Create random key HVs (`color_name → HV`) and value HVs (`rgb_code → HV`), e.g., color names→RGB.
2. For each key-value pair, bind the key HV with the value HV → a *binding HV*.
3. Bundle all binding HVs together into a single *associative memory* HV.
4. To query: bind the query key HV with the memory HV → noisy version of the value HV.
5. "Clean up" by finding the closest value HV in the item memory (exhaustive similarity search).

**Demonstrates:**
- Binding as a key-value encoding mechanism
- Bundling as a superposition of associations
- The cleanup / similarity search pattern (fundamental in HD computing)

**Sample data:** 5 color entries (red→#FF0000, green→#00FF00, blue→#0000FF, etc.).

**Expected output:**
```
Query: red    -> recovered #FF0000 (confidence 0.92)
Query: green  -> recovered #00FF00 (confidence 0.94)
Query: blue   -> recovered #0000FF (confidence 0.88)
Query: yellow -> recovered #FFFF00 (confidence 0.85)
Query: black  -> recovered #000000 (confidence 0.91)
```

## Example 3: Sequence Encoding & Comparison

**File:** `examples/sequence_encoding.cpp`

**Concept:** Encode variable-length sequences as hypervectors and compare their similarity.

**Algorithm:**
1. Create item memory for symbols (A, B, C, D, etc.).
2. Encode a sequence by permuting each symbol's HV by its position, then bundling all position-specific HVs.
3. Show that:
   - A sequence is most similar to itself.
   - Similar sequences (e.g., "ABC" vs. "ABD") have higher similarity than dissimilar ones ("ABC" vs. "XYZ").
   - Reversed sequences have low similarity (permutation is directional).
   - Subsequence similarity (shifts vs. truncations).

**Demonstrates:**
- Permutation for position encoding
- Bundling for sequence summarization
- Similarity as a proxy for sequence alignment

**Expected output (illustrative values):**
```
Comparing sequences (dim=10000):
  ABC vs ABC     similarity: 1.000
  ABC vs ABD     similarity: ~0.85
  ABC vs XYZ     similarity: ~0.50
  ABC vs CBA     similarity: ~0.51  (reversal)
  ABC vs AB      similarity: ~0.61  (subsequence)
  ABC vs ABCD    similarity: ~0.78  (prefix)
```

## Example 4: Hyperdimensional Graph Encoding

**File:** `examples/graph_encoding.cpp`

**Concept:** Encode small graphs as hypervectors and measure graph similarity.

**Algorithm:**
1. Create item memory for node labels (A, B, C, …).
2. For each edge `(u, v)` in the graph, create a binding `permute(node_u, role_u) ^ permute(node_v, role_v)`, where `role_u` and `role_v` are fixed permute amounts distinguishing source vs. target.
3. Bundle all edge HVs → graph HV.
4. Compare graphs by dot product similarity.

**Demonstrates:**
- Graph encoding via edge-level binding
- Role-based permutation (source vs. target distinction)
- Graph similarity without alignment

**Sample data:** Small directed graphs (3–5 nodes, 4–6 edges).

**Expected output (illustrative values):**
```
Graph similarity matrix:
         G1     G2     G3     G4
   G1   1.000  0.51   0.72   0.49
   G2   0.51   1.000  0.50   0.70
   G3   0.72   0.50   1.000  0.52
   G4   0.49   0.70   0.52   1.000

Interpretation: G1 and G3 share 2 of 4 edges.
```

## Build Instructions

Each example compiles with the same command:

```bash
g++ -std=c++17 -O3 -march=native -DNDEBUG examples/<name>.cpp -o <name>
```

For CMake, add targets:

```cmake
option(hdcpp_BUILD_EXAMPLES "Build examples" OFF)
if(hdcpp_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
```

With an `examples/CMakeLists.txt` listing each example as an executable.

## Common Patterns Across Examples

| Pattern | Used by |
|---------|---------|
| Random item memory via `std::mt19937` | All examples |
| Fixed seed for reproducibility | All examples |
| Bipolar cosine similarity (dot_product / dimension) | All examples |
| Bundling with normalization (majority vote) | Ex1, Ex2 |
| Permutation for position/role encoding | Ex1, Ex3, Ex4 |
| Cleanup via exhaustive nearest-neighbor search | Ex2 |

## Future Expansion

- Add a record-based classification example (e.g., Iris dataset encoded as hypervectors)
- Add a simple image recognition example (binary pixel patterns → class HVs)
- Pull in small external datasets (Shakespeare, UCI ML repo) for more realistic demos
