/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Memory/CommonAllocators.h>

namespace
{
  using st = xiiConstructionCounter;

  struct Collision
  {
    xiiUInt32 hash;
    int       key;

    inline Collision(xiiUInt32 uiHash, int iKey)
    {
      this->hash = uiHash;
      this->key  = iKey;
    }

    inline bool operator==(const Collision& other) const { return key == other.key; }

    XII_DECLARE_POD_TYPE();
  };

  class OnlyMovable
  {
  public:
    OnlyMovable(xiiUInt32 uiHash) :
      hash(uiHash)
    {
    }
    OnlyMovable(OnlyMovable&& other) { *this = std::move(other); }

    void operator=(OnlyMovable&& other)
    {
      hash            = other.hash;
      m_NumTimesMoved = 0;
      ++other.m_NumTimesMoved;
    }

    bool operator==(const OnlyMovable& other) const { return hash == other.hash; }

    int       m_NumTimesMoved = 0;
    xiiUInt32 hash;

  private:
    OnlyMovable(const OnlyMovable&);
    void operator=(const OnlyMovable&);
  };
} // namespace

template <>
struct xiiHashHelper<Collision>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const Collision& value) { return value.hash; }

  XII_ALWAYS_INLINE static bool Equal(const Collision& a, const Collision& b) { return a == b; }
};

template <>
struct xiiHashHelper<OnlyMovable>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const OnlyMovable& value) { return value.hash; }

  XII_ALWAYS_INLINE static bool Equal(const OnlyMovable& a, const OnlyMovable& b) { return a.hash == b.hash; }
};

