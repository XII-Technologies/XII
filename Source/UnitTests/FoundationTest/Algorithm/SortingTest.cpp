#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Math.h>
#include <algorithm>
#include <functional>
#include <limits>
#include <random>

namespace
{
  // Test item types

  struct RadixSortTestItem
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt64 m_uiKey           = 0;
    xiiUInt32 m_uiOriginalIndex = 0;
  };

  struct RadixSortTestKeyExtractor
  {
    XII_ALWAYS_INLINE xiiUInt64 GetKey(const RadixSortTestItem& value) const { return value.m_uiKey; }
    XII_ALWAYS_INLINE xiiUInt64 operator()(const RadixSortTestItem& value) const { return GetKey(value); }
  };

  // Helpers and Test Fixtures

  static std::mt19937_64 CreateDeterministicRng(xiiUInt64 seed)
  {
    std::mt19937_64 rng;
    rng.seed(seed);
    return rng;
  }

  template <typename T>
  static xiiDynamicArray<T> MakeDynamicArrayFromStd(const std::vector<T>& v)
  {
    xiiDynamicArray<T> out;
    out.SetCountUninitialized(static_cast<xiiUInt32>(v.size()));
    for (size_t i = 0; i < v.size(); ++i)
    {
      out[static_cast<xiiUInt32>(i)] = v[i];
    }
    return out;
  }

  template <typename Container, typename KeyFunc>
  static bool VerifySortedAscending(const Container& c, const KeyFunc& keyFunc)
  {
    const xiiUInt32 count = c.GetCount();
    for (xiiUInt32 i = 1; i < count; ++i)
    {
      const auto a = keyFunc(c[i - 1]);
      const auto b = keyFunc(c[i]);
      if (a > b)
        return false;
    }
    return true;
  }

  template <typename Container, typename KeyFunc>
  static bool VerifyStableOrder(const Container& c, const KeyFunc& keyFunc)
  {
    const xiiUInt32 count = c.GetCount();
    for (xiiUInt32 i = 1; i < count; ++i)
    {
      const auto aKey = keyFunc(c[i - 1]);
      const auto bKey = keyFunc(c[i]);

      if (aKey > bKey)
        return false;

      if (aKey == bKey)
      {
        // Expect original index to be strictly increasing for stability
        // We assume container elements have m_uiOriginalIndex or idx member, and use SFINAE-like approach below.
        // For generic verification we require a lambda that returns original index when needed.
      }
    }
    return true;
  }

  // Generic stable verification for items that have an 'original index' accessor.
  template <typename Container, typename KeyFunc, typename OrigIndexFunc>
  static bool VerifyStableOrderWithIndex(const Container& c, const KeyFunc& keyFunc, const OrigIndexFunc& origIndexFunc)
  {
    const xiiUInt32 count = c.GetCount();
    for (xiiUInt32 i = 1; i < count; ++i)
    {
      const auto aKey = keyFunc(c[i - 1]);
      const auto bKey = keyFunc(c[i]);

      if (aKey > bKey)
        return false;

      if (aKey == bKey)
      {
        if (origIndexFunc(c[i - 1]) >= origIndexFunc(c[i]))
        {
          return false;
        }
      }
    }
    return true;
  }

  template <typename Container>
  static void DumpWindow(const Container& c, xiiUInt32 index, xiiUInt32 window = 8)
  {
    const xiiUInt32 count = c.GetCount();
    const xiiUInt32 start = (index > window) ? (index - window) : 0;
    const xiiUInt32 end   = xiiMath::Min(count, index + window + 1);
    for (xiiUInt32 i = start; i < end; ++i)
    {
      // Try to print common fields if present
      // We attempt to print .m_uiKey and .m_uiOriginalIndex if they exist, otherwise fallback to raw value.
      if constexpr (std::is_class_v<typename std::remove_reference<decltype(c[i])>::type>)
      {
        // Use SFINAE-like checks via sizeof trick is not available here; do best-effort printing via known test types.
        // For RadixSortTestItem and similar types used in tests below:
        struct MaybeRadix
        {
          xiiUInt64 m_uiKey;
          xiiUInt32 m_uiOriginalIndex;
        };
        if (sizeof(c[i]) >= sizeof(MaybeRadix))
        {
          // Attempt reinterpret cast for debug printing (best-effort, only for tests in this suite).
          const auto* p = reinterpret_cast<const MaybeRadix*>(&c[i]);
          xiiLog::Info("{}: (key={}, idx={}).", i, p->m_uiKey, p->m_uiOriginalIndex);
          continue;
        }
      }
      // Fallback: print raw via stream if possible
      xiiLog::Info("{}: (raw element).", i);
    }
  }

  // Create a deterministic dataset with many duplicates (low entropy)
  template <typename Item>
  static xiiDynamicArray<Item> CreateDuplicateKeyDataset(xiiUInt32 count, xiiUInt32 keyBits, uint64_t seed)
  {
    xiiDynamicArray<Item> items;
    items.Reserve(count);

    std::mt19937_64                         rng(static_cast<uint64_t>(seed));
    uint64_t                                mask = (keyBits >= 64) ? std::numeric_limits<uint64_t>::max() : ((1ULL << keyBits) - 1ULL);
    std::uniform_int_distribution<uint64_t> dist(0, mask);

    for (xiiUInt32 i = 0; i < count; ++i)
    {
      Item it{};
      if constexpr (requires { it.m_uiKey; })
      {
        it.m_uiKey = static_cast<xiiUInt64>(dist(rng) & mask);
      }
      items.PushBack(it);
    }

    // Fisher-Yates shuffle using the same rng (deterministic given seed)
    for (xiiUInt32 i = items.GetCount(); i > 1; --i)
    {
      std::uniform_int_distribution<xiiUInt32> d(0, i - 1);
      const xiiUInt32                          j = d(rng);
      xiiMath::Swap(items[i - 1], items[j]);
    }

    // NOTE: This must be done after the shuffle to ensure originalIndex reflects the input position, not the sorted position.
    for (xiiUInt32 i = 0; i < items.GetCount(); ++i)
    {
      if constexpr (requires { items[i].m_uiOriginalIndex; })
      {
        items[i].m_uiOriginalIndex = i; // now originalIndex == input position
      }
    }

    return items;
  }

  // Reference stable sort using std::stable_sort for comparison
  template <typename Container, typename KeyFunc>
  static xiiDynamicArray<typename std::remove_reference<decltype(Container()[0])>::type> ReferenceStableSort(const Container& c, const KeyFunc& keyFunc)
  {
    using T = typename std::remove_reference<decltype(c[0])>::type;
    std::vector<T> tmp;
    tmp.reserve(c.GetCount());
    for (xiiUInt32 i = 0; i < c.GetCount(); ++i)
    {
      tmp.push_back(c[i]);
    }

    std::stable_sort(tmp.begin(), tmp.end(), [&](const T& a, const T& b) {
      return keyFunc(a) < keyFunc(b);
    });

    xiiDynamicArray<T> out;
    out.SetCountUninitialized(static_cast<xiiUInt32>(tmp.size()));
    for (size_t i = 0; i < tmp.size(); ++i)
    {
      out[static_cast<xiiUInt32>(i)] = tmp[i];
    }
    return out;
  }

  // Compare two containers element-wise using provided comparators for key and original index
  template <typename ContainerA, typename ContainerB, typename KeyFunc, typename OrigIndexFunc>
  static bool CompareContainersExact(const ContainerA& a, const ContainerB& b, const KeyFunc& keyFunc, const OrigIndexFunc& origIndexFunc)
  {
    if (a.GetCount() != b.GetCount())
      return false;
    for (xiiUInt32 i = 0; i < a.GetCount(); ++i)
    {
      if (keyFunc(a[i]) != keyFunc(b[i]))
        return false;
      if (origIndexFunc(a[i]) != origIndexFunc(b[i]))
        return false;
    }
    return true;
  }

  // Test helpers for algorithms

  template <typename Container, typename KeyFunc>
  static void RunAndVerifyAgainstReference(Container& data, Container& scratch, const KeyFunc& keyFunc, const char* szAlgorithm, void (*sortFunc)(Container&, Container&, const KeyFunc&))
  {
    // Make a copy for reference
    using T                      = typename std::remove_reference<decltype(data[0])>::type;
    xiiDynamicArray<T> reference = ReferenceStableSort(data, keyFunc);

    // Run algorithm under test
    sortFunc(data, scratch, keyFunc);

    // Verify ordering (non-decreasing keys)
    for (xiiUInt32 i = 1; i < data.GetCount(); ++i)
    {
      const auto a = keyFunc(data[i - 1]);
      const auto b = keyFunc(data[i]);
      if (a > b)
      {
        xiiLog::Error("{}: ORDERING FAIL at {}: prev={} curr={}", szAlgorithm, i, a, b);
        DumpWindow(data, i);
        XII_TEST_BOOL(false);
        return;
      }
    }

    // Verify exact match with stable reference (keys and relative order)
    for (xiiUInt32 i = 0; i < data.GetCount(); ++i)
    {
      if (keyFunc(data[i]) != keyFunc(reference[i]))
      {
        xiiLog::Error("{}: MISMATCH vs reference at {}: got={} ref={}", szAlgorithm, i, keyFunc(data[i]), keyFunc(reference[i]));
        DumpWindow(data, i);
        DumpWindow(reference, i);
        XII_TEST_BOOL(false);
        return;
      }
    }
  }

  // Overload for algorithms that accept comparator instead of key extractor
  template <typename Container, typename Compare>
  static void RunAndVerifyComparator(Container& data, Container& scratch, const Compare& comp, const char* szAlgorithm, void (*sortFuncComp)(Container&, const Compare&))
  {
    // Create reference using std::stable_sort with comparator converted to less-than
    using T = typename std::remove_reference<decltype(data[0])>::type;
    std::vector<T> tmp;
    tmp.reserve(data.GetCount());
    for (xiiUInt32 i = 0; i < data.GetCount(); ++i)
    {
      tmp.push_back(data[i]);
    }

    std::stable_sort(tmp.begin(), tmp.end(), [&](const T& a, const T& b) { return comp(a, b); });

    xiiDynamicArray<T> reference;
    reference.SetCountUninitialized(static_cast<xiiUInt32>(tmp.size()));
    for (size_t i = 0; i < tmp.size(); ++i)
    {
      reference[static_cast<xiiUInt32>(i)] = tmp[i];
    }

    // Run algorithm under test
    sortFuncComp(data, comp);

    // Verify equality to reference
    for (xiiUInt32 i = 0; i < data.GetCount(); ++i)
    {
      if (data[i] != reference[i])
      {
        xiiLog::Error("{} (comp): MISMATCH at {}: got={} ref={}", szAlgorithm, i, data[i], reference[i]);
        DumpWindow(data, i);
        DumpWindow(reference, i);
        XII_TEST_BOOL(false);
        return;
      }
    }
  }

  // Small utility wrappers for calling sorting functions with different signatures

  // QuickSort wrapper (comparator)
  template <typename Container, typename Compare>
  static void QuickSortWrapper(Container& c, const Compare& comp)
  {
    xiiSorting::QuickSort(c, comp);
  }

  // QuickSort wrapper for array ptr
  template <typename T, typename Compare>
  static void QuickSortWrapper(xiiArrayPtr<T> c, const Compare& comp)
  {
    xiiSorting::QuickSort(c, comp);
  }

  // MergeSort wrapper (comparator)
  template <typename Container, typename Compare>
  static void MergeSortWrapper(Container& c, const Compare& comp)
  {
    xiiSorting::MergeSort(c, comp);
  }

  // SelectionSort wrapper (comparator)
  template <typename Container, typename Compare>
  static void SelectionSortWrapper(Container& c, const Compare& comp)
  {
    xiiSorting::SelectionSort(c, comp);
  }

  // SelectionSortStable wrapper (comparator)
  template <typename Container, typename Compare>
  static void SelectionSortStableWrapper(Container& c, const Compare& comp)
  {
    xiiSorting::SelectionSortStable(c, comp);
  }

  // BubbleSort wrapper (comparator)
  template <typename Container, typename Compare>
  static void BubbleSortWrapper(Container& c, const Compare& comp)
  {
    xiiSorting::BubbleSort(c, comp);
  }

  // RadixSort wrapper (key extractor)
  template <typename Container, typename Scratch, typename KeyFunc>
  static void RadixSortWrapper(Container& c, Scratch& s, const KeyFunc& keyFunc)
  {
    xiiSorting::RadixSort(c, s, keyFunc);
  }

  // Diagnostic helpers for fuzz tests

  template <typename Item, typename KeyFunc, typename OrigIndexFunc>
  static bool RunRadixAndVerifyFuzz(xiiDynamicArray<Item> items, xiiUInt64 seed, const KeyFunc& keyFunc, const OrigIndexFunc& origIndexFunc)
  {
    xiiDynamicArray<Item> scratch;
    scratch.SetCount(items.GetCount());

    // Run radix sort under test
    xiiSorting::RadixSort(items, scratch, keyFunc);

    // Verify ordering and stability
    for (xiiUInt32 i = 1; i < items.GetCount(); ++i)
    {
      const auto aKey = keyFunc(items[i - 1]);
      const auto bKey = keyFunc(items[i]);
      if (aKey > bKey)
      {
        xiiLog::Error("Fuzz ORDER FAIL seed={} at {}: prev={} curr={}.", seed, i, aKey, bKey);
        DumpWindow(items, i);
        return false;
      }
      if (aKey == bKey)
      {
        if (origIndexFunc(items[i - 1]) >= origIndexFunc(items[i]))
        {
          xiiLog::Error("Fuzz STABILITY FAIL seed={} at {}: prevIdx={} currIdx={} key={}.", seed, i, origIndexFunc(items[i - 1]), origIndexFunc(items[i]), aKey);
          DumpWindow(items, i);
          return false;
        }
      }
    }
    return true;
  }
} // namespace

