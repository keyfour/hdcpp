// hdcpp/hypervector.hpp
#ifndef HDC_HYPERVECTOR_HPP
#define HDC_HYPERVECTOR_HPP

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace hdc {

// -----------------------------------------------------------------------------
// Internal utilities
// -----------------------------------------------------------------------------

// Popcount: fallback for compilers without C++20 std::popcount
template <typename T> inline constexpr int popcount_impl(T x) noexcept {
  if constexpr (std::is_same_v<T, unsigned long long>)
    return __builtin_popcountll(x); // GCC/Clang
  else if constexpr (std::is_same_v<T, unsigned long>)
    return __builtin_popcountl(x);
  else if constexpr (std::is_same_v<T, unsigned int>)
    return __builtin_popcount(x);
  else
    return std::__popcount(x); // C++20 for all other unsigned types
}

// Block type traits
template <typename BlockType> struct block_traits {
  static constexpr size_t bits_per_block = sizeof(BlockType) * 8;
  static_assert(std::is_unsigned_v<BlockType>,
                "BlockType must be unsigned integer");
};

// Number of blocks needed to store 'dim' bits
template <typename BlockType>
constexpr size_t num_blocks_for_dim(size_t dim) noexcept {
  return (dim + block_traits<BlockType>::bits_per_block - 1) /
         block_traits<BlockType>::bits_per_block;
}

// -----------------------------------------------------------------------------
// Static hypervector (compile‑time dimension)
// -----------------------------------------------------------------------------
template <size_t Dimension, typename BlockType = uint64_t>
class static_hypervector {
  static_assert(Dimension > 0, "Dimension must be positive");
  static constexpr size_t num_blocks = num_blocks_for_dim<BlockType>(Dimension);
  static constexpr size_t bits_per_block =
      block_traits<BlockType>::bits_per_block;

  std::array<BlockType, num_blocks> data_{};

public:
  using block_type = BlockType;
  static constexpr size_t dimension = Dimension;
  static constexpr size_t block_count = num_blocks;

  // Constructors
  constexpr static_hypervector() noexcept = default;

  // Fill with a constant bit (0 or 1)
  explicit constexpr static_hypervector(bool bit) noexcept {
    BlockType fill = bit ? ~BlockType(0) : BlockType(0);
    data_.fill(fill);
    // Zero out trailing bits in the last block (if dimension not a multiple of
    // block size)
    if constexpr (Dimension % bits_per_block != 0) {
      constexpr size_t valid_bits = Dimension % bits_per_block;
      constexpr BlockType mask = (BlockType(1) << valid_bits) - 1;
      data_.back() &= mask;
    }
  }

  // Construct from initializer list of blocks (low‑order bits first)
  constexpr static_hypervector(std::initializer_list<BlockType> blocks) {
    auto it = data_.begin();
    auto blk = blocks.begin();
    for (; it != data_.end() && blk != blocks.end(); ++it, ++blk)
      *it = *blk;
    // remaining blocks stay zero
  }

  // Access raw block (read‑only)
  constexpr BlockType block(size_t idx) const noexcept { return data_[idx]; }

  // Set a block (for advanced use)
  constexpr void set_block(size_t idx, BlockType value) noexcept {
    data_[idx] = value;
  }

  // Bit access (with bounds checking in debug)
  bool get_bit(size_t pos) const {
#ifdef _DEBUG
    if (pos >= Dimension)
      throw std::out_of_range("bit position out of range");
#endif
    size_t block_idx = pos / bits_per_block;
    size_t bit_idx = pos % bits_per_block;
    return (data_[block_idx] >> bit_idx) & 1;
  }

  void set_bit(size_t pos, bool value) {
#ifdef _DEBUG
    if (pos >= Dimension)
      throw std::out_of_range("bit position out of range");
#endif
    size_t block_idx = pos / bits_per_block;
    size_t bit_idx = pos % bits_per_block;
    if (value)
      data_[block_idx] |= (BlockType(1) << bit_idx);
    else
      data_[block_idx] &= ~(BlockType(1) << bit_idx);
  }

  // In‑place binding (XOR)
  constexpr static_hypervector &
  bind_inplace(const static_hypervector &other) noexcept {
    for (size_t i = 0; i < num_blocks; ++i)
      data_[i] ^= other.data_[i];
    return *this;
  }

  // In‑place bundling (majority vote with a third hypervector, typical HD
  // bundling) This implements bundling as bitwise majority: for each bit, the
  // value that appears at least twice.
  static_hypervector &bundle_inplace(const static_hypervector &a,
                                     const static_hypervector &b) noexcept {
    for (size_t i = 0; i < num_blocks; ++i) {
      BlockType x = data_[i];
      BlockType y = a.data_[i];
      BlockType z = b.data_[i];
      // Majority = (x & y) | (x & z) | (y & z)
      data_[i] = (x & y) | (x & z) | (y & z);
    }
    return *this;
  }

  // In‑place permutation (cyclic shift left by `shift` bits)
  static_hypervector &permute_inplace(size_t shift) noexcept {
    shift %= Dimension;
    if (shift == 0)
      return *this;
    static_hypervector copy = *this;
    for (size_t i = 0; i < Dimension; ++i) {
      bool bit = copy.get_bit((i + shift) % Dimension);
      set_bit(i, bit);
    }
    return *this;
  }

  // Hamming distance (number of differing bits)
  size_t hamming_distance(const static_hypervector &other) const noexcept {
    size_t dist = 0;
    for (size_t i = 0; i < num_blocks; ++i) {
      BlockType diff = data_[i] ^ other.data_[i];
      dist += popcount_impl(diff);
    }
    // Trailing bits in last block should be zero; they never contribute.
    return dist;
  }

  // Cosine similarity for bipolar interpretation (treat 0 as -1, 1 as +1)
  // Returns dot product (range –dim … dim). For cosine, divide by dimension.
  int64_t dot_product(const static_hypervector &other) const noexcept {
    int64_t sum = 0;
    for (size_t i = 0; i < num_blocks; ++i) {
      BlockType x = data_[i];
      BlockType y = other.data_[i];
      // count where both bits are 1: popcount(x & y)
      // count where both bits are 0: popcount(~x & ~y) = popcount(~(x|y))
      // contribution = (+1)*(+1) for 1,1 and (-1)*(-1) for 0,0 => both +1.
      // contribution = 2*popcount(x&y) + 2*popcount(~(x|y)) -
      // total_bits_in_block? Simpler: For each bit pair: (2*x-1)*(2*y-1) = 1 if
      // equal, -1 if different. So sum of (1 - 2*inequality). Inequality = XOR.
      // So sum = N - 2*popcount(x^y). But that's for whole vector. We can
      // compute per block:
      sum += bits_per_block - 2 * static_cast<int64_t>(popcount_impl(x ^ y));
    }
    // subtract trailing bits of last block that are not part of dimension
    constexpr size_t valid_bits_last = (Dimension % bits_per_block == 0)
                                           ? bits_per_block
                                           : (Dimension % bits_per_block);
    constexpr size_t extra_bits = bits_per_block - valid_bits_last;
    // The trailing bits are always zero, so they contribute as equal (both -1?
    // Actually our storage treats extra bits as 0, but those positions do not
    // belong to the vector. They must not affect the dot product. We simply
    // subtract the contribution that we artificially added for them: For each
    // extra bit, x=y=0 => contribution = (+1) because
    // (2*0-1)*(2*0-1)=(-1)*(-1)=+1. We mistakenly added +1 for each extra bit.
    // So subtract extra_bits.
    sum -= extra_bits;
    return sum;
  }

  // Comparison operators
  bool operator==(const static_hypervector &other) const noexcept {
    return data_ == other.data_;
  }
  bool operator!=(const static_hypervector &other) const noexcept {
    return !(*this == other);
  }

  // Output for debugging
  friend std::ostream &operator<<(std::ostream &os,
                                  const static_hypervector &hv) {
    for (size_t i = 0; i < Dimension; ++i)
      os << (hv.get_bit(i) ? '1' : '0');
    return os;
  }
};

// -----------------------------------------------------------------------------
// Dynamic hypervector (runtime dimension)
// -----------------------------------------------------------------------------
template <typename BlockType = uint64_t> class dynamic_hypervector {
  using traits = block_traits<BlockType>;
  static constexpr size_t bits_per_block = traits::bits_per_block;

  size_t dim_;
  std::vector<BlockType> data_;

  // helper to zero out unused bits in last block
  void sanitize_last_block() noexcept {
    if (dim_ % bits_per_block != 0) {
      size_t valid_bits = dim_ % bits_per_block;
      BlockType mask = (BlockType(1) << valid_bits) - 1;
      if (!data_.empty())
        data_.back() &= mask;
    }
  }

public:
  using block_type = BlockType;

  // Constructor: create zero hypervector of given dimension
  explicit dynamic_hypervector(size_t dimension)
      : dim_(dimension), data_(num_blocks_for_dim<BlockType>(dimension), 0) {}

  // Fill with constant bit
  dynamic_hypervector(size_t dimension, bool bit)
      : dim_(dimension), data_(num_blocks_for_dim<BlockType>(dimension),
                               bit ? ~BlockType(0) : BlockType(0)) {
    sanitize_last_block();
  }

  // Copy and move
  dynamic_hypervector(const dynamic_hypervector &) = default;
  dynamic_hypervector(dynamic_hypervector &&) noexcept = default;
  dynamic_hypervector &operator=(const dynamic_hypervector &) = default;
  dynamic_hypervector &operator=(dynamic_hypervector &&) noexcept = default;

  // Accessors
  size_t dimension() const noexcept { return dim_; }
  const std::vector<BlockType> &blocks() const noexcept { return data_; }
  BlockType block(size_t idx) const { return data_.at(idx); }

  // Bit access (bounds checked)
  bool get_bit(size_t pos) const {
    if (pos >= dim_)
      throw std::out_of_range("bit position out of range");
    size_t block_idx = pos / bits_per_block;
    size_t bit_idx = pos % bits_per_block;
    return (data_[block_idx] >> bit_idx) & 1;
  }

  void set_bit(size_t pos, bool value) {
    if (pos >= dim_)
      throw std::out_of_range("bit position out of range");
    size_t block_idx = pos / bits_per_block;
    size_t bit_idx = pos % bits_per_block;
    if (value)
      data_[block_idx] |= (BlockType(1) << bit_idx);
    else
      data_[block_idx] &= ~(BlockType(1) << bit_idx);
  }

  // In‑place operations (dimensions must match)
  dynamic_hypervector &bind_inplace(const dynamic_hypervector &other) {
    if (dim_ != other.dim_)
      throw std::invalid_argument("dimension mismatch");
    for (size_t i = 0; i < data_.size(); ++i)
      data_[i] ^= other.data_[i];
    return *this;
  }

  dynamic_hypervector &bundle_inplace(const dynamic_hypervector &a,
                                      const dynamic_hypervector &b) {
    if (dim_ != a.dim_ || dim_ != b.dim_)
      throw std::invalid_argument("dimension mismatch");
    for (size_t i = 0; i < data_.size(); ++i) {
      BlockType x = data_[i], y = a.data_[i], z = b.data_[i];
      data_[i] = (x & y) | (x & z) | (y & z);
    }
    return *this;
  }

  dynamic_hypervector &permute_inplace(size_t shift) {
    shift %= dim_;
    if (shift == 0)
      return *this;
    dynamic_hypervector copy = *this;
    for (size_t i = 0; i < dim_; ++i) {
      bool bit = copy.get_bit((i + shift) % dim_);
      set_bit(i, bit);
    }
    return *this;
  }

  size_t hamming_distance(const dynamic_hypervector &other) const {
    if (dim_ != other.dim_)
      throw std::invalid_argument("dimension mismatch");
    size_t dist = 0;
    for (size_t i = 0; i < data_.size(); ++i) {
      BlockType diff = data_[i] ^ other.data_[i];
      dist += popcount_impl(diff);
    }
    return dist;
  }

  int64_t dot_product(const dynamic_hypervector &other) const {
    if (dim_ != other.dim_)
      throw std::invalid_argument("dimension mismatch");
    int64_t sum = 0;
    for (size_t i = 0; i < data_.size(); ++i) {
      sum += bits_per_block -
             2 * static_cast<int64_t>(popcount_impl(data_[i] ^ other.data_[i]));
    }
    size_t extra_bits =
        (bits_per_block - (dim_ % bits_per_block)) % bits_per_block;
    sum -= extra_bits;
    return sum;
  }

  bool operator==(const dynamic_hypervector &other) const {
    return dim_ == other.dim_ && data_ == other.data_;
  }
  bool operator!=(const dynamic_hypervector &other) const {
    return !(*this == other);
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const dynamic_hypervector &hv) {
    for (size_t i = 0; i < hv.dim_; ++i)
      os << (hv.get_bit(i) ? '1' : '0');
    return os;
  }
};

// -----------------------------------------------------------------------------
// Convenience free functions (return new hypervectors)
// -----------------------------------------------------------------------------
template <typename Hypervec>
Hypervec bind(const Hypervec &a, const Hypervec &b) {
  Hypervec result = a;
  result.bind_inplace(b);
  return result;
}

template <typename Hypervec>
Hypervec bundle(const Hypervec &a, const Hypervec &b, const Hypervec &c) {
  Hypervec result = a;
  result.bundle_inplace(b, c);
  return result;
}

template <typename Hypervec>
Hypervec permute(const Hypervec &hv, size_t shift) {
  Hypervec result = hv;
  result.permute_inplace(shift);
  return result;
}

} // namespace hdc

#endif // HDC_HYPERVECTOR_HPP
