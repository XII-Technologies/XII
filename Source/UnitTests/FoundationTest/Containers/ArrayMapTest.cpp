/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Stopwatch.h>

XII_CREATE_SIMPLE_TEST(Containers, ArrayMap)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Iterator")
  {
    xiiArrayMap<xiiUInt32, xiiUInt32> m;
    for (xiiUInt32 i = 0; i < 1000; ++i)
      m[i] = i + 1;

    // element with the given key (and such, value "key + 1")
    auto findable = m.Find(499u);

    // non-const
    {
      // findable
      auto itfound = std::find_if(begin(m), end(m), [](const xiiArrayMap<xiiUInt32, xiiUInt32>::Pair& val) { return val.value == 500; });
      XII_TEST_BOOL(findable == itfound->key);

      // unfindable
      itfound = std::find_if(begin(m), end(m), [](const xiiArrayMap<xiiUInt32, xiiUInt32>::Pair& val) { return val.value == 1001; });
      XII_TEST_BOOL(end(m) == itfound);
    }

    // const
    {
      // findable
      auto itfound = std::find_if(cbegin(m), cend(m), [](const xiiArrayMap<xiiUInt32, xiiUInt32>::Pair& val) { return val.value == 500; });
      XII_TEST_BOOL(findable == itfound->key);

      // unfindable
      itfound = std::find_if(cbegin(m), cend(m), [](const xiiArrayMap<xiiUInt32, xiiUInt32>::Pair& val) { return val.value == 1001; });
      XII_TEST_BOOL(cend(m) == itfound);
    }

    // non-const reverse
    {
      // findable
      auto itfound = std::find_if(rbegin(m), rend(m), [](const xiiArrayMap<xiiUInt32, xiiUInt32>::Pair& val) { return val.value == 500; });
      XII_TEST_BOOL(findable == itfound->key);

      // unfindable
      itfound = std::find_if(rbegin(m), rend(m), [](const xiiArrayMap<xiiUInt32, xiiUInt32>::Pair& val) { return val.value == 1001; });
      XII_TEST_BOOL(rend(m) == itfound);
    }

    // const reverse
    {
      // findable
      auto itfound = std::find_if(crbegin(m), crend(m), [](const xiiArrayMap<xiiUInt32, xiiUInt32>::Pair& val) { return val.value == 500; });
      XII_TEST_BOOL(findable == itfound->key);

      // unfindable
      itfound = std::find_if(crbegin(m), crend(m), [](const xiiArrayMap<xiiUInt32, xiiUInt32>::Pair& val) { return val.value == 1001; });
      XII_TEST_BOOL(crend(m) == itfound);
    }

    // forward
    xiiUInt32 prev = begin(m)->key;
    for (const auto& elem : m)
    {
      XII_TEST_BOOL(elem.value == prev + 1);
      prev = elem.value;
    }

    XII_TEST_BOOL(prev == 1000);

    // backward
    prev = (rbegin(m))->value + 1;
    for (auto it = rbegin(m); it < rend(m); ++it)
    {
      XII_TEST_BOOL(it->value == prev - 1);
      prev = it->value;
    }

    XII_TEST_BOOL(prev == 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetData with Iterators")
  {
    xiiArrayMap<xiiUInt32, xiiUInt32> m;
    for (xiiUInt32 i = 0; i < 1000; ++i)
      m[i] = i + 1;

    // element with the given key (and such, value "key + 1")
    auto findable = m.Find(499u);

    // check if modification of the keys via direct data access
    // keeps iterability and access via keys intact

    // modify
    auto& data = m.GetData();
    for (auto& p : data)
    {
      p.key += 1000;
    }

    // ...and test with new key
    XII_TEST_BOOL(m[findable + 1000] == 500);

    // and index...
    XII_TEST_BOOL(m.GetValue(499u) == 500);

    // and old key.
    XII_TEST_BOOL(m.Find(499u) == xiiInvalidIndex);

    // findable
    auto itfound = std::find_if(begin(m), end(m), [](const xiiArrayMap<xiiUInt32, xiiUInt32>::Pair& val) { return val.value == 500; });
    XII_TEST_BOOL((findable + 1000) == itfound->key);

    // unfindable
    itfound = std::find_if(begin(m), end(m), [](const xiiArrayMap<xiiUInt32, xiiUInt32>::Pair& val) { return val.value == 1001; });
    XII_TEST_BOOL(end(m) == itfound);

    // forward
    xiiUInt32 prev = 0;
    for (const auto& elem : m)
    {
      XII_TEST_BOOL(elem.value == prev + 1);
      prev = elem.value;
    }

    XII_TEST_BOOL(prev == 1000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert / Find / Reserve / Clear / IsEmpty / Compact / GetCount")
  {
    xiiArrayMap<xiiString, xiiInt32> sa;

    XII_TEST_BOOL(sa.GetHeapMemoryUsage() == 0);

    XII_TEST_INT(sa.GetCount(), 0);
    XII_TEST_BOOL(sa.IsEmpty());

    sa.Reserve(10);

    XII_TEST_BOOL(sa.GetHeapMemoryUsage() >= 10 * (sizeof(xiiString) + sizeof(xiiInt32)));

    sa.Insert("z", 0);
    sa.Insert("y", 1);
    sa.Insert("x", 2);
    sa.Insert("c", 3);
    sa.Insert("b", 4);
    sa.Insert("a", 5);

    XII_TEST_INT(sa.GetCount(), 6);
    XII_TEST_BOOL(!sa.IsEmpty());

    XII_TEST_INT(sa.Find("a"), 0);
    XII_TEST_INT(sa.Find("b"), 1);
    XII_TEST_INT(sa.Find("c"), 2);
    XII_TEST_INT(sa.Find("x"), 3);
    XII_TEST_INT(sa.Find("y"), 4);
    XII_TEST_INT(sa.Find("z"), 5);

    XII_TEST_INT(sa.GetPair(sa.Find("a")).value, 5);
    XII_TEST_INT(sa.GetPair(sa.Find("b")).value, 4);
    XII_TEST_INT(sa.GetPair(sa.Find("c")).value, 3);
    XII_TEST_INT(sa.GetPair(sa.Find("x")).value, 2);
    XII_TEST_INT(sa.GetPair(sa.Find("y")).value, 1);
    XII_TEST_INT(sa.GetPair(sa.Find("z")).value, 0);

    sa.Clear();
    XII_TEST_BOOL(sa.IsEmpty());

    XII_TEST_BOOL(sa.GetHeapMemoryUsage() > 0);
    sa.Compact();
    XII_TEST_BOOL(sa.GetHeapMemoryUsage() == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert / Find / = / == / != ")
  {
    xiiArrayMap<xiiInt32, xiiInt32> sa, sa2;

    sa.Insert(20, 0);
    sa.Insert(19, 1);
    sa.Insert(18, 2);
    sa.Insert(12, 3);
    sa.Insert(11, 4);

    sa2 = sa;

    XII_TEST_BOOL(sa == sa2);

    sa.Insert(10, 5);

    XII_TEST_BOOL(sa != sa2);

    XII_TEST_INT(sa.Find(10), 0);
    XII_TEST_INT(sa.Find(11), 1);
    XII_TEST_INT(sa.Find(12), 2);
    XII_TEST_INT(sa.Find(18), 3);
    XII_TEST_INT(sa.Find(19), 4);
    XII_TEST_INT(sa.Find(20), 5);

    sa2.Insert(10, 5);

    XII_TEST_BOOL(sa == sa2);

    XII_TEST_INT(sa.GetValue(sa.Find(10)), 5);
    XII_TEST_INT(sa.GetValue(sa.Find(11)), 4);
    XII_TEST_INT(sa.GetValue(sa.Find(12)), 3);
    XII_TEST_INT(sa.GetValue(sa.Find(18)), 2);
    XII_TEST_INT(sa.GetValue(sa.Find(19)), 1);
    XII_TEST_INT(sa.GetValue(sa.Find(20)), 0);

    XII_TEST_BOOL(sa == sa2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains")
  {
    xiiArrayMap<xiiString, xiiInt32> sa;

    XII_TEST_BOOL(!sa.Contains("a"));
    XII_TEST_BOOL(!sa.Contains("z"));

    sa.Insert("z", 0);
    sa.Insert("y", 1);
    sa.Insert("x", 2);

    XII_TEST_BOOL(!sa.Contains("a"));
    XII_TEST_BOOL(sa.Contains("z"));

    sa.Insert("c", 3);
    sa.Insert("b", 4);
    sa.Insert("a", 5);

    XII_TEST_BOOL(sa.Contains("a"));
    XII_TEST_BOOL(sa.Contains("z"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains")
  {
    xiiArrayMap<xiiString, xiiInt32> sa;

    XII_TEST_BOOL(!sa.Contains("a", 0));
    XII_TEST_BOOL(!sa.Contains("z", 0));

    sa.Insert("z", 0);
    sa.Insert("y", 1);
    sa.Insert("x", 2);

    XII_TEST_BOOL(!sa.Contains("a", 0));
    XII_TEST_BOOL(sa.Contains("z", 0));
    XII_TEST_BOOL(sa.Contains("y", 1));
    XII_TEST_BOOL(sa.Contains("x", 2));

    sa.Insert("c", 3);
    sa.Insert("b", 4);
    sa.Insert("a", 5);

    XII_TEST_BOOL(sa.Contains("a", 5));
    XII_TEST_BOOL(sa.Contains("b", 4));
    XII_TEST_BOOL(sa.Contains("c", 3));
    XII_TEST_BOOL(sa.Contains("z", 0));
    XII_TEST_BOOL(sa.Contains("y", 1));
    XII_TEST_BOOL(sa.Contains("x", 2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetValue / GetKey / Copy Constructor")
  {
    xiiArrayMap<xiiString, xiiInt32> sa;

    sa.Insert("z", 1);
    sa.Insert("y", 3);
    sa.Insert("x", 5);
    sa.Insert("c", 7);
    sa.Insert("b", 9);
    sa.Insert("a", 11);

    sa.Sort();

    const xiiArrayMap<xiiString, xiiInt32> sa2(sa);

    XII_TEST_INT(sa.GetValue(0), 11);
    XII_TEST_INT(sa.GetValue(2), 7);

    XII_TEST_INT(sa2.GetValue(0), 11);
    XII_TEST_INT(sa2.GetValue(2), 7);

    XII_TEST_STRING(sa.GetKey(1), "b");
    XII_TEST_STRING(sa.GetKey(3), "x");

    XII_TEST_INT(sa["b"], 9);
    XII_TEST_INT(sa["y"], 3);

    XII_TEST_INT(sa.GetPair(2).value, 7);
    XII_TEST_STRING(sa.GetPair(4).key, "y");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove")
  {
    xiiArrayMap<xiiString, xiiInt32> sa;

    bool bExisted = true;

    sa.FindOrAdd("a", &bExisted) = 2;
    XII_TEST_BOOL(!bExisted);

    sa.FindOrAdd("b", &bExisted) = 4;
    XII_TEST_BOOL(!bExisted);

    sa.FindOrAdd("c", &bExisted) = 6;
    XII_TEST_BOOL(!bExisted);

    sa.FindOrAdd("b", &bExisted) = 5;
    XII_TEST_BOOL(bExisted);

    XII_TEST_INT(sa.GetCount(), 3);

    XII_TEST_INT(sa.Find("a"), 0);
    XII_TEST_INT(sa.Find("c"), 2);

    sa.RemoveAndCopy("b");
    XII_TEST_INT(sa.GetCount(), 2);

    XII_TEST_INT(sa.Find("b"), xiiInvalidIndex);

    XII_TEST_INT(sa.Find("a"), 0);
    XII_TEST_INT(sa.Find("c"), 1);

    sa.RemoveAtAndCopy(1);
    XII_TEST_INT(sa.GetCount(), 1);

    XII_TEST_INT(sa.Find("a"), 0);
    XII_TEST_INT(sa.Find("c"), xiiInvalidIndex);

    sa.RemoveAtAndCopy(0);
    XII_TEST_INT(sa.GetCount(), 0);

    XII_TEST_INT(sa.Find("a"), xiiInvalidIndex);
    XII_TEST_INT(sa.Find("c"), xiiInvalidIndex);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Stresstest")
  {
    // Interestingly the map is not really slower than the sorted array, at least not in debug builds

    xiiStopwatch                    s;
    xiiArrayMap<xiiInt32, xiiInt32> sa;
    xiiMap<xiiInt32, xiiInt32>      map;

    const xiiInt32 uiElements = 100000;

    // const xiiTime t0 = s.Checkpoint();

    {
      sa.Reserve(uiElements);

      for (xiiUInt32 i = 0; i < uiElements; ++i)
      {
        sa.Insert(uiElements - i, i * 2);
      }

      sa.Sort();
    }

    // const xiiTime t1 = s.Checkpoint();

    {
      for (xiiInt32 i = 0; i < uiElements; ++i)
      {
        XII_TEST_INT(sa.GetValue(sa.Find(uiElements - i)), i * 2);
      }
    }

    // const xiiTime t2 = s.Checkpoint();

    {
      for (xiiUInt32 i = 0; i < uiElements; ++i)
      {
        map.Insert(uiElements - i, i * 2);
      }
    }

    // const xiiTime t3 = s.Checkpoint();

    {
      for (xiiUInt32 i = 0; i < uiElements; ++i)
      {
        XII_TEST_INT(map[uiElements - i], i * 2);
      }
    }

    // const xiiTime t4 = s.Checkpoint();

    // int breakpoint = 0;
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Lower Bound / Upper Bound")
  {
    xiiArrayMap<xiiInt32, xiiInt32> sa;
    sa[1]  = 23;
    sa[3]  = 23;
    sa[4]  = 23;
    sa[6]  = 23;
    sa[7]  = 23;
    sa[9]  = 23;
    sa[11] = 23;
    sa[14] = 23;
    sa[17] = 23;

    XII_TEST_INT(sa.LowerBound(0), 0);
    XII_TEST_INT(sa.LowerBound(1), 0);
    XII_TEST_INT(sa.LowerBound(2), 1);
    XII_TEST_INT(sa.LowerBound(3), 1);
    XII_TEST_INT(sa.LowerBound(4), 2);
    XII_TEST_INT(sa.LowerBound(5), 3);
    XII_TEST_INT(sa.LowerBound(6), 3);
    XII_TEST_INT(sa.LowerBound(7), 4);
    XII_TEST_INT(sa.LowerBound(8), 5);
    XII_TEST_INT(sa.LowerBound(9), 5);
    XII_TEST_INT(sa.LowerBound(10), 6);
    XII_TEST_INT(sa.LowerBound(11), 6);
    XII_TEST_INT(sa.LowerBound(12), 7);
    XII_TEST_INT(sa.LowerBound(13), 7);
    XII_TEST_INT(sa.LowerBound(14), 7);
    XII_TEST_INT(sa.LowerBound(15), 8);
    XII_TEST_INT(sa.LowerBound(16), 8);
    XII_TEST_INT(sa.LowerBound(17), 8);
    XII_TEST_INT(sa.LowerBound(18), xiiInvalidIndex);
    XII_TEST_INT(sa.LowerBound(19), xiiInvalidIndex);
    XII_TEST_INT(sa.LowerBound(20), xiiInvalidIndex);

    XII_TEST_INT(sa.UpperBound(0), 0);
    XII_TEST_INT(sa.UpperBound(1), 1);
    XII_TEST_INT(sa.UpperBound(2), 1);
    XII_TEST_INT(sa.UpperBound(3), 2);
    XII_TEST_INT(sa.UpperBound(4), 3);
    XII_TEST_INT(sa.UpperBound(5), 3);
    XII_TEST_INT(sa.UpperBound(6), 4);
    XII_TEST_INT(sa.UpperBound(7), 5);
    XII_TEST_INT(sa.UpperBound(8), 5);
    XII_TEST_INT(sa.UpperBound(9), 6);
    XII_TEST_INT(sa.UpperBound(10), 6);
    XII_TEST_INT(sa.UpperBound(11), 7);
    XII_TEST_INT(sa.UpperBound(12), 7);
    XII_TEST_INT(sa.UpperBound(13), 7);
    XII_TEST_INT(sa.UpperBound(14), 8);
    XII_TEST_INT(sa.UpperBound(15), 8);
    XII_TEST_INT(sa.UpperBound(16), 8);
    XII_TEST_INT(sa.UpperBound(17), xiiInvalidIndex);
    XII_TEST_INT(sa.UpperBound(18), xiiInvalidIndex);
    XII_TEST_INT(sa.UpperBound(19), xiiInvalidIndex);
    XII_TEST_INT(sa.UpperBound(20), xiiInvalidIndex);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Duplicate Keys")
  {
    xiiArrayMap<xiiInt32, xiiInt32> sa;

    sa.Insert(32, 1);
    sa.Insert(31, 1);
    sa.Insert(33, 1);

    sa.Insert(40, 1);
    sa.Insert(44, 1);
    sa.Insert(46, 1);

    sa.Insert(11, 1);
    sa.Insert(15, 1);
    sa.Insert(19, 1);

    sa.Insert(11, 2);
    sa.Insert(15, 2);
    sa.Insert(31, 2);
    sa.Insert(44, 2);

    sa.Insert(11, 3);
    sa.Insert(15, 3);
    sa.Insert(44, 3);

    sa.Insert(60, 1);
    sa.Insert(60, 2);
    sa.Insert(60, 3);
    sa.Insert(60, 4);
    sa.Insert(60, 5);
    sa.Insert(60, 6);
    sa.Insert(60, 7);
    sa.Insert(60, 8);
    sa.Insert(60, 9);
    sa.Insert(60, 10);

    sa.Sort();

    XII_TEST_INT(sa.LowerBound(11), 0);
    XII_TEST_INT(sa.LowerBound(15), 3);
    XII_TEST_INT(sa.LowerBound(19), 6);

    XII_TEST_INT(sa.LowerBound(31), 7);
    XII_TEST_INT(sa.LowerBound(32), 9);
    XII_TEST_INT(sa.LowerBound(33), 10);

    XII_TEST_INT(sa.LowerBound(40), 11);
    XII_TEST_INT(sa.LowerBound(44), 12);
    XII_TEST_INT(sa.LowerBound(46), 15);

    XII_TEST_INT(sa.LowerBound(60), 16);


    XII_TEST_INT(sa.UpperBound(11), 3);
    XII_TEST_INT(sa.UpperBound(15), 6);
    XII_TEST_INT(sa.UpperBound(19), 7);

    XII_TEST_INT(sa.UpperBound(31), 9);
    XII_TEST_INT(sa.UpperBound(32), 10);
    XII_TEST_INT(sa.UpperBound(33), 11);

    XII_TEST_INT(sa.UpperBound(40), 12);
    XII_TEST_INT(sa.UpperBound(44), 15);
    XII_TEST_INT(sa.UpperBound(46), 16);

    XII_TEST_INT(sa.UpperBound(60), xiiInvalidIndex);
  }
}
