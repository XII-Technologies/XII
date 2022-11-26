#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

namespace HashTableTestDetail
{
  typedef xiiConstructionCounter st;

  struct Collision
  {
    xiiUInt32 hash;
    int       key;

    inline Collision(xiiUInt32 hash, int key)
    {
      this->hash = hash;
      this->key  = key;
    }

    inline bool operator==(const Collision& other) const { return key == other.key; }

    XII_DECLARE_POD_TYPE();
  };

  class OnlyMovable
  {
  public:
    OnlyMovable(xiiUInt32 hash) :
      hash(hash), m_NumTimesMoved(0)
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

    int       m_NumTimesMoved;
    xiiUInt32 hash;

  private:
    OnlyMovable(const OnlyMovable&);
    void operator=(const OnlyMovable&);
  };
} // namespace HashTableTestDetail

template <>
struct xiiHashHelper<HashTableTestDetail::Collision>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const HashTableTestDetail::Collision& value) { return value.hash; }

  XII_ALWAYS_INLINE static bool Equal(const HashTableTestDetail::Collision& a, const HashTableTestDetail::Collision& b) { return a == b; }
};

template <>
struct xiiHashHelper<HashTableTestDetail::OnlyMovable>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const HashTableTestDetail::OnlyMovable& value) { return value.hash; }

  XII_ALWAYS_INLINE static bool Equal(const HashTableTestDetail::OnlyMovable& a, const HashTableTestDetail::OnlyMovable& b)
  {
    return a.hash == b.hash;
  }
};

