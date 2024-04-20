#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Map.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>
#include <algorithm>
#include <iterator>

XII_CREATE_SIMPLE_TEST(Containers, Map)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Iterator")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;
    for (xiiUInt32 i = 0; i < 1000; ++i)
      m[i] = i + 1;

    // XII_TEST_INT(std::find(begin(m), end(m), 500).Key(), 499);

    auto itfound = std::find_if(begin(m), end(m), [](xiiMap<xiiUInt32, xiiUInt32>::ConstIterator val) { return val.Value() == 500; });

    // XII_TEST_BOOL(std::find(begin(m), end(m), 500) == itfound);

    xiiUInt32 prev = begin(m).Key();
    for (auto it : m)
    {
      XII_TEST_BOOL(it.Value() >= prev);
      prev = it.Value();
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiMap<xiiUInt32, xiiUInt32>                           m;
    xiiMap<xiiConstructionCounter, xiiUInt32>              m2;
    xiiMap<xiiConstructionCounter, xiiConstructionCounter> m3;
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEmpty")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;
    XII_TEST_BOOL(m.IsEmpty());

    m[1] = 2;
    XII_TEST_BOOL(!m.IsEmpty());

    m.Clear();
    XII_TEST_BOOL(m.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetCount")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;
    XII_TEST_INT(m.GetCount(), 0);

    m[0] = 1;
    XII_TEST_INT(m.GetCount(), 1);

    m[1] = 2;
    XII_TEST_INT(m.GetCount(), 2);

    m[2] = 3;
    XII_TEST_INT(m.GetCount(), 3);

    m[0] = 1;
    XII_TEST_INT(m.GetCount(), 3);

    m.Clear();
    XII_TEST_INT(m.GetCount(), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear")
  {
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    {
      xiiMap<xiiUInt32, xiiConstructionCounter> m1;
      m1[0] = xiiConstructionCounter(1);
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(3, 2)); // for inserting new elements 2 temporaries are created (and destroyed)

      m1[1] = xiiConstructionCounter(3);
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(3, 2)); // for inserting new elements 2 temporaries are created (and destroyed)

      m1[0] = xiiConstructionCounter(2);
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 2));
      XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());
    }

    {
      xiiMap<xiiConstructionCounter, xiiUInt32> m1;
      m1[xiiConstructionCounter(0)] = 1;
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 1)); // one temporary

      m1[xiiConstructionCounter(1)] = 3;
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 1)); // one temporary

      m1[xiiConstructionCounter(0)] = 2;
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 2));
      XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    XII_TEST_BOOL(m.GetHeapMemoryUsage() == 0);

    XII_TEST_BOOL(m.Insert(1, 10).IsValid());
    XII_TEST_BOOL(m.Insert(1, 10).IsValid());
    m.Insert(3, 30);
    auto it7 = m.Insert(7, 70);
    m.Insert(9, 90);
    m.Insert(4, 40);
    m.Insert(2, 20);
    m.Insert(8, 80);
    m.Insert(5, 50);
    m.Insert(6, 60);

    XII_TEST_BOOL(m.Insert(7, 70).Value() == 70);
    XII_TEST_BOOL(m.Insert(7, 70) == it7);

    XII_TEST_BOOL(m.GetHeapMemoryUsage() >= sizeof(xiiUInt32) * 2 * 9);

    XII_TEST_INT(m[1], 10);
    XII_TEST_INT(m[2], 20);
    XII_TEST_INT(m[3], 30);
    XII_TEST_INT(m[4], 40);
    XII_TEST_INT(m[5], 50);
    XII_TEST_INT(m[6], 60);
    XII_TEST_INT(m[7], 70);
    XII_TEST_INT(m[8], 80);
    XII_TEST_INT(m[9], 90);

    XII_TEST_INT(m.GetCount(), 9);

    for (xiiUInt32 i = 0; i < 1000000; ++i)
      m[i] = i;

    XII_TEST_BOOL(m.GetHeapMemoryUsage() >= sizeof(xiiUInt32) * 2 * 1000000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Find")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (xiiInt32 i = 1000 - 1; i >= 0; --i)
      XII_TEST_INT(m.Find(i).Value(), i * 10);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetValue/TryGetValue")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 100; ++i)
      m[i] = i * 10;

    for (xiiInt32 i = 100 - 1; i >= 0; --i)
    {
      XII_TEST_INT(*m.GetValue(i), i * 10);

      xiiUInt32 v = 0;
      XII_TEST_BOOL(m.TryGetValue(i, v));
      XII_TEST_INT(v, i * 10);

      xiiUInt32* pV = nullptr;
      XII_TEST_BOOL(m.TryGetValue(i, pV));
      XII_TEST_INT(*pV, i * 10);
    }

    XII_TEST_BOOL(m.GetValue(101) == nullptr);

    xiiUInt32 v = 0;
    XII_TEST_BOOL(m.TryGetValue(101, v) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetValue/TryGetValue (const)")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 100; ++i)
      m[i] = i * 10;

    const xiiMap<xiiUInt32, xiiUInt32>& mConst = m;

    for (xiiInt32 i = 100 - 1; i >= 0; --i)
    {
      XII_TEST_INT(*mConst.GetValue(i), i * 10);

      xiiUInt32 v = 0;
      XII_TEST_BOOL(m.TryGetValue(i, v));
      XII_TEST_INT(v, i * 10);

      xiiUInt32* pV = nullptr;
      XII_TEST_BOOL(m.TryGetValue(i, pV));
      XII_TEST_INT(*pV, i * 10);
    }

    XII_TEST_BOOL(mConst.GetValue(101) == nullptr);

    xiiUInt32 v = 0;
    XII_TEST_BOOL(mConst.TryGetValue(101, v) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetValueOrDefault")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 100; ++i)
      m[i] = i * 10;

    for (xiiInt32 i = 100 - 1; i >= 0; --i)
      XII_TEST_INT(m.GetValueOrDefault(i, 999), i * 10);

    XII_TEST_BOOL(m.GetValueOrDefault(101, 999) == 999);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; i += 2)
      m[i] = i * 10;

    for (xiiInt32 i = 0; i < 1000; i += 2)
    {
      XII_TEST_BOOL(m.Contains(i));
      XII_TEST_BOOL(!m.Contains(i + 1));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FindOrAdd")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
    {
      bool bExisted                     = true;
      m.FindOrAdd(i, &bExisted).Value() = i * 10;
      XII_TEST_BOOL(!bExisted);
    }

    for (xiiInt32 i = 1000 - 1; i >= 0; --i)
    {
      bool bExisted = false;
      XII_TEST_INT(m.FindOrAdd(i, &bExisted).Value(), i * 10);
      XII_TEST_BOOL(bExisted);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator[]")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (xiiInt32 i = 1000 - 1; i >= 0; --i)
      XII_TEST_INT(m[i], i * 10);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove (non-existing)")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
    {
      XII_TEST_BOOL(!m.Remove(i));
    }

    for (xiiInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (xiiInt32 i = 0; i < 1000; ++i)
    {
      XII_TEST_BOOL(m.Remove(i + 500) == (i < 500));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove (Iterator)")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (xiiInt32 i = 0; i < 1000 - 1; ++i)
    {
      xiiMap<xiiUInt32, xiiUInt32>::Iterator itNext = m.Remove(m.Find(i));
      XII_TEST_BOOL(!m.Find(i).IsValid());
      XII_TEST_BOOL(itNext.Key() == i + 1);

      XII_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove (Key)")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (xiiInt32 i = 0; i < 1000; ++i)
    {
      XII_TEST_BOOL(m.Remove(i));
      XII_TEST_BOOL(!m.Find(i).IsValid());

      XII_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator=")
  {
    xiiMap<xiiUInt32, xiiUInt32> m, m2;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    m2 = m;

    for (xiiInt32 i = 1000 - 1; i >= 0; --i)
      XII_TEST_INT(m2[i], i * 10);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy Constructor")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    xiiMap<xiiUInt32, xiiUInt32> m2(m);

    for (xiiInt32 i = 1000 - 1; i >= 0; --i)
      XII_TEST_INT(m2[i], i * 10);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetIterator / Forward Iteration")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    xiiInt32 i = 0;
    for (xiiMap<xiiUInt32, xiiUInt32>::Iterator it = m.GetIterator(); it.IsValid(); ++it)
    {
      XII_TEST_INT(it.Key(), i);
      XII_TEST_INT(it.Value(), i * 10);
      ++i;
    }

    XII_TEST_INT(i, 1000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetIterator / Forward Iteration (const)")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    const xiiMap<xiiUInt32, xiiUInt32> m2(m);

    xiiInt32 i = 0;
    for (xiiMap<xiiUInt32, xiiUInt32>::ConstIterator it = m2.GetIterator(); it.IsValid(); ++it)
    {
      XII_TEST_INT(it.Key(), i);
      XII_TEST_INT(it.Value(), i * 10);
      ++i;
    }

    XII_TEST_INT(i, 1000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLastIterator / Backward Iteration")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    xiiInt32 i = 1000 - 1;
    for (xiiMap<xiiUInt32, xiiUInt32>::Iterator it = m.GetLastIterator(); it.IsValid(); --it)
    {
      XII_TEST_INT(it.Key(), i);
      XII_TEST_INT(it.Value(), i * 10);
      --i;
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLastIterator / Backward Iteration (const)")
  {
    xiiMap<xiiUInt32, xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    const xiiMap<xiiUInt32, xiiUInt32> m2(m);

    xiiInt32 i = 1000 - 1;
    for (xiiMap<xiiUInt32, xiiUInt32>::ConstIterator it = m2.GetLastIterator(); it.IsValid(); --it)
    {
      XII_TEST_INT(it.Key(), i);
      XII_TEST_INT(it.Value(), i * 10);
      --i;
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "LowerBound")
  {
    xiiMap<xiiInt32, xiiInt32> m, m2;

    m[0] = 0;
    m[3] = 30;
    m[7] = 70;
    m[9] = 90;

    XII_TEST_INT(m.LowerBound(-1).Key(), 0);
    XII_TEST_INT(m.LowerBound(0).Key(), 0);
    XII_TEST_INT(m.LowerBound(1).Key(), 3);
    XII_TEST_INT(m.LowerBound(2).Key(), 3);
    XII_TEST_INT(m.LowerBound(3).Key(), 3);
    XII_TEST_INT(m.LowerBound(4).Key(), 7);
    XII_TEST_INT(m.LowerBound(5).Key(), 7);
    XII_TEST_INT(m.LowerBound(6).Key(), 7);
    XII_TEST_INT(m.LowerBound(7).Key(), 7);
    XII_TEST_INT(m.LowerBound(8).Key(), 9);
    XII_TEST_INT(m.LowerBound(9).Key(), 9);

    XII_TEST_BOOL(!m.LowerBound(10).IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "UpperBound")
  {
    xiiMap<xiiInt32, xiiInt32> m, m2;

    m[0] = 0;
    m[3] = 30;
    m[7] = 70;
    m[9] = 90;

    XII_TEST_INT(m.UpperBound(-1).Key(), 0);
    XII_TEST_INT(m.UpperBound(0).Key(), 3);
    XII_TEST_INT(m.UpperBound(1).Key(), 3);
    XII_TEST_INT(m.UpperBound(2).Key(), 3);
    XII_TEST_INT(m.UpperBound(3).Key(), 7);
    XII_TEST_INT(m.UpperBound(4).Key(), 7);
    XII_TEST_INT(m.UpperBound(5).Key(), 7);
    XII_TEST_INT(m.UpperBound(6).Key(), 7);
    XII_TEST_INT(m.UpperBound(7).Key(), 9);
    XII_TEST_INT(m.UpperBound(8).Key(), 9);
    XII_TEST_BOOL(!m.UpperBound(9).IsValid());
    XII_TEST_BOOL(!m.UpperBound(10).IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert / Remove")
  {
    // Tests whether reusing of elements makes problems

    xiiMap<xiiInt32, xiiInt32> m;

    for (xiiUInt32 r = 0; r < 5; ++r)
    {
      // Insert
      for (xiiUInt32 i = 0; i < 10000; ++i)
        m.Insert(i, i * 10);

      XII_TEST_INT(m.GetCount(), 10000);

      // Remove
      for (xiiUInt32 i = 0; i < 5000; ++i)
        XII_TEST_BOOL(m.Remove(i));

      // Insert others
      for (xiiUInt32 j = 1; j < 1000; ++j)
        m.Insert(20000 * j, j);

      // Remove
      for (xiiUInt32 i = 0; i < 5000; ++i)
        XII_TEST_BOOL(m.Remove(5000 + i));

      // Remove others
      for (xiiUInt32 j = 1; j < 1000; ++j)
      {
        XII_TEST_BOOL(m.Find(20000 * j).IsValid());
        XII_TEST_BOOL(m.Remove(20000 * j));
      }
    }

    XII_TEST_BOOL(m.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator == / !=")
  {
    xiiMap<xiiUInt32, xiiUInt32> m, m2;

    XII_TEST_BOOL(m == m2);

    for (xiiInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    XII_TEST_BOOL(m != m2);

    m2 = m;

    XII_TEST_BOOL(m == m2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompatibleKeyType")
  {
    {
      xiiMap<xiiString, int> stringTable;
      const char*            szChar   = "Char";
      const char*            szString = "ViewBla";
      xiiStringView          sView(szString, szString + 4);
      xiiStringBuilder       sBuilder("Builder");
      xiiString              sString("String");
      stringTable.Insert(szChar, 1);
      stringTable.Insert(sView, 2);
      stringTable.Insert(sBuilder, 3);
      stringTable.Insert(sString, 4);

      XII_TEST_BOOL(stringTable.Contains(szChar));
      XII_TEST_BOOL(stringTable.Contains(sView));
      XII_TEST_BOOL(stringTable.Contains(sBuilder));
      XII_TEST_BOOL(stringTable.Contains(sString));

      XII_TEST_INT(*stringTable.GetValue(szChar), 1);
      XII_TEST_INT(*stringTable.GetValue(sView), 2);
      XII_TEST_INT(*stringTable.GetValue(sBuilder), 3);
      XII_TEST_INT(*stringTable.GetValue(sString), 4);

      XII_TEST_BOOL(stringTable.Remove(szChar));
      XII_TEST_BOOL(stringTable.Remove(sView));
      XII_TEST_BOOL(stringTable.Remove(sBuilder));
      XII_TEST_BOOL(stringTable.Remove(sString));
    }

    // dynamic array as key, check for allocations in comparisons
    {
      xiiProxyAllocator        testAllocator("Test", xiiFoundation::GetDefaultAllocator());
      xiiLocalAllocatorWrapper allocWrapper(&testAllocator);
      using TestDynArray = xiiDynamicArray<int, xiiLocalAllocatorWrapper>;
      TestDynArray a;
      TestDynArray b;
      for (int i = 0; i < 10; ++i)
      {
        a.PushBack(i);
        b.PushBack(i * 2);
      }

      xiiMap<TestDynArray, int> arrayTable;
      arrayTable.Insert(a, 1);
      arrayTable.Insert(b, 2);

      xiiArrayPtr<const int> aPtr = a.GetArrayPtr();
      xiiArrayPtr<const int> bPtr = b.GetArrayPtr();

      xiiUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

      bool existed;
      auto it = arrayTable.FindOrAdd(aPtr, &existed);
      XII_TEST_BOOL(existed);

      XII_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

      XII_TEST_BOOL(arrayTable.Contains(aPtr));
      XII_TEST_BOOL(arrayTable.Contains(bPtr));
      XII_TEST_BOOL(arrayTable.Contains(a));

      XII_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

      XII_TEST_INT(*arrayTable.GetValue(aPtr), 1);
      XII_TEST_INT(*arrayTable.GetValue(bPtr), 2);
      XII_TEST_INT(*arrayTable.GetValue(a), 1);

      XII_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

      XII_TEST_BOOL(arrayTable.Remove(aPtr));
      XII_TEST_BOOL(arrayTable.Remove(bPtr));

      XII_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swap")
  {
    xiiStringBuilder            tmp;
    xiiMap<xiiString, xiiInt32> map1;
    xiiMap<xiiString, xiiInt32> map2;

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      map1[tmp] = i;

      tmp.SetFormat("{0}{0}{0}", i);
      map2[tmp] = i;
    }

    map1.Swap(map2);

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      XII_TEST_BOOL(map2.Contains(tmp));
      XII_TEST_INT(map2[tmp], i);

      tmp.SetFormat("{0}{0}{0}", i);
      XII_TEST_BOOL(map1.Contains(tmp));
      XII_TEST_INT(map1[tmp], i);
    }
  }

  constexpr xiiUInt32 uiMapSize = sizeof(xiiMap<xiiString, xiiInt32>);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swap")
  {
    xiiUInt8 map1Mem[uiMapSize];
    xiiUInt8 map2Mem[uiMapSize];
    xiiMemoryUtils::PatternFill(map1Mem, 0xCA, uiMapSize);
    xiiMemoryUtils::PatternFill(map2Mem, 0xCA, uiMapSize);

    xiiStringBuilder             tmp;
    xiiMap<xiiString, xiiInt32>* map1 = new (map1Mem)(xiiMap<xiiString, xiiInt32>);
    xiiMap<xiiString, xiiInt32>* map2 = new (map2Mem)(xiiMap<xiiString, xiiInt32>);

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      map1->Insert(tmp, i);

      tmp.SetFormat("{0}{0}{0}", i);
      map2->Insert(tmp, i);
    }

    map1->Swap(*map2);

    // test swapped elements
    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      XII_TEST_BOOL(map2->Contains(tmp));
      XII_TEST_INT((*map2)[tmp], i);

      tmp.SetFormat("{0}{0}{0}", i);
      XII_TEST_BOOL(map1->Contains(tmp));
      XII_TEST_INT((*map1)[tmp], i);
    }

    // test iterators after swap
    {
      for (auto it : *map1)
      {
        XII_TEST_BOOL(!map2->Contains(it.Key()));
      }

      for (auto it : *map2)
      {
        XII_TEST_BOOL(!map1->Contains(it.Key()));
      }
    }

    // due to a compiler bug in VS 2017, PatternFill cannot be called here, because it will move the memset BEFORE the destructor call!
    // seems to be fixed in VS 2019 though

    map1->~xiiMap<xiiString, xiiInt32>();
    // xiiMemoryUtils::PatternFill(map1Mem, 0xBA, uiSetSize);

    map2->~xiiMap<xiiString, xiiInt32>();
    xiiMemoryUtils::PatternFill(map2Mem, 0xBA, uiMapSize);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swap Empty")
  {
    xiiUInt8 map1Mem[uiMapSize];
    xiiUInt8 map2Mem[uiMapSize];
    xiiMemoryUtils::PatternFill(map1Mem, 0xCA, uiMapSize);
    xiiMemoryUtils::PatternFill(map2Mem, 0xCA, uiMapSize);

    xiiStringBuilder             tmp;
    xiiMap<xiiString, xiiInt32>* map1 = new (map1Mem)(xiiMap<xiiString, xiiInt32>);
    xiiMap<xiiString, xiiInt32>* map2 = new (map2Mem)(xiiMap<xiiString, xiiInt32>);

    for (xiiUInt32 i = 0; i < 100; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      map1->Insert(tmp, i);
    }

    map1->Swap(*map2);
    XII_TEST_BOOL(map1->IsEmpty());

    map1->~xiiMap<xiiString, xiiInt32>();
    xiiMemoryUtils::PatternFill(map1Mem, 0xBA, uiMapSize);

    // test swapped elements
    for (xiiUInt32 i = 0; i < 100; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      XII_TEST_BOOL(map2->Contains(tmp));
    }

    // test iterators after swap
    {
      for (auto it : *map2)
      {
        XII_TEST_BOOL(map2->Contains(it.Key()));
      }
    }

    map2->~xiiMap<xiiString, xiiInt32>();
    xiiMemoryUtils::PatternFill(map2Mem, 0xBA, uiMapSize);
  }
}
