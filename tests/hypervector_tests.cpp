// tests/hypervector_tests.cpp
#include <hdcpp/hypervector.hpp>

#include <cassert>
#include <iostream>
#include <stdexcept>

using namespace hdc;

static int failures = 0;

template <typename T> void check(const char *name, T got, T expected) {
  if (got != expected) {
    std::cerr << "FAIL: " << name << " got " << got << " expected " << expected
              << "\n";
    ++failures;
  }
}

int main() {
  // --- static: trailing bits sanitized in initializer_list ctor ---
  {
    static_hypervector<10> h({static_cast<uint64_t>(0xFF) << 20});
    static_hypervector<10> z(false);
    check("static ctor trailing hamming", h.hamming_distance(z), size_t{0});

    static_hypervector<10> h2({static_cast<uint64_t>(0xFF) << 20});
    check("static ctor equality", h == h2, true);

    // dot_product must also ignore the garbage high bits: h's valid bits are
    // all 0, ones' valid bits are all 1 -> all different -> -10
    static_hypervector<10> ones(true);
    check("static ctor trailing dot", h.dot_product(ones), int64_t{-10});
  }

  // --- static: dot_product for non-multiple dimension ---
  {
    static_hypervector<10> a(true), b(true);
    check("static dot identical", a.dot_product(b), int64_t{10});
    static_hypervector<10> c(false);
    check("static dot opposite", a.dot_product(c), int64_t{-10});
  }

  // --- static: bool ctor trailing bits ---
  {
    static_hypervector<10> all_one(true);
    static_hypervector<10> ref(false);
    for (size_t i = 0; i < 10; ++i)
      ref.set_bit(i, true);
    check("static bool ctor", all_one == ref, true);
    check("static bool ctor hamming vs zero", all_one.hamming_distance(
                                                 static_hypervector<10>(false)),
          size_t{10});
  }

  // --- dot_product / hamming for non-multiple dimensions ---
  {
    dynamic_hypervector<uint64_t> a(10, true), b(10, true);
    check("dot identical", a.dot_product(b), int64_t{10});
    dynamic_hypervector<uint64_t> c(10, false), d(10, false);
    check("dot zero-zero", c.dot_product(d), int64_t{10});
    check("dot opposite", a.dot_product(c), int64_t{-10});
    check("hamming identical", a.hamming_distance(b), size_t{0});
    check("hamming opposite", a.hamming_distance(c), size_t{10});
  }

  // --- permute (cyclic shift) ---
  {
    dynamic_hypervector<uint64_t> a(8, false);
    a.set_bit(0, true);
    a.set_bit(7, true);
    auto b = permute(a, 1);
    // left cyclic shift by 1: old bit0 -> new bit7, old bit7 -> new bit6
    check("permute bit0->7", b.get_bit(7), true);
    check("permute bit7->6", b.get_bit(6), true);
    check("permute bit0 clear", b.get_bit(0), false);
    auto c = permute(b, 7); // shift back
    check("permute inverse", c == a, true);
  }

  // --- bundle majority ---
  {
    dynamic_hypervector<uint64_t> x(8, false), y(8, false), z(8, false);
    x.set_bit(0, true);
    y.set_bit(1, true);
    z.set_bit(2, true);
    auto r = bundle(x, y, z); // all different -> no majority -> 0
    check("bundle all-different", r == dynamic_hypervector<uint64_t>(8, false),
          true);
    x.set_bit(3, true);
    y.set_bit(3, true); // majority at bit 3
    auto r2 = bundle(x, y, z);
    check("bundle majority bit3", r2.get_bit(3), true);
  }

  // --- bind (XOR) ---
  {
    dynamic_hypervector<uint64_t> a(8, false);
    a.set_bit(0, true);
    auto c = bind(a, a);
    check("bind self = zero", c == dynamic_hypervector<uint64_t>(8, false),
          true);
  }

  // --- dynamic: trailing bits ignored after set_bit ---
  {
    dynamic_hypervector<uint64_t> a(10, false);
    // set a bit beyond the valid 10 bits would be OOB, so instead verify that
    // a constructor-fill then read of valid range is consistent and that
    // sanitize_last_block keeps stored block's high bits zero.
    dynamic_hypervector<uint64_t> b(10, true);
    dynamic_hypervector<uint64_t> z(10, false);
    check("dynamic fill hamming", b.hamming_distance(z), size_t{10});
    // internal last block high bits must be 0
    uint64_t last = b.blocks().back();
    check("dynamic last block sanitized", (last >> 10) == 0, true);
  }

  // --- in-place ops (not just free functions) ---
  {
    dynamic_hypervector<uint64_t> a(8, false), b(8, false);
    a.set_bit(0, true);
    b.set_bit(0, true);
    a.bind_inplace(b);
    check("dynamic bind_inplace self=0",
          a == dynamic_hypervector<uint64_t>(8, false), true);

    dynamic_hypervector<uint64_t> x(8, false), y(8, false), z3(8, false);
    x.set_bit(2, true);
    y.set_bit(2, true);
    z3.set_bit(5, true);
    dynamic_hypervector<uint64_t> r(8, false);
    r.bundle_inplace(x, y); // majority of r(0),x,y at bit2 -> 1
    check("dynamic bundle_inplace majority", r.get_bit(2), true);
    check("dynamic bundle_inplace no-majority", r.get_bit(5), false);
  }

  // --- permute_inplace inverse for several shifts (covers shift %= dim_) ---
  {
    dynamic_hypervector<uint64_t> a(8, false);
    for (size_t i = 0; i < 8; ++i)
      a.set_bit(i, (i % 2) == 0);
    for (size_t s = 0; s < 8; ++s) {
      auto b = a;
      b.permute_inplace(s);
      b.permute_inplace((8 - s) % 8);
      check("dynamic permute_inplace inverse", b == a, true);
    }
    // shift by dimension is identity
    auto c = a;
    c.permute_inplace(8);
    check("dynamic permute_inplace dim=identity", c == a, true);
  }

  // --- popcount_impl portability check ---
  {
    check("popcount 0", popcount_impl(uint64_t{0}), 0);
    check("popcount 0xFF", popcount_impl(uint64_t{0xFF}), 8);
    check("popcount all ones u32",
          popcount_impl(uint32_t{0xFFFFFFFFu}), 32);
    check("popcount single bit u8", popcount_impl(uint8_t{1u << 4}), 1);
  }

  // --- bounds checking throws ---
  {
    dynamic_hypervector<uint64_t> a(8, false);
    bool threw = false;
    try {
      a.get_bit(8);
    } catch (const std::out_of_range &) {
      threw = true;
    }
    check("dynamic get_bit OOB throws", threw, true);

    static_hypervector<8> s;
    threw = false;
    try {
      s.get_bit(8);
    } catch (const std::out_of_range &) {
      threw = true;
    }
    check("static get_bit OOB throws", threw, true);

    threw = false;
    try {
      s.set_bit(8, true);
    } catch (const std::out_of_range &) {
      threw = true;
    }
    check("static set_bit OOB throws", threw, true);

    dynamic_hypervector<uint64_t> a8(8, false);
    threw = false;
    try {
      a8.set_bit(8, true);
    } catch (const std::out_of_range &) {
      threw = true;
    }
    check("dynamic set_bit OOB throws", threw, true);
  }

  // --- dimension 0 rejected ---
  {
    bool threw = false;
    try {
      dynamic_hypervector<uint64_t> a(0);
    } catch (const std::invalid_argument &) {
      threw = true;
    }
    check("dynamic dim 0 throws", threw, true);
  }

  if (failures == 0) {
    std::cout << "All tests passed.\n";
    return 0;
  }
  std::cerr << failures << " test(s) failed.\n";
  return 1;
}