XII_CREATE_SIMPLE_TEST(Containers, HashSet)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiHashSet<xiiInt32> table1;

    XII_TEST_BOOL(table1.GetCount() == 0);
    XII_TEST_BOOL(table1.IsEmpty());

    xiiUInt32 counter = 0;
    for (auto it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ++counter;
    }
    XII_TEST_INT(counter, 0);

    XII_TEST_BOOL(begin(table1) == end(table1));
    XII_TEST_BOOL(cbegin(table1) == cend(table1));
    table1.Reserve(10);
    XII_TEST_BOOL(begin(table1) == end(table1));
    XII_TEST_BOOL(cbegin(table1) == cend(table1));

    for (auto value : table1)
    {
      ++counter;
    }
    XII_TEST_INT(counter, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy Constructor/Assignment/Iterator")
  {
    xiiHashSet<xiiInt32> table1;

    for (xiiInt32 i = 0; i < 64; ++i)
    {
      xiiInt32 key;

      do
      {
        key = rand() % 100000;
      } while (table1.Contains(key));

      table1.Insert(key);
    }

    // insert an element at the very end
    table1.Insert(47);

    xiiHashSet<xiiInt32> table2;
    table2 = table1;
    xiiHashSet<xiiInt32> table3(table1);

    XII_TEST_INT(table1.GetCount(), 65);
    XII_TEST_INT(table2.GetCount(), 65);
    XII_TEST_INT(table3.GetCount(), 65);
    XII_TEST_BOOL(begin(table1) != end(table1));
    XII_TEST_BOOL(cbegin(table1) != cend(table1));

    xiiUInt32 uiCounter = 0;
    for (auto it = table1.GetIterator(); it.IsValid(); ++it)
    {
      xiiConstructionCounter value;
      XII_TEST_BOOL(table2.Contains(it.Key()));
      XII_TEST_BOOL(table3.Contains(it.Key()));
      ++uiCounter;
    }
    XII_TEST_INT(uiCounter, table1.GetCount());

    uiCounter = 0;
    for (const auto& value : table1)
    {
      XII_TEST_BOOL(table2.Contains(value));
      XII_TEST_BOOL(table3.Contains(value));
      ++uiCounter;
    }
    XII_TEST_INT(uiCounter, table1.GetCount());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move Copy Constructor/Assignment")
  {
    xiiHashSet<st> set1;
    for (xiiInt32 i = 0; i < 64; ++i)
    {
      set1.Insert(xiiConstructionCounter(i));
    }

    xiiUInt64 memoryUsage = set1.GetHeapMemoryUsage();

    xiiHashSet<st> set2;
    set2 = std::move(set1);

    XII_TEST_INT(set1.GetCount(), 0);
    XII_TEST_INT(set1.GetHeapMemoryUsage(), 0);
    XII_TEST_INT(set2.GetCount(), 64);
    XII_TEST_INT(set2.GetHeapMemoryUsage(), memoryUsage);

    xiiHashSet<st> set3(std::move(set2));

    XII_TEST_INT(set2.GetCount(), 0);
    XII_TEST_INT(set2.GetHeapMemoryUsage(), 0);
    XII_TEST_INT(set3.GetCount(), 64);
    XII_TEST_INT(set3.GetHeapMemoryUsage(), memoryUsage);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Collision Tests")
  {
    xiiHashSet<Collision> set2;

    set2.Insert(Collision(0, 0));
    set2.Insert(Collision(1, 1));
    set2.Insert(Collision(0, 2));
    set2.Insert(Collision(1, 3));
    set2.Insert(Collision(1, 4));
    set2.Insert(Collision(0, 5));

    XII_TEST_BOOL(set2.Contains(Collision(0, 0)));
    XII_TEST_BOOL(set2.Contains(Collision(1, 1)));
    XII_TEST_BOOL(set2.Contains(Collision(0, 2)));
    XII_TEST_BOOL(set2.Contains(Collision(1, 3)));
    XII_TEST_BOOL(set2.Contains(Collision(1, 4)));
    XII_TEST_BOOL(set2.Contains(Collision(0, 5)));

    XII_TEST_BOOL(set2.Remove(Collision(0, 0)));
    XII_TEST_BOOL(set2.Remove(Collision(1, 1)));

    XII_TEST_BOOL(!set2.Contains(Collision(0, 0)));
    XII_TEST_BOOL(!set2.Contains(Collision(1, 1)));
    XII_TEST_BOOL(set2.Contains(Collision(0, 2)));
    XII_TEST_BOOL(set2.Contains(Collision(1, 3)));
    XII_TEST_BOOL(set2.Contains(Collision(1, 4)));
    XII_TEST_BOOL(set2.Contains(Collision(0, 5)));

    set2.Insert(Collision(0, 6));
    set2.Insert(Collision(1, 7));

    XII_TEST_BOOL(set2.Contains(Collision(0, 2)));
    XII_TEST_BOOL(set2.Contains(Collision(1, 3)));
    XII_TEST_BOOL(set2.Contains(Collision(1, 4)));
    XII_TEST_BOOL(set2.Contains(Collision(0, 5)));
    XII_TEST_BOOL(set2.Contains(Collision(0, 6)));
    XII_TEST_BOOL(set2.Contains(Collision(1, 7)));

    XII_TEST_BOOL(set2.Remove(Collision(1, 4)));
    XII_TEST_BOOL(set2.Remove(Collision(0, 6)));

    XII_TEST_BOOL(!set2.Contains(Collision(1, 4)));
    XII_TEST_BOOL(!set2.Contains(Collision(0, 6)));
    XII_TEST_BOOL(set2.Contains(Collision(0, 2)));
    XII_TEST_BOOL(set2.Contains(Collision(1, 3)));
    XII_TEST_BOOL(set2.Contains(Collision(0, 5)));
    XII_TEST_BOOL(set2.Contains(Collision(1, 7)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear")
  {
    XII_TEST_BOOL(st::HasAllDestructed());

    {
      xiiHashSet<st> m1;
      m1.Insert(st(1));
      XII_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

      m1.Insert(st(3));
      XII_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 2 temporary is created (and destroyed)

      m1.Insert(st(1));
      XII_TEST_BOOL(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      XII_TEST_BOOL(st::HasDone(0, 2));
      XII_TEST_BOOL(st::HasAllDestructed());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert")
  {
    xiiHashSet<xiiInt32> a1;

    for (xiiInt32 i = 0; i < 10; ++i)
    {
      XII_TEST_BOOL(!a1.Insert(i));
    }

    for (xiiInt32 i = 0; i < 10; ++i)
    {
      XII_TEST_BOOL(a1.Insert(i));
    }
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move Insert")
  {
    OnlyMovable noCopyObject(42);

    xiiHashSet<OnlyMovable> noCopyKey;
    // noCopyKey.Insert(noCopyObject); // Should not compile
    noCopyKey.Insert(std::move(noCopyObject));
    XII_TEST_INT(noCopyObject.m_NumTimesMoved, 1);
    XII_TEST_BOOL(noCopyKey.Contains(noCopyObject));
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove/Compact")
  {
    xiiHashSet<xiiInt32> a;

    XII_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    for (xiiInt32 i = 0; i < 1000; ++i)
    {
      a.Insert(i);
      XII_TEST_INT(a.GetCount(), i + 1);
    }

    XII_TEST_BOOL(a.GetHeapMemoryUsage() >= 1000 * (sizeof(xiiInt32)));

    a.Compact();

    for (xiiInt32 i = 0; i < 500; ++i)
    {
      XII_TEST_BOOL(a.Remove(i));
    }

    a.Compact();

    for (xiiInt32 i = 500; i < 1000; ++i)
    {
      XII_TEST_BOOL(a.Contains(i));
    }

    a.Clear();
    a.Compact();

    XII_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove (Iterator)")
  {
    xiiHashSet<xiiInt32> a;

    XII_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
    for (xiiInt32 i = 0; i < 1000; ++i)
      a.Insert(i);

    xiiHashSet<xiiInt32>::ConstIterator it = a.GetIterator();

    for (xiiInt32 i = 0; i < 1000 - 1; ++i)
    {
      xiiInt32 value = it.Key();
      it             = a.Remove(it);
      XII_TEST_BOOL(!a.Contains(value));
      XII_TEST_BOOL(it.IsValid());
      XII_TEST_INT(a.GetCount(), 1000 - 1 - i);
    }
    it = a.Remove(it);
    XII_TEST_BOOL(!it.IsValid());
    XII_TEST_BOOL(a.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Set Operations")
  {
    xiiHashSet<xiiUInt32> base;
    base.Insert(1);
    base.Insert(3);
    base.Insert(5);

    xiiHashSet<xiiUInt32> empty;

    xiiHashSet<xiiUInt32> disjunct;
    disjunct.Insert(2);
    disjunct.Insert(4);
    disjunct.Insert(6);

    xiiHashSet<xiiUInt32> subSet;
    subSet.Insert(1);
    subSet.Insert(5);

    xiiHashSet<xiiUInt32> superSet;
    superSet.Insert(1);
    superSet.Insert(3);
    superSet.Insert(5);
    superSet.Insert(7);

    xiiHashSet<xiiUInt32> nonDisjunctNonEmptySubSet;
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
      xiiHashSet<xiiUInt32> res;

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
      xiiHashSet<xiiUInt32> res;
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
      xiiHashSet<xiiUInt32> res;
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator==/!=")
  {
    xiiStaticArray<xiiInt32, 64> keys[2];

    for (xiiUInt32 i = 0; i < 64; ++i)
    {
      keys[0].PushBack(rand());
    }

    keys[1] = keys[0];

    xiiHashSet<xiiInt32> t[2];

    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      while (!keys[i].IsEmpty())
      {
        const xiiUInt32 uiIndex = rand() % keys[i].GetCount();
        const xiiInt32  key     = keys[i][uiIndex];
        t[i].Insert(key);

        keys[i].RemoveAtAndSwap(uiIndex);
      }
    }

    XII_TEST_BOOL(t[0] == t[1]);

    t[0].Insert(32);
    XII_TEST_BOOL(t[0] != t[1]);

    t[1].Insert(32);
    XII_TEST_BOOL(t[0] == t[1]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompatibleKeyType")
  {
    xiiProxyAllocator        testAllocator("Test", xiiFoundation::GetDefaultAllocator());
    xiiLocalAllocatorWrapper allocWrapper(&testAllocator);
    using TestString = xiiHybridString<32, xiiLocalAllocatorWrapper>;

    xiiHashSet<TestString> stringSet;
    const char*            szChar   = "VeryLongStringDefinitelyMoreThan32Chars1111elf!!!!";
    const char*            szString = "AnotherVeryLongStringThisTimeUsedForStringView!!!!";
    xiiStringView          sView(szString);
    xiiStringBuilder       sBuilder("BuilderAlsoNeedsToBeAVeryLongStringToTriggerAllocation");
    xiiString              sString("String");
    XII_TEST_BOOL(!stringSet.Insert(szChar));
    XII_TEST_BOOL(!stringSet.Insert(sView));
    XII_TEST_BOOL(!stringSet.Insert(sBuilder));
    XII_TEST_BOOL(!stringSet.Insert(sString));
    XII_TEST_BOOL(stringSet.Insert(szString));

    xiiUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

    XII_TEST_BOOL(stringSet.Contains(szChar));
    XII_TEST_BOOL(stringSet.Contains(sView));
    XII_TEST_BOOL(stringSet.Contains(sBuilder));
    XII_TEST_BOOL(stringSet.Contains(sString));

    XII_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

    XII_TEST_BOOL(stringSet.Remove(szChar));
    XII_TEST_BOOL(stringSet.Remove(sView));
    XII_TEST_BOOL(stringSet.Remove(sBuilder));
    XII_TEST_BOOL(stringSet.Remove(sString));

    XII_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swap")
  {
    xiiStringBuilder      tmp;
    xiiHashSet<xiiString> set1;
    xiiHashSet<xiiString> set2;

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set1.Insert(tmp);

      tmp.SetFormat("{0}{0}{0}", i);
      set2.Insert(tmp);
    }

    set1.Swap(set2);

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      XII_TEST_BOOL(set2.Contains(tmp));

      tmp.SetFormat("{0}{0}{0}", i);
      XII_TEST_BOOL(set1.Contains(tmp));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "foreach")
  {
    xiiStringBuilder      tmp;
    xiiHashSet<xiiString> set;
    xiiHashSet<xiiString> set2;

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set.Insert(tmp);
    }

    XII_TEST_INT(set.GetCount(), 1000);

    set2 = set;
    XII_TEST_INT(set2.GetCount(), set.GetCount());

    for (xiiHashSet<xiiString>::ConstIterator it = begin(set); it != end(set); ++it)
    {
      const xiiString& k = it.Key();
      set2.Remove(k);
    }

    XII_TEST_BOOL(set2.IsEmpty());
    set2 = set;

    for (auto key : set)
    {
      set2.Remove(key);
    }

    XII_TEST_BOOL(set2.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Find")
  {
    xiiStringBuilder      tmp;
    xiiHashSet<xiiString> set;

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.SetFormat("stuff{}bla", i);
      set.Insert(tmp);
    }

    for (xiiInt32 i = set.GetCount() - 1; i > 0; --i)
    {
      tmp.SetFormat("stuff{}bla", i);

      auto it = set.Find(tmp);

      XII_TEST_STRING(it.Key(), tmp);

      xiiInt32 allowedIterations = set.GetCount();
      for (auto it2 = it; it2.IsValid(); ++it2)
      {
        // just test that iteration is possible and terminates correctly
        --allowedIterations;
        XII_TEST_BOOL(allowedIterations >= 0);
      }

      set.Remove(it);
    }
  }
}
