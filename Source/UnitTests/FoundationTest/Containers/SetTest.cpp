/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Set.h>
#include <Foundation/Memory/CommonAllocators.h>

XII_CREATE_SIMPLE_TEST(Containers, Set)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiSet<xiiUInt32>                                      m;
    xiiSet<xiiConstructionCounter, xiiUInt32>              m2;
    xiiSet<xiiConstructionCounter, xiiConstructionCounter> m3;
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEmpty")
  {
    xiiSet<xiiUInt32> m;
    XII_TEST_BOOL(m.IsEmpty());

    m.Insert(1);
    XII_TEST_BOOL(!m.IsEmpty());

    m.Clear();
    XII_TEST_BOOL(m.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetCount")
  {
    xiiSet<xiiUInt32> m;
    XII_TEST_INT(m.GetCount(), 0);

    m.Insert(0);
    XII_TEST_INT(m.GetCount(), 1);

    m.Insert(1);
    XII_TEST_INT(m.GetCount(), 2);

    m.Insert(2);
    XII_TEST_INT(m.GetCount(), 3);

    m.Insert(1);
    XII_TEST_INT(m.GetCount(), 3);

    m.Clear();
    XII_TEST_INT(m.GetCount(), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear")
  {
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    {
      xiiSet<xiiConstructionCounter> m1;
      m1.Insert(xiiConstructionCounter(1));
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 1));

      m1.Insert(xiiConstructionCounter(3));
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 1));

      m1.Insert(xiiConstructionCounter(1));
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 2));
      XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());
    }

    {
      xiiSet<xiiConstructionCounter> m1;
      m1.Insert(xiiConstructionCounter(0));
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 1)); // one temporary

      m1.Insert(xiiConstructionCounter(1));
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 1)); // one temporary

      m1.Insert(xiiConstructionCounter(0));
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 2));
      XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert")
  {
    xiiSet<xiiUInt32> m;
    XII_TEST_BOOL(m.GetHeapMemoryUsage() == 0);

    XII_TEST_BOOL(m.Insert(1).IsValid());
    XII_TEST_BOOL(m.Insert(1).IsValid());

    m.Insert(3);
    auto it7 = m.Insert(7);
    m.Insert(9);
    m.Insert(4);
    m.Insert(2);
    m.Insert(8);
    m.Insert(5);
    m.Insert(6);

    XII_TEST_BOOL(m.Insert(1).Key() == 1);
    XII_TEST_BOOL(m.Insert(3).Key() == 3);
    XII_TEST_BOOL(m.Insert(7) == it7);

    XII_TEST_BOOL(m.GetHeapMemoryUsage() >= sizeof(xiiUInt32) * 1 * 9);

    XII_TEST_BOOL(m.Find(1).IsValid());
    XII_TEST_BOOL(m.Find(2).IsValid());
    XII_TEST_BOOL(m.Find(3).IsValid());
    XII_TEST_BOOL(m.Find(4).IsValid());
    XII_TEST_BOOL(m.Find(5).IsValid());
    XII_TEST_BOOL(m.Find(6).IsValid());
    XII_TEST_BOOL(m.Find(7).IsValid());
    XII_TEST_BOOL(m.Find(8).IsValid());
    XII_TEST_BOOL(m.Find(9).IsValid());

    XII_TEST_BOOL(!m.Find(0).IsValid());
    XII_TEST_BOOL(!m.Find(10).IsValid());

    XII_TEST_INT(m.GetCount(), 9);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains")
  {
    xiiSet<xiiUInt32> m;
    m.Insert(1);
    m.Insert(3);
    m.Insert(7);
    m.Insert(9);
    m.Insert(4);
    m.Insert(2);
    m.Insert(8);
    m.Insert(5);
    m.Insert(6);

    XII_TEST_BOOL(m.Contains(1));
    XII_TEST_BOOL(m.Contains(2));
    XII_TEST_BOOL(m.Contains(3));
    XII_TEST_BOOL(m.Contains(4));
    XII_TEST_BOOL(m.Contains(5));
    XII_TEST_BOOL(m.Contains(6));
    XII_TEST_BOOL(m.Contains(7));
    XII_TEST_BOOL(m.Contains(8));
    XII_TEST_BOOL(m.Contains(9));

    XII_TEST_BOOL(!m.Contains(0));
    XII_TEST_BOOL(!m.Contains(10));

    XII_TEST_INT(m.GetCount(), 9);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Set Operations")
  {
    xiiSet<xiiUInt32> base;
    base.Insert(1);
    base.Insert(3);
    base.Insert(5);

    xiiSet<xiiUInt32> empty;

    xiiSet<xiiUInt32> disjunct;
    disjunct.Insert(2);
    disjunct.Insert(4);
    disjunct.Insert(6);

    xiiSet<xiiUInt32> subSet;
    subSet.Insert(1);
    subSet.Insert(5);

    xiiSet<xiiUInt32> superSet;
    superSet.Insert(1);
    superSet.Insert(3);
    superSet.Insert(5);
    superSet.Insert(7);

    xiiSet<xiiUInt32> nonDisjunctNonEmptySubSet;
    nonDisjunctNonEmptySubSet.Insert(1);
    nonDisjunctNonEmptySubSet.Insert(4);
    nonDisjunctNonEmptySubSet.Insert(5);

    // ContainsSet
    XII_TEST_BOOL(base.ContainsSet(base));

    XII_TEST_BOOL(base.ContainsSet(empty));
    XII_TEST_BOOL(!empty.ContainsSet(base));

    XII_TEST_BOOL(!base.ContainsSet(disjunct));
    XII_TEST_BOOL(!disjunct.ContainsSet(base));

    XII_TEST_BOOL(base.ContainsSet(subSet));
    XII_TEST_BOOL(!subSet.ContainsSet(base));

    XII_TEST_BOOL(!base.ContainsSet(superSet));
    XII_TEST_BOOL(superSet.ContainsSet(base));

    XII_TEST_BOOL(!base.ContainsSet(nonDisjunctNonEmptySubSet));
    XII_TEST_BOOL(!nonDisjunctNonEmptySubSet.ContainsSet(base));

    // Union
    {
      xiiSet<xiiUInt32> res;

      res.Union(base);
      XII_TEST_BOOL(res.ContainsSet(base));
      XII_TEST_BOOL(base.ContainsSet(res));
      res.Union(subSet);
      XII_TEST_BOOL(res.ContainsSet(base));
      XII_TEST_BOOL(res.ContainsSet(subSet));
      XII_TEST_BOOL(base.ContainsSet(res));
      res.Union(superSet);
      XII_TEST_BOOL(res.ContainsSet(base));
      XII_TEST_BOOL(res.ContainsSet(subSet));
      XII_TEST_BOOL(res.ContainsSet(superSet));
      XII_TEST_BOOL(superSet.ContainsSet(res));
    }

    // Difference
    {
      xiiSet<xiiUInt32> res;
      res.Union(base);
      res.Difference(empty);
      XII_TEST_BOOL(res.ContainsSet(base));
      XII_TEST_BOOL(base.ContainsSet(res));
      res.Difference(disjunct);
      XII_TEST_BOOL(res.ContainsSet(base));
      XII_TEST_BOOL(base.ContainsSet(res));
      res.Difference(subSet);
      XII_TEST_INT(res.GetCount(), 1);
      XII_TEST_BOOL(res.Contains(3));
    }

    // Intersection
    {
      xiiSet<xiiUInt32> res;
      res.Union(base);
      res.Intersection(disjunct);
      XII_TEST_BOOL(res.IsEmpty());
      res.Union(base);
      res.Intersection(subSet);
      XII_TEST_BOOL(base.ContainsSet(subSet));
      XII_TEST_BOOL(res.ContainsSet(subSet));
      XII_TEST_BOOL(subSet.ContainsSet(res));
      res.Intersection(superSet);
      XII_TEST_BOOL(superSet.ContainsSet(res));
      XII_TEST_BOOL(res.ContainsSet(subSet));
      XII_TEST_BOOL(subSet.ContainsSet(res));
      res.Intersection(empty);
      XII_TEST_BOOL(res.IsEmpty());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Find")
  {
    xiiSet<xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (xiiInt32 i = 1000 - 1; i >= 0; --i)
      XII_TEST_INT(m.Find(i).Key(), i);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove (non-existing)")
  {
    xiiSet<xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      XII_TEST_BOOL(!m.Remove(i));

    for (xiiInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (xiiInt32 i = 0; i < 1000; ++i)
      XII_TEST_BOOL(m.Remove(i + 500) == (i < 500));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove (Iterator)")
  {
    xiiSet<xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (xiiInt32 i = 0; i < 1000 - 1; ++i)
    {
      xiiSet<xiiUInt32>::Iterator itNext = m.Remove(m.Find(i));
      XII_TEST_BOOL(!m.Find(i).IsValid());
      XII_TEST_BOOL(itNext.Key() == i + 1);

      XII_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove (Key)")
  {
    xiiSet<xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (xiiInt32 i = 0; i < 1000; ++i)
    {
      XII_TEST_BOOL(m.Remove(i));
      XII_TEST_BOOL(!m.Find(i).IsValid());

      XII_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator=")
  {
    xiiSet<xiiUInt32> m, m2;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    m2 = m;

    for (xiiInt32 i = 1000 - 1; i >= 0; --i)
      XII_TEST_BOOL(m2.Find(i).IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy Constructor")
  {
    xiiSet<xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    xiiSet<xiiUInt32> m2(m);

    for (xiiInt32 i = 1000 - 1; i >= 0; --i)
      XII_TEST_BOOL(m2.Find(i).IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetIterator / Forward Iteration")
  {
    xiiSet<xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    xiiInt32 i = 0;
    for (xiiSet<xiiUInt32>::Iterator it = m.GetIterator(); it.IsValid(); ++it)
    {
      XII_TEST_INT(it.Key(), i);
      ++i;
    }

    XII_TEST_INT(i, 1000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetIterator / Forward Iteration (const)")
  {
    xiiSet<xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    const xiiSet<xiiUInt32> m2(m);

    xiiInt32 i = 0;
    for (xiiSet<xiiUInt32>::Iterator it = m2.GetIterator(); it.IsValid(); ++it)
    {
      XII_TEST_INT(it.Key(), i);
      ++i;
    }

    XII_TEST_INT(i, 1000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "LowerBound")
  {
    xiiSet<xiiInt32> m, m2;

    m.Insert(0);
    m.Insert(3);
    m.Insert(7);
    m.Insert(9);

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
    xiiSet<xiiInt32> m, m2;

    m.Insert(0);
    m.Insert(3);
    m.Insert(7);
    m.Insert(9);

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

    xiiSet<xiiInt32> m;

    for (xiiUInt32 r = 0; r < 5; ++r)
    {
      // Insert
      for (xiiUInt32 i = 0; i < 10000; ++i)
        m.Insert(i);

      XII_TEST_INT(m.GetCount(), 10000);

      // Remove
      for (xiiUInt32 i = 0; i < 5000; ++i)
        XII_TEST_BOOL(m.Remove(i));

      // Insert others
      for (xiiUInt32 j = 1; j < 1000; ++j)
        m.Insert(20000 * j);

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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Iterator")
  {
    xiiSet<xiiUInt32> m;
    for (xiiUInt32 i = 0; i < 1000; ++i)
      m.Insert(i + 1);

    XII_TEST_INT(std::find(begin(m), end(m), 500).Key(), 500);

    auto itfound = std::find_if(begin(m), end(m), [](xiiUInt32 uiVal) { return uiVal == 500; });

    XII_TEST_BOOL(std::find(begin(m), end(m), 500) == itfound);

    xiiUInt32 prev = *begin(m);
    for (xiiUInt32 val : m)
    {
      XII_TEST_BOOL(val >= prev);
      prev = val;
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator == / !=")
  {
    xiiSet<xiiUInt32> m, m2;

    XII_TEST_BOOL(m == m2);

    for (xiiInt32 i = 0; i < 1000; ++i)
      m.Insert(i * 10);

    XII_TEST_BOOL(m != m2);

    m2 = m;

    XII_TEST_BOOL(m == m2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompatibleKeyType")
  {
    {
      xiiSet<xiiString> stringSet;
      const char*       szChar   = "Char";
      const char*       szString = "ViewBla";
      xiiStringView     sView(szString, szString + 4);
      xiiStringBuilder  sBuilder("Builder");
      xiiString         sString("String");
      stringSet.Insert(szChar);
      stringSet.Insert(sView);
      stringSet.Insert(sBuilder);
      stringSet.Insert(sString);

      XII_TEST_BOOL(stringSet.Contains(szChar));
      XII_TEST_BOOL(stringSet.Contains(sView));
      XII_TEST_BOOL(stringSet.Contains(sBuilder));
      XII_TEST_BOOL(stringSet.Contains(sString));

      XII_TEST_BOOL(stringSet.Remove(szChar));
      XII_TEST_BOOL(stringSet.Remove(sView));
      XII_TEST_BOOL(stringSet.Remove(sBuilder));
      XII_TEST_BOOL(stringSet.Remove(sString));
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

      xiiSet<TestDynArray> arraySet;
      arraySet.Insert(a);
      arraySet.Insert(b);

      xiiArrayPtr<const int> aPtr = a.GetArrayPtr();
      xiiArrayPtr<const int> bPtr = b.GetArrayPtr();

      xiiUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

      XII_TEST_BOOL(arraySet.Contains(aPtr));
      XII_TEST_BOOL(arraySet.Contains(bPtr));
      XII_TEST_BOOL(arraySet.Contains(a));

      XII_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

      XII_TEST_BOOL(arraySet.Remove(aPtr));
      XII_TEST_BOOL(arraySet.Remove(bPtr));

      XII_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
    }
  }

  constexpr xiiUInt32 uiSetSize = sizeof(xiiSet<xiiString>);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swap")
  {
    xiiUInt8 set1Mem[uiSetSize];
    xiiUInt8 set2Mem[uiSetSize];
    xiiMemoryUtils::PatternFill(set1Mem, 0xCA, uiSetSize);
    xiiMemoryUtils::PatternFill(set2Mem, 0xCA, uiSetSize);

    xiiStringBuilder   tmp;
    xiiSet<xiiString>* set1 = new (set1Mem)(xiiSet<xiiString>);
    xiiSet<xiiString>* set2 = new (set2Mem)(xiiSet<xiiString>);

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set1->Insert(tmp);

      tmp.SetFormat("{0}{0}{0}", i);
      set2->Insert(tmp);
    }

    set1->Swap(*set2);

    // test swapped elements
    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      XII_TEST_BOOL(set2->Contains(tmp));

      tmp.SetFormat("{0}{0}{0}", i);
      XII_TEST_BOOL(set1->Contains(tmp));
    }

    // test iterators after swap
    {
      for (const auto& element : *set1)
      {
        XII_TEST_BOOL(!set2->Contains(element));
      }

      for (const auto& element : *set2)
      {
        XII_TEST_BOOL(!set1->Contains(element));
      }
    }

    // due to a compiler bug in VS 2017, PatternFill cannot be called here, because it will move the memset BEFORE the destructor call!
    // seems to be fixed in VS 2019 though

    set1->~xiiSet<xiiString>();
    // xiiMemoryUtils::PatternFill(set1Mem, 0xBA, uiSetSize);

    set2->~xiiSet<xiiString>();
    xiiMemoryUtils::PatternFill(set2Mem, 0xBA, uiSetSize);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swap Empty")
  {
    xiiUInt8 set1Mem[uiSetSize];
    xiiUInt8 set2Mem[uiSetSize];
    xiiMemoryUtils::PatternFill(set1Mem, 0xCA, uiSetSize);
    xiiMemoryUtils::PatternFill(set2Mem, 0xCA, uiSetSize);

    xiiStringBuilder   tmp;
    xiiSet<xiiString>* set1 = new (set1Mem)(xiiSet<xiiString>);
    xiiSet<xiiString>* set2 = new (set2Mem)(xiiSet<xiiString>);

    for (xiiUInt32 i = 0; i < 100; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set1->Insert(tmp);
    }

    set1->Swap(*set2);
    XII_TEST_BOOL(set1->IsEmpty());

    set1->~xiiSet<xiiString>();
    xiiMemoryUtils::PatternFill(set1Mem, 0xBA, uiSetSize);

    // test swapped elements
    for (xiiUInt32 i = 0; i < 100; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      XII_TEST_BOOL(set2->Contains(tmp));
    }

    // test iterators after swap
    {
      for (const auto& element : *set2)
      {
        XII_TEST_BOOL(set2->Contains(element));
      }
    }

    set2->~xiiSet<xiiString>();
    xiiMemoryUtils::PatternFill(set2Mem, 0xBA, uiSetSize);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetReverseIterator")
  {
    xiiSet<xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    xiiInt32 i = 1000 - 1;
    for (xiiSet<xiiUInt32>::ReverseIterator it = m.GetReverseIterator(); it.IsValid(); ++it)
    {
      XII_TEST_INT(it.Key(), i);
      --i;
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetReverseIterator (const)")
  {
    xiiSet<xiiUInt32> m;

    for (xiiInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    const xiiSet<xiiUInt32> m2(m);

    xiiInt32 i = 1000 - 1;
    for (xiiSet<xiiUInt32>::ReverseIterator it = m2.GetReverseIterator(); it.IsValid(); ++it)
    {
      XII_TEST_INT(it.Key(), i);
      --i;
    }
  }
}