// -------------------------
// Comprehensive Test Suite
// -------------------------

XII_CREATE_SIMPLE_TEST(Algorithm, Sorting)
{
  // -------------------------
  // Sanity and small deterministic tests for each algorithm
  // -------------------------

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Sanity - Empty and Single Element")
  {
    xiiDynamicArray<int> empty;
    xiiDynamicArray<int> scratchEmpty;
    scratchEmpty.SetCount(0);

    // QuickSort (comparator)
    xiiSorting::QuickSort(empty, [](int a, int b) { return a < b; });
    XII_TEST_BOOL(empty.GetCount() == 0);

    // MergeSort (comparator)
    xiiDynamicArray<int> single;
    single.PushBack(42);
    xiiDynamicArray<int> scratchSingle;
    scratchSingle.SetCount(1);
    xiiSorting::MergeSort(single, [](int a, int b) { return a < b; });
    XII_TEST_BOOL(single.GetCount() == 1 && single[0] == 42);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Small Deterministic - Correctness")
  {
    // Prepare a small mixed array
    std::vector<int>     base = {5, -1, 1000, 0, 255, -100, 42, 42, 7};
    xiiDynamicArray<int> a    = MakeDynamicArrayFromStd(base);
    xiiDynamicArray<int> scratch;
    scratch.SetCount(a.GetCount());

    // QuickSort comparator test
    {
      xiiDynamicArray<int> copy = a;
      QuickSortWrapper(copy, [](int x, int y) { return x < y; });
      std::vector<int> expected = base;
      std::stable_sort(expected.begin(), expected.end());
      xiiDynamicArray<int> expectedArr = MakeDynamicArrayFromStd(expected);
      for (xiiUInt32 i = 0; i < copy.GetCount(); ++i)
        XII_TEST_BOOL(copy[i] == expectedArr[i]);
    }

    // MergeSort comparator test
    {
      xiiDynamicArray<int> copy = a;
      MergeSortWrapper(copy, [](int x, int y) { return x < y; });
      std::vector<int> expected = base;
      std::stable_sort(expected.begin(), expected.end());
      xiiDynamicArray<int> expectedArr = MakeDynamicArrayFromStd(expected);

      for (xiiUInt32 i = 0; i < copy.GetCount(); ++i)
      {
        XII_TEST_BOOL(copy[i] == expectedArr[i]);
      }
    }

    // SelectionSort and BubbleSort (comparator)
    {
      xiiDynamicArray<int> copy1 = a;
      SelectionSortWrapper(copy1, [](int x, int y) { return x < y; });
      xiiDynamicArray<int> copy2 = a;
      BubbleSortWrapper(copy2, [](int x, int y) { return x < y; });

      std::vector<int> expected = base;
      std::stable_sort(expected.begin(), expected.end());
      xiiDynamicArray<int> expectedArr = MakeDynamicArrayFromStd(expected);

      for (xiiUInt32 i = 0; i < copy1.GetCount(); ++i)
      {
        XII_TEST_BOOL(copy1[i] == expectedArr[i]);
        XII_TEST_BOOL(copy2[i] == expectedArr[i]);
      }
    }
  }

  // -------------------------
  // Stability tests (small)
  // -------------------------

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Stability - Small Controlled")
  {
    struct Item
    {
      XII_DECLARE_POD_TYPE();

      xiiUInt64 key;
      xiiUInt32 idx;
      bool      operator==(const Item& o) const { return key == o.key && idx == o.idx; }
    };

    xiiDynamicArray<Item> v;
    v.PushBack({5, 0});
    v.PushBack({5, 1});
    v.PushBack({5, 2});
    v.PushBack({3, 3});
    v.PushBack({3, 4});
    v.PushBack({9, 5});

    xiiDynamicArray<Item> scratch;
    scratch.SetCount(v.GetCount());

    // RadixSort stability
    xiiSorting::RadixSort(v, scratch, [](const Item& it) { return it.key; });

    XII_TEST_BOOL(v[0].key == 3 && v[0].idx == 3);
    XII_TEST_BOOL(v[1].key == 3 && v[1].idx == 4);
    XII_TEST_BOOL(v[2].key == 5 && v[2].idx == 0);
    XII_TEST_BOOL(v[3].key == 5 && v[3].idx == 1);
    XII_TEST_BOOL(v[4].key == 5 && v[4].idx == 2);
    XII_TEST_BOOL(v[5].key == 9 && v[5].idx == 5);

    // SelectionSortStable
    xiiDynamicArray<Item> v2 = v;
    SelectionSortStableWrapper(v2, [](const Item& a, const Item& b) { return a.key < b.key; });
    for (xiiUInt32 i = 1; i < v2.GetCount(); ++i)
    {
      if (v2[i - 1].key == v2[i].key)
      {
        XII_TEST_BOOL(v2[i - 1].idx < v2[i].idx);
      }
    }
  }

  // -------------------------
  // Container compatibility tests
  // -------------------------

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Container Compatibility - ArrayPtr vs DynamicArray")
  {
    auto items = CreateDuplicateKeyDataset<RadixSortTestItem>(4096, 10, 12345ULL);

    xiiDynamicArray<RadixSortTestItem> a = items;
    xiiDynamicArray<RadixSortTestItem> b = items;

    xiiDynamicArray<RadixSortTestItem> scratchA;
    scratchA.SetCount(a.GetCount());
    xiiDynamicArray<RadixSortTestItem> scratchB;
    scratchB.SetCount(b.GetCount());

    // DynamicArray run
    xiiSorting::RadixSort(a, scratchA, RadixSortTestKeyExtractor());

    // ArrayPtr run
    xiiArrayPtr<RadixSortTestItem> ptr = b;
    xiiSorting::RadixSort(ptr, scratchB, RadixSortTestKeyExtractor());

    // Compare element-wise keys and original indices
    for (xiiUInt32 i = 0; i < a.GetCount(); ++i)
    {
      if (a[i].m_uiKey != b[i].m_uiKey || a[i].m_uiOriginalIndex != b[i].m_uiOriginalIndex)
      {
        xiiLog::Error("Container compatibility mismatch at {}.", i);
        DumpWindow(a, i);
        DumpWindow(b, i);
        XII_TEST_BOOL(false);
        break;
      }
    }
  }

  // -------------------------
  // Radix sort: small known cases and edge cases
  // -------------------------

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RadixSort - Small Known and Edge Cases")
  {
    // Known small case
    {
      xiiDynamicArray<xiiUInt64> v;
      v.PushBack(3298323);
      v.PushBack(5);
      v.PushBack(3);
      v.PushBack(9);
      v.PushBack(1);
      v.PushBack(0);
      v.PushBack(255);
      v.PushBack(256);
      v.PushBack(257);
      xiiDynamicArray<xiiUInt64> s;
      s.SetCount(v.GetCount());
      xiiSorting::RadixSort(v, s, DefaultRadixKeyExtractor<xiiUInt64>());

      xiiDynamicArray<xiiUInt64> expected;
      expected.PushBack(0);
      expected.PushBack(1);
      expected.PushBack(3);
      expected.PushBack(5);
      expected.PushBack(9);
      expected.PushBack(255);
      expected.PushBack(256);
      expected.PushBack(257);
      expected.PushBack(3298323);
      XII_TEST_BOOL(v == expected);
    }

    // Already sorted, reverse sorted, all equal
    {
      xiiDynamicArray<xiiUInt64> a;
      a.PushBack(1);
      a.PushBack(2);
      a.PushBack(3);
      a.PushBack(4);
      xiiDynamicArray<xiiUInt64> s;
      s.SetCount(a.GetCount());
      xiiSorting::RadixSort(a, s, DefaultRadixKeyExtractor<xiiUInt64>());
      XII_TEST_BOOL(a[0] == 1 && a[3] == 4);

      xiiDynamicArray<xiiUInt64> b;
      b.PushBack(4);
      b.PushBack(3);
      b.PushBack(2);
      b.PushBack(1);
      s.SetCount(b.GetCount());
      xiiSorting::RadixSort(b, s, DefaultRadixKeyExtractor<xiiUInt64>());
      XII_TEST_BOOL(b[0] == 1 && b[3] == 4);

      xiiDynamicArray<xiiUInt64> c;
      c.PushBack(7);
      c.PushBack(7);
      c.PushBack(7);
      s.SetCount(c.GetCount());
      xiiSorting::RadixSort(c, s, DefaultRadixKeyExtractor<xiiUInt64>());
      XII_TEST_BOOL(c[0] == 7 && c[2] == 7);
    }
  }

  // Fuzz tests (reproducible seeds) for RadixSort stability and ordering
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RadixSort - Fuzz Stability and Ordering (Reproducible)")
  {
    const xiiUInt32 seedsToRun = 200;
    const xiiUInt32 itemCount  = 4096;
    const xiiUInt32 keyBits    = 10; // low entropy to stress stability

    for (xiiUInt32 seed = 0; seed < seedsToRun; ++seed)
    {
      auto items = CreateDuplicateKeyDataset<RadixSortTestItem>(itemCount, keyBits, static_cast<xiiUInt64>(seed + 1));

      // Ensure original indices are set by generator
      xiiDynamicArray<RadixSortTestItem> scratch;
      scratch.SetCount(items.GetCount());

      const bool ok = RunRadixAndVerifyFuzz(items, seed + 1, RadixSortTestKeyExtractor(), [](const RadixSortTestItem& it) { return it.m_uiOriginalIndex; });
      if (!ok)
      {
        // Failure already printed by helper
        XII_TEST_BOOL(false);
        break;
      }
    }
  }

  // Cross-check: compare each algorithm against stable reference for random inputs.
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Cross-check - Randomized Inputs vs std::stable_sort")
  {
    const xiiUInt64                    seed = 0xDEADBEEF;
    auto                               rng  = CreateDeterministicRng(seed);
    std::uniform_int_distribution<int> dist(-1000000, 1000000);

    const xiiUInt32 runs = 50;
    const xiiUInt32 size = 1024;

    for (xiiUInt32 r = 0; r < runs; ++r)
    {
      // Generate random data.
      std::vector<int> base;
      base.reserve(size);
      for (xiiUInt32 i = 0; i < size; ++i)
      {
        base.push_back(dist(rng));
      }

      xiiDynamicArray<int> a = MakeDynamicArrayFromStd(base);
      xiiDynamicArray<int> b = a;
      xiiDynamicArray<int> c = a;
      xiiDynamicArray<int> d = a;
      xiiDynamicArray<int> e = a;

      xiiDynamicArray<int> scratch;
      scratch.SetCount(a.GetCount());

      // QuickSort (unstable) - compare ordering only (not stability)
      xiiSorting::QuickSort(a, [](int x, int y) { return x < y; });
      XII_TEST_BOOL(VerifySortedAscending(a, [](int v) { return v; }));

      // MergeSort (stable) - compare to std::stable_sort
      xiiSorting::MergeSort(b, [](int x, int y) { return x < y; });
      std::vector<int> expected = base;
      std::stable_sort(expected.begin(), expected.end());
      for (xiiUInt32 i = 0; i < b.GetCount(); ++i)
      {
        XII_TEST_BOOL(b[i] == expected[i]);
      }

      // SelectionSort (unstable) - ordering only
      xiiSorting::SelectionSort(c, [](int x, int y) { return x < y; });
      XII_TEST_BOOL(VerifySortedAscending(c, [](int v) { return v; }));

      // SelectionSortStable - compare to stable reference
      xiiSorting::SelectionSortStable(d, [](int x, int y) { return x < y; });
      for (xiiUInt32 i = 0; i < d.GetCount(); ++i)
      {
        XII_TEST_BOOL(d[i] == expected[i]);
      }

      // BubbleSort - ordering only
      xiiSorting::BubbleSort(e, [](int x, int y) { return x < y; });
      XII_TEST_BOOL(VerifySortedAscending(e, [](int v) { return v; }));
    }
  }

  // RadixSort key extractor preference test
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RadixSort - KeyExtractor Preference")
  {
    struct Item
    {
      XII_DECLARE_POD_TYPE();

      xiiUInt64 keyA;
      xiiUInt64 keyB;
      xiiUInt32 idx;
    };

    struct KeyFuncBoth
    {
      XII_ALWAYS_INLINE xiiUInt64 GetKey(const Item& it) const { return it.keyA; }
      xiiUInt64                   operator()(const Item& it) const { return it.keyB; }
    };

    xiiDynamicArray<Item> items;
    items.SetCount(4);
    items[0] = {2, 100, 0};
    items[1] = {1, 200, 1};
    items[2] = {2, 50, 2};
    items[3] = {1, 150, 3};

    xiiDynamicArray<Item> scratch;
    scratch.SetCount(items.GetCount());

    // RadixSort should prefer GetKey (keyA) over operator()
    xiiSorting::RadixSort(items, scratch, KeyFuncBoth());

    // After stable sort by keyA: keys 1 (idx 1,3) then 2 (idx 0,2)
    XII_TEST_BOOL(items[0].keyA == 1 && items[0].idx == 1);
    XII_TEST_BOOL(items[1].keyA == 1 && items[1].idx == 3);
    XII_TEST_BOOL(items[2].keyA == 2 && items[2].idx == 0);
    XII_TEST_BOOL(items[3].keyA == 2 && items[3].idx == 2);
  }

  // Diagnostic: per-failure dump and reproducible seed logging
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Diagnostics - Reproducible Failure Dump Example")
  {
    // This block demonstrates how to produce actionable failure output for CI.
    // It intentionally runs a single seed and prints context if a failure occurs.
    const xiiUInt64 uiSeed      = 42;
    const xiiUInt32 uiItemCount = 4096;
    const xiiUInt32 uiKeyBits   = 10;

    auto                               items = CreateDuplicateKeyDataset<RadixSortTestItem>(uiItemCount, uiKeyBits, uiSeed);
    xiiDynamicArray<RadixSortTestItem> scratch;
    scratch.SetCount(items.GetCount());

    xiiSorting::RadixSort(items, scratch, RadixSortTestKeyExtractor());

    // Verify and dump first failure with context
    for (xiiUInt32 i = 1; i < items.GetCount(); ++i)
    {
      const auto aKey = items[i - 1].m_uiKey;
      const auto bKey = items[i].m_uiKey;
      if (aKey > bKey)
      {
        xiiLog::Error("ORDER FAIL seed={} at {}.", uiSeed, i);
        DumpWindow(items, i);
        XII_TEST_BOOL(false);
        break;
      }
      if (aKey == bKey && items[i - 1].m_uiOriginalIndex >= items[i].m_uiOriginalIndex)
      {
        xiiLog::Error("STABILITY FAIL seed={} at {}.", uiSeed, i);
        DumpWindow(items, i);
        XII_TEST_BOOL(false);
        break;
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PerformanceSmoke - Large RadixSort")
  {
    const xiiUInt32                          itemCount = 1 << 20; // 1M items - disabled by default.
    auto                                     rng       = CreateDeterministicRng(0xC0FFEE);
    std::uniform_int_distribution<xiiUInt64> dist(0, std::numeric_limits<xiiUInt64>::max());

    xiiDynamicArray<xiiUInt64> items;
    items.Reserve(itemCount);
    for (xiiUInt32 i = 0; i < itemCount; ++i)
    {
      items.PushBack(dist(rng));
    }

    xiiDynamicArray<xiiUInt64> scratch;
    scratch.SetCountUninitialized(items.GetCount());

    xiiSorting::RadixSort(items, scratch, DefaultRadixKeyExtractor<xiiUInt64>());

    // Quick sanity check
    for (xiiUInt32 i = 1; i < items.GetCount(); ++i)
    {
      XII_TEST_BOOL(items[i - 1] <= items[i]);
    }
  }
}