XII_CREATE_SIMPLE_TEST(Containers, HashTable)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiHashTable<xiiInt32, HashTableTestDetail::st> table1;

    XII_TEST_BOOL(table1.GetCount() == 0);
    XII_TEST_BOOL(table1.IsEmpty());

    xiiUInt32 counter = 0;
    for (xiiHashTable<xiiInt32, HashTableTestDetail::st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ++counter;
    }
    XII_TEST_INT(counter, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy Constructor/Assignment/Iterator")
  {
    xiiHashTable<xiiInt32, HashTableTestDetail::st> table1;

    for (xiiInt32 i = 0; i < 64; ++i)
    {
      xiiInt32 key;

      do
      {
        key = rand() % 100000;
      } while (table1.Contains(key));

      table1.Insert(key, xiiConstructionCounter(i));
    }

    // insert an element at the very end
    table1.Insert(47, xiiConstructionCounter(64));

    xiiHashTable<xiiInt32, HashTableTestDetail::st> table2;
    table2 = table1;
    xiiHashTable<xiiInt32, HashTableTestDetail::st> table3(table1);

    XII_TEST_INT(table1.GetCount(), 65);
    XII_TEST_INT(table2.GetCount(), 65);
    XII_TEST_INT(table3.GetCount(), 65);

    xiiUInt32 uiCounter = 0;
    for (xiiHashTable<xiiInt32, HashTableTestDetail::st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      xiiConstructionCounter value;

      XII_TEST_BOOL(table2.TryGetValue(it.Key(), value));
      XII_TEST_BOOL(it.Value() == value);
      XII_TEST_BOOL(*table2.GetValue(it.Key()) == it.Value());

      XII_TEST_BOOL(table3.TryGetValue(it.Key(), value));
      XII_TEST_BOOL(it.Value() == value);
      XII_TEST_BOOL(*table3.GetValue(it.Key()) == it.Value());

      ++uiCounter;
    }
    XII_TEST_INT(uiCounter, table1.GetCount());

    for (xiiHashTable<xiiInt32, HashTableTestDetail::st>::Iterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      it.Value() = HashTableTestDetail::st(42);
    }

    for (xiiHashTable<xiiInt32, HashTableTestDetail::st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      xiiConstructionCounter value;

      XII_TEST_BOOL(table1.TryGetValue(it.Key(), value));
      XII_TEST_BOOL(it.Value() == value);
      XII_TEST_BOOL(value.m_iData == 42);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move Copy Constructor/Assignment")
  {
    xiiHashTable<xiiInt32, HashTableTestDetail::st> table1;
    for (xiiInt32 i = 0; i < 64; ++i)
    {
      table1.Insert(i, xiiConstructionCounter(i));
    }

    xiiUInt64 memoryUsage = table1.GetHeapMemoryUsage();

    xiiHashTable<xiiInt32, HashTableTestDetail::st> table2;
    table2 = std::move(table1);

    XII_TEST_INT(table1.GetCount(), 0);
    XII_TEST_INT(table1.GetHeapMemoryUsage(), 0);
    XII_TEST_INT(table2.GetCount(), 64);
    XII_TEST_INT(table2.GetHeapMemoryUsage(), memoryUsage);

    xiiHashTable<xiiInt32, HashTableTestDetail::st> table3(std::move(table2));

    XII_TEST_INT(table2.GetCount(), 0);
    XII_TEST_INT(table2.GetHeapMemoryUsage(), 0);
    XII_TEST_INT(table3.GetCount(), 64);
    XII_TEST_INT(table3.GetHeapMemoryUsage(), memoryUsage);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move Insert")
  {
    HashTableTestDetail::OnlyMovable noCopyObject(42);

    {
      xiiHashTable<HashTableTestDetail::OnlyMovable, int> noCopyKey;
      // noCopyKey.Insert(noCopyObject, 10); // Should not compile
      noCopyKey.Insert(std::move(noCopyObject), 10);
      XII_TEST_INT(noCopyObject.m_NumTimesMoved, 1);
      XII_TEST_BOOL(noCopyKey.Contains(noCopyObject));
    }

    {
      xiiHashTable<int, HashTableTestDetail::OnlyMovable> noCopyValue;
      // noCopyValue.Insert(10, noCopyObject); // Should not compile
      noCopyValue.Insert(10, std::move(noCopyObject));
      XII_TEST_INT(noCopyObject.m_NumTimesMoved, 2);
      XII_TEST_BOOL(noCopyValue.Contains(10));
    }

    {
      xiiHashTable<HashTableTestDetail::OnlyMovable, HashTableTestDetail::OnlyMovable> noCopyAnything;
      // noCopyAnything.Insert(10, noCopyObject); // Should not compile
      // noCopyAnything.Insert(noCopyObject, 10); // Should not compile
      noCopyAnything.Insert(std::move(noCopyObject), std::move(noCopyObject));
      XII_TEST_INT(noCopyObject.m_NumTimesMoved, 4);
      XII_TEST_BOOL(noCopyAnything.Contains(noCopyObject));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Collision Tests")
  {
    xiiHashTable<HashTableTestDetail::Collision, int> map2;

    map2[HashTableTestDetail::Collision(0, 0)] = 0;
    map2[HashTableTestDetail::Collision(1, 1)] = 1;
    map2[HashTableTestDetail::Collision(0, 2)] = 2;
    map2[HashTableTestDetail::Collision(1, 3)] = 3;
    map2[HashTableTestDetail::Collision(1, 4)] = 4;
    map2[HashTableTestDetail::Collision(0, 5)] = 5;

    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 0)] == 0);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 1)] == 1);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 4)] == 4);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);

    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 0)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 1)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 4)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));

    XII_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(0, 0)));
    XII_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(1, 1)));

    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 4)] == 4);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);

    XII_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(0, 0)));
    XII_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(1, 1)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 4)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));

    map2[HashTableTestDetail::Collision(0, 6)] = 6;
    map2[HashTableTestDetail::Collision(1, 7)] = 7;

    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 4)] == 4);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 6)] == 6);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 7)] == 7);

    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 4)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 6)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 7)));

    XII_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(1, 4)));
    XII_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(0, 6)));

    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 7)] == 7);

    XII_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(1, 4)));
    XII_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(0, 6)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));
    XII_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 7)));

    map2[HashTableTestDetail::Collision(0, 2)] = 3;
    map2[HashTableTestDetail::Collision(0, 5)] = 6;
    map2[HashTableTestDetail::Collision(1, 3)] = 4;

    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 3);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 6);
    XII_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear")
  {
    XII_TEST_BOOL(HashTableTestDetail::st::HasAllDestructed());

    {
      xiiHashTable<xiiUInt32, HashTableTestDetail::st> m1;
      m1[0] = HashTableTestDetail::st(1);
      XII_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

      m1[1] = HashTableTestDetail::st(3);
      XII_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // for inserting new elements 2 temporary is created (and destroyed)

      m1[0] = HashTableTestDetail::st(2);
      XII_TEST_BOOL(HashTableTestDetail::st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      XII_TEST_BOOL(HashTableTestDetail::st::HasDone(0, 2));
      XII_TEST_BOOL(HashTableTestDetail::st::HasAllDestructed());
    }

    {
      xiiHashTable<HashTableTestDetail::st, xiiUInt32> m1;
      m1[HashTableTestDetail::st(0)] = 1;
      XII_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // one temporary

      m1[HashTableTestDetail::st(1)] = 3;
      XII_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // one temporary

      m1[HashTableTestDetail::st(0)] = 2;
      XII_TEST_BOOL(HashTableTestDetail::st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      XII_TEST_BOOL(HashTableTestDetail::st::HasDone(0, 2));
      XII_TEST_BOOL(HashTableTestDetail::st::HasAllDestructed());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert/TryGetValue/GetValue")
  {
    xiiHashTable<xiiInt32, HashTableTestDetail::st> a1;

    for (xiiInt32 i = 0; i < 10; ++i)
    {
      XII_TEST_BOOL(!a1.Insert(i, i - 20));
    }

    for (xiiInt32 i = 0; i < 10; ++i)
    {
      HashTableTestDetail::st oldValue;
      XII_TEST_BOOL(a1.Insert(i, i, &oldValue));
      XII_TEST_INT(oldValue.m_iData, i - 20);
    }

    HashTableTestDetail::st value;
    XII_TEST_BOOL(a1.TryGetValue(9, value));
    XII_TEST_INT(value.m_iData, 9);
    XII_TEST_INT(a1.GetValue(9)->m_iData, 9);

    XII_TEST_BOOL(!a1.TryGetValue(11, value));
    XII_TEST_INT(value.m_iData, 9);
    XII_TEST_BOOL(a1.GetValue(11) == nullptr);

    HashTableTestDetail::st* pValue;
    XII_TEST_BOOL(a1.TryGetValue(9, pValue));
    XII_TEST_INT(pValue->m_iData, 9);

    pValue->m_iData = 20;
    XII_TEST_INT(a1[9].m_iData, 20);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove/Compact")
  {
    xiiHashTable<xiiInt32, HashTableTestDetail::st> a;

    XII_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    for (xiiInt32 i = 0; i < 1000; ++i)
    {
      a.Insert(i, i);
      XII_TEST_INT(a.GetCount(), i + 1);
    }

    XII_TEST_BOOL(a.GetHeapMemoryUsage() >= 1000 * (sizeof(xiiInt32) + sizeof(HashTableTestDetail::st)));

    a.Compact();

    for (xiiInt32 i = 0; i < 1000; ++i)
      XII_TEST_INT(a[i].m_iData, i);


    for (xiiInt32 i = 0; i < 250; ++i)
    {
      HashTableTestDetail::st oldValue;
      XII_TEST_BOOL(a.Remove(i, &oldValue));
      XII_TEST_INT(oldValue.m_iData, i);
    }
    XII_TEST_INT(a.GetCount(), 750);

    for (xiiHashTable<xiiInt32, HashTableTestDetail::st>::Iterator it = a.GetIterator(); it.IsValid();)
    {
      if (it.Key() < 500)
        it = a.Remove(it);
      else
        ++it;
    }
    XII_TEST_INT(a.GetCount(), 500);
    a.Compact();

    for (xiiInt32 i = 500; i < 1000; ++i)
      XII_TEST_INT(a[i].m_iData, i);

    a.Clear();
    a.Compact();

    XII_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator[]")
  {
    xiiHashTable<xiiInt32, xiiInt32> a;

    a.Insert(4, 20);
    a[2] = 30;

    XII_TEST_INT(a[4], 20);
    XII_TEST_INT(a[2], 30);
    XII_TEST_INT(a[1], 0); // new values are default constructed
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator==/!=")
  {
    xiiStaticArray<xiiInt32, 64> keys[2];

    for (xiiUInt32 i = 0; i < 64; ++i)
    {
      keys[0].PushBack(rand());
    }

    keys[1] = keys[0];

    xiiHashTable<xiiInt32, HashTableTestDetail::st> t[2];

    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      while (!keys[i].IsEmpty())
      {
        const xiiUInt32 uiIndex = rand() % keys[i].GetCount();
        const xiiInt32  key     = keys[i][uiIndex];
        t[i].Insert(key, HashTableTestDetail::st(key * 3456));

        keys[i].RemoveAtAndSwap(uiIndex);
      }
    }

    XII_TEST_BOOL(t[0] == t[1]);

    t[0].Insert(32, HashTableTestDetail::st(64));
    XII_TEST_BOOL(t[0] != t[1]);

    t[1].Insert(32, HashTableTestDetail::st(47));
    XII_TEST_BOOL(t[0] != t[1]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CompatibleKeyType")
  {
    xiiProxyAllocator        testAllocator("Test", xiiFoundation::GetDefaultAllocator());
    xiiLocalAllocatorWrapper allocWrapper(&testAllocator);
    using TestString = xiiHybridString<32, xiiLocalAllocatorWrapper>;

    xiiHashTable<TestString, int> stringTable;
    const char*                   szChar   = "VeryLongStringDefinitelyMoreThan32Chars1111elf!!!!";
    const char*                   szString = "AnotherVeryLongStringThisTimeUsedForStringView!!!!";
    xiiStringView                 sView(szString);
    xiiStringBuilder              sBuilder("BuilderAlsoNeedsToBeAVeryLongStringToTriggerAllocation");
    xiiString                     sString("String");
    XII_TEST_BOOL(!stringTable.Insert(szChar, 1));
    XII_TEST_BOOL(!stringTable.Insert(sView, 2));
    XII_TEST_BOOL(!stringTable.Insert(sBuilder, 3));
    XII_TEST_BOOL(!stringTable.Insert(sString, 4));
    XII_TEST_BOOL(stringTable.Insert(szString, 2));

    xiiUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

    XII_TEST_BOOL(stringTable.Contains(szChar));
    XII_TEST_BOOL(stringTable.Contains(sView));
    XII_TEST_BOOL(stringTable.Contains(sBuilder));
    XII_TEST_BOOL(stringTable.Contains(sString));

    XII_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

    XII_TEST_INT(*stringTable.GetValue(szChar), 1);
    XII_TEST_INT(*stringTable.GetValue(sView), 2);
    XII_TEST_INT(*stringTable.GetValue(sBuilder), 3);
    XII_TEST_INT(*stringTable.GetValue(sString), 4);

    XII_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

    XII_TEST_BOOL(stringTable.Remove(szChar));
    XII_TEST_BOOL(stringTable.Remove(sView));
    XII_TEST_BOOL(stringTable.Remove(sBuilder));
    XII_TEST_BOOL(stringTable.Remove(sString));

    XII_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swap")
  {
    xiiStringBuilder                  tmp;
    xiiHashTable<xiiString, xiiInt32> map1;
    xiiHashTable<xiiString, xiiInt32> map2;

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      map1[tmp] = i;

      tmp.Format("{0}{0}{0}", i);
      map2[tmp] = i;
    }

    map1.Swap(map2);

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      XII_TEST_BOOL(map2.Contains(tmp));
      XII_TEST_INT(map2[tmp], i);

      tmp.Format("{0}{0}{0}", i);
      XII_TEST_BOOL(map1.Contains(tmp));
      XII_TEST_INT(map1[tmp], i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "foreach")
  {
    xiiStringBuilder                  tmp;
    xiiHashTable<xiiString, xiiInt32> map;
    xiiHashTable<xiiString, xiiInt32> map2;

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      map[tmp] = i;
    }

    XII_TEST_INT(map.GetCount(), 1000);

    map2 = map;
    XII_TEST_INT(map2.GetCount(), map.GetCount());

    for (xiiHashTable<xiiString, xiiInt32>::Iterator it = begin(map); it != end(map); ++it)
    {
      const xiiString& k = it.Key();
      xiiInt32         v = it.Value();

      map2.Remove(k);
    }

    XII_TEST_BOOL(map2.IsEmpty());
    map2 = map;

    for (auto it : map)
    {
      const xiiString& k = it.Key();
      xiiInt32         v = it.Value();

      map2.Remove(k);
    }

    XII_TEST_BOOL(map2.IsEmpty());
    map2 = map;

    // just check that this compiles
    for (auto it : static_cast<const xiiHashTable<xiiString, xiiInt32>&>(map))
    {
      const xiiString& k = it.Key();
      xiiInt32         v = it.Value();

      map2.Remove(k);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Find")
  {
    xiiStringBuilder                  tmp;
    xiiHashTable<xiiString, xiiInt32> map;

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      map[tmp] = i;
    }

    for (xiiInt32 i = map.GetCount() - 1; i > 0; --i)
    {
      tmp.Format("stuff{}bla", i);

      auto it  = map.Find(tmp);
      auto cit = static_cast<const xiiHashTable<xiiString, xiiInt32>&>(map).Find(tmp);

      XII_TEST_STRING(it.Key(), tmp);
      XII_TEST_INT(it.Value(), i);

      XII_TEST_STRING(cit.Key(), tmp);
      XII_TEST_INT(cit.Value(), i);

      int allowedIterations = map.GetCount();
      for (auto it2 = it; it2.IsValid(); ++it2)
      {
        // just test that iteration is possible and terminates correctly
        --allowedIterations;
        XII_TEST_BOOL(allowedIterations >= 0);
      }

      allowedIterations = map.GetCount();
      for (auto cit2 = cit; cit2.IsValid(); ++cit2)
      {
        // just test that iteration is possible and terminates correctly
        --allowedIterations;
        XII_TEST_BOOL(allowedIterations >= 0);
      }

      map.Remove(it);
    }
  }
}
