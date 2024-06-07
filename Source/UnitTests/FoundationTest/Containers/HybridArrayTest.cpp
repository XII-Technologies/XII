#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

namespace HybridArrayTestDetail
{
  class Dummy
  {
  public:
    int         a = 0;
    std::string s = "Test";

    Dummy() = default;
    Dummy(int a) :
      a(a)
    {
    }
    Dummy(const Dummy& other) = default;
    ~Dummy()                  = default;

    Dummy& operator=(const Dummy& other) = default;

    bool operator<=(const Dummy& dummy) const { return a <= dummy.a; }
    bool operator>=(const Dummy& dummy) const { return a >= dummy.a; }
    bool operator>(const Dummy& dummy) const { return a > dummy.a; }
    bool operator<(const Dummy& dummy) const { return a < dummy.a; }
    bool operator==(const Dummy& dummy) const { return a == dummy.a; }
  };

  class NonMovableClass
  {
  public:
    NonMovableClass(int iVal)
    {
      m_val  = iVal;
      m_pVal = &m_val;
    }

    NonMovableClass(const NonMovableClass& other)
    {
      m_val  = other.m_val;
      m_pVal = &m_val;
    }

    void operator=(const NonMovableClass& other) { m_val = other.m_val; }

    int  m_val  = 0;
    int* m_pVal = nullptr;
  };

  template <typename T>
  static xiiHybridArray<T, 16> CreateArray(xiiUInt32 uiSize, xiiUInt32 uiOffset)
  {
    xiiHybridArray<T, 16> a;
    a.SetCount(uiSize);

    for (xiiUInt32 i = 0; i < uiSize; ++i)
      a[i] = T(uiOffset + i);

    return a;
  }

  struct ExternalCounter
  {
    XII_DECLARE_MEM_RELOCATABLE_TYPE();

    ExternalCounter() = default;

    ExternalCounter(int& ref_iCounter) :
      m_counter{&ref_iCounter}
    {
    }

    ~ExternalCounter()
    {
      if (m_counter)
        (*m_counter)++;
    }

    int* m_counter{};
  };
} // namespace HybridArrayTestDetail

static void TakesDynamicArray(xiiDynamicArray<int>& ref_ar, int iNum, int iStart);

#if XII_ENABLED(XII_PLATFORM_64BIT)
static_assert(sizeof(xiiHybridArray<xiiInt32, 1>) == 32);
#else
static_assert(sizeof(xiiHybridArray<xiiInt32, 1>) == 20);
#endif

static_assert(xiiGetTypeClass<xiiHybridArray<xiiInt32, 1>>::value == xiiTypeIsClass::value);
static_assert(xiiGetTypeClass<xiiHybridArray<HybridArrayTestDetail::NonMovableClass, 1>>::value == xiiTypeIsClass::value);

XII_CREATE_SIMPLE_TEST(Containers, HybridArray)
{
  xiiConstructionCounter::Reset();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiHybridArray<xiiInt32, 16>               a1;
    xiiHybridArray<xiiConstructionCounter, 16> a2;

    XII_TEST_BOOL(a1.GetCount() == 0);
    XII_TEST_BOOL(a2.GetCount() == 0);
    XII_TEST_BOOL(a1.IsEmpty());
    XII_TEST_BOOL(a2.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy Constructor")
  {
    xiiHybridArray<xiiInt32, 16> a1;

    XII_TEST_BOOL(a1.GetHeapMemoryUsage() == 0);

    for (xiiInt32 i = 0; i < 32; ++i)
    {
      a1.PushBack(rand() % 100000);

      if (i < 16)
      {
        XII_TEST_BOOL(a1.GetHeapMemoryUsage() == 0);
      }
      else
      {
        XII_TEST_BOOL(a1.GetHeapMemoryUsage() >= i * sizeof(xiiInt32));
      }
    }

    xiiHybridArray<xiiInt32, 16> a2 = a1;
    xiiHybridArray<xiiInt32, 16> a3(a1);

    XII_TEST_BOOL(a1 == a2);
    XII_TEST_BOOL(a1 == a3);
    XII_TEST_BOOL(a2 == a3);

    xiiInt32              test[] = {1, 2, 3, 4};
    xiiArrayPtr<xiiInt32> aptr(test);

    xiiHybridArray<xiiInt32, 16> a4(aptr);

    XII_TEST_BOOL(a4 == aptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move Constructor / Operator")
  {
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    {
      // move constructor external storage
      xiiHybridArray<xiiConstructionCounter, 16> a1(HybridArrayTestDetail::CreateArray<xiiConstructionCounter>(100, 20));

      XII_TEST_INT(a1.GetCount(), 100);
      for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
        XII_TEST_INT(a1[i].m_iData, 20 + i);

      // move operator external storage
      a1 = HybridArrayTestDetail::CreateArray<xiiConstructionCounter>(200, 50);

      XII_TEST_INT(a1.GetCount(), 200);
      for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
        XII_TEST_INT(a1[i].m_iData, 50 + i);
    }

    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());
    xiiConstructionCounter::Reset();

    {
      // move constructor internal storage
      xiiHybridArray<xiiConstructionCounter, 16> a2(HybridArrayTestDetail::CreateArray<xiiConstructionCounter>(10, 30));

      XII_TEST_INT(a2.GetCount(), 10);
      for (xiiUInt32 i = 0; i < a2.GetCount(); ++i)
        XII_TEST_INT(a2[i].m_iData, 30 + i);

      // move operator internal storage
      a2 = HybridArrayTestDetail::CreateArray<xiiConstructionCounter>(8, 70);

      XII_TEST_INT(a2.GetCount(), 8);
      for (xiiUInt32 i = 0; i < a2.GetCount(); ++i)
        XII_TEST_INT(a2[i].m_iData, 70 + i);
    }

    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());
    xiiConstructionCounter::Reset();

    xiiConstructionCounterRelocatable::Reset();
    {
      // move constructor external storage relocatable
      xiiHybridArray<xiiConstructionCounterRelocatable, 16> a1(HybridArrayTestDetail::CreateArray<xiiConstructionCounterRelocatable>(100, 20));

      XII_TEST_BOOL(xiiConstructionCounterRelocatable::HasDone(100, 0));

      XII_TEST_INT(a1.GetCount(), 100);
      for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
        XII_TEST_INT(a1[i].m_iData, 20 + i);

      // move operator external storage
      a1 = HybridArrayTestDetail::CreateArray<xiiConstructionCounterRelocatable>(200, 50);
      XII_TEST_BOOL(xiiConstructionCounterRelocatable::HasDone(200, 100));

      XII_TEST_INT(a1.GetCount(), 200);
      for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
        XII_TEST_INT(a1[i].m_iData, 50 + i);
    }

    XII_TEST_BOOL(xiiConstructionCounterRelocatable::HasAllDestructed());
    xiiConstructionCounterRelocatable::Reset();

    {
      // move constructor internal storage relocatable
      xiiHybridArray<xiiConstructionCounterRelocatable, 16> a2(HybridArrayTestDetail::CreateArray<xiiConstructionCounterRelocatable>(10, 30));
      XII_TEST_BOOL(xiiConstructionCounterRelocatable::HasDone(10, 0));

      XII_TEST_INT(a2.GetCount(), 10);
      for (xiiUInt32 i = 0; i < a2.GetCount(); ++i)
        XII_TEST_INT(a2[i].m_iData, 30 + i);

      // move operator internal storage
      a2 = HybridArrayTestDetail::CreateArray<xiiConstructionCounterRelocatable>(8, 70);
      XII_TEST_BOOL(xiiConstructionCounterRelocatable::HasDone(8, 10));

      XII_TEST_INT(a2.GetCount(), 8);
      for (xiiUInt32 i = 0; i < a2.GetCount(); ++i)
        XII_TEST_INT(a2[i].m_iData, 70 + i);
    }

    XII_TEST_BOOL(xiiConstructionCounterRelocatable::HasAllDestructed());
    xiiConstructionCounterRelocatable::Reset();

    {
      // move constructor with different allocators
      xiiProxyAllocator proxyAllocator("test allocator", xiiFoundation::GetDefaultAllocator());
      {
        xiiHybridArray<xiiConstructionCounterRelocatable, 16> a1(&proxyAllocator);

        a1 = HybridArrayTestDetail::CreateArray<xiiConstructionCounterRelocatable>(8, 70);
        XII_TEST_BOOL(xiiConstructionCounterRelocatable::HasDone(8, 0));
        XII_TEST_BOOL(a1.GetAllocator() == &proxyAllocator); // allocator must not change

        XII_TEST_INT(a1.GetCount(), 8);
        for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
          XII_TEST_INT(a1[i].m_iData, 70 + i);

        a1 = HybridArrayTestDetail::CreateArray<xiiConstructionCounterRelocatable>(32, 100);
        XII_TEST_BOOL(xiiConstructionCounterRelocatable::HasDone(32, 8));
        XII_TEST_BOOL(a1.GetAllocator() == &proxyAllocator); // allocator must not change

        XII_TEST_INT(a1.GetCount(), 32);
        for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
          XII_TEST_INT(a1[i].m_iData, 100 + i);
      }

      XII_TEST_BOOL(xiiConstructionCounterRelocatable::HasAllDestructed());
      xiiConstructionCounterRelocatable::Reset();

      auto allocatorStats = proxyAllocator.GetStats();
      XII_TEST_BOOL(allocatorStats.m_uiNumAllocations == allocatorStats.m_uiNumDeallocations); // check for memory leak?
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Convert to ArrayPtr")
  {
    xiiHybridArray<xiiInt32, 16> a1;

    for (xiiInt32 i = 0; i < 100; ++i)
    {
      xiiInt32 r = rand() % 100000;
      a1.PushBack(r);
    }

    xiiArrayPtr<xiiInt32> ap = a1;

    XII_TEST_BOOL(ap.GetCount() == a1.GetCount());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator =")
  {
    xiiHybridArray<xiiInt32, 16> a1, a2;

    for (xiiInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    a2 = a1;

    XII_TEST_BOOL(a1 == a2);

    xiiArrayPtr<xiiInt32> arrayPtr(a1);

    a2 = arrayPtr;

    XII_TEST_BOOL(a2 == arrayPtr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator == / !=")
  {
    xiiHybridArray<xiiInt32, 16> a1, a2;

    XII_TEST_BOOL(a1 == a1);
    XII_TEST_BOOL(a2 == a2);
    XII_TEST_BOOL(a1 == a2);

    XII_TEST_BOOL((a1 != a1) == false);
    XII_TEST_BOOL((a2 != a2) == false);
    XII_TEST_BOOL((a1 != a2) == false);

    for (xiiInt32 i = 0; i < 100; ++i)
    {
      xiiInt32 r = rand() % 100000;
      a1.PushBack(r);
      a2.PushBack(r);
    }

    XII_TEST_BOOL(a1 == a1);
    XII_TEST_BOOL(a2 == a2);
    XII_TEST_BOOL(a1 == a2);

    XII_TEST_BOOL((a1 != a2) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Index operator")
  {
    xiiHybridArray<xiiInt32, 16> a1;
    a1.SetCountUninitialized(100);

    for (xiiInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (xiiInt32 i = 0; i < 100; ++i)
      XII_TEST_INT(a1[i], i);

    const xiiHybridArray<xiiInt32, 16> ca1 = a1;

    for (xiiInt32 i = 0; i < 100; ++i)
      XII_TEST_INT(ca1[i], i);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    xiiHybridArray<xiiInt32, 16> a1;

    XII_TEST_BOOL(a1.IsEmpty());

    for (xiiInt32 i = 0; i < 128; ++i)
    {
      a1.SetCount(i + 1);
      XII_TEST_INT(a1[i], 0);
      a1[i] = i;

      XII_TEST_INT(a1.GetCount(), i + 1);
      XII_TEST_BOOL(!a1.IsEmpty());
    }

    for (xiiInt32 i = 0; i < 128; ++i)
      XII_TEST_INT(a1[i], i);

    for (xiiInt32 i = 128; i >= 0; --i)
    {
      a1.SetCount(i);

      XII_TEST_INT(a1.GetCount(), i);

      for (xiiInt32 i2 = 0; i2 < i; ++i2)
        XII_TEST_INT(a1[i2], i2);
    }

    XII_TEST_BOOL(a1.IsEmpty());

    a1.SetCountUninitialized(32);
    XII_TEST_INT(a1.GetCount(), 32);
    a1[31] = 45;
    XII_TEST_INT(a1[31], 45);

    // Test SetCount with fill value
    {
      xiiHybridArray<xiiInt32, 2> a2;
      a2.PushBack(5);
      a2.PushBack(3);
      a2.SetCount(10, 42);

      if (XII_TEST_INT(a2.GetCount(), 10))
      {
        XII_TEST_INT(a2[0], 5);
        XII_TEST_INT(a2[1], 3);
        XII_TEST_INT(a2[4], 42);
        XII_TEST_INT(a2[9], 42);
      }

      a2.Clear();
      a2.PushBack(1);
      a2.PushBack(2);
      a2.PushBack(3);

      a2.SetCount(2, 10);
      if (XII_TEST_INT(a2.GetCount(), 2))
      {
        XII_TEST_INT(a2[0], 1);
        XII_TEST_INT(a2[1], 2);
      }
    }
  }

  // Test SetCount with fill value
  {
    xiiHybridArray<xiiInt32, 2> a2;
    a2.PushBack(5);
    a2.PushBack(3);
    a2.SetCount(10, 42);

    if (XII_TEST_INT(a2.GetCount(), 10))
    {
      XII_TEST_INT(a2[0], 5);
      XII_TEST_INT(a2[1], 3);
      XII_TEST_INT(a2[4], 42);
      XII_TEST_INT(a2[9], 42);
    }

    a2.Clear();
    a2.PushBack(1);
    a2.PushBack(2);
    a2.PushBack(3);

    a2.SetCount(2, 10);
    if (XII_TEST_INT(a2.GetCount(), 2))
    {
      XII_TEST_INT(a2[0], 1);
      XII_TEST_INT(a2[1], 2);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear")
  {
    xiiHybridArray<xiiInt32, 16> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    XII_TEST_BOOL(a1.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    xiiHybridArray<xiiInt32, 16> a1;

    for (xiiInt32 i = -100; i < 100; ++i)
      XII_TEST_BOOL(!a1.Contains(i));

    for (xiiInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    for (xiiInt32 i = 0; i < 100; ++i)
    {
      XII_TEST_BOOL(a1.Contains(i));
      XII_TEST_INT(a1.IndexOf(i), i);
      XII_TEST_INT(a1.LastIndexOf(i), i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "InsertAt")
  {
    xiiHybridArray<xiiInt32, 16> a1;

    // always inserts at the front
    for (xiiInt32 i = 0; i < 100; ++i)
      a1.InsertAt(0, i);

    for (xiiInt32 i = 0; i < 100; ++i)
      XII_TEST_INT(a1[i], 99 - i);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveAndCopy")
  {
    xiiHybridArray<xiiInt32, 16> a1;

    for (xiiInt32 i = 0; i < 100; ++i)
      a1.PushBack(i % 2);

    while (a1.RemoveAndCopy(1))
    {
    }

    XII_TEST_BOOL(a1.GetCount() == 50);

    for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
      XII_TEST_INT(a1[i], 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveAndSwap")
  {
    xiiHybridArray<xiiInt32, 16> a1;

    for (xiiInt32 i = 0; i < 10; ++i)
      a1.InsertAt(i, i); // inserts at the end

    a1.RemoveAndSwap(9);
    a1.RemoveAndSwap(7);
    a1.RemoveAndSwap(5);
    a1.RemoveAndSwap(3);
    a1.RemoveAndSwap(1);

    XII_TEST_INT(a1.GetCount(), 5);

    for (xiiInt32 i = 0; i < 5; ++i)
      XII_TEST_BOOL(xiiMath::IsEven(a1[i]));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveAtAndCopy")
  {
    xiiHybridArray<xiiInt32, 16> a1;

    for (xiiInt32 i = 0; i < 10; ++i)
      a1.InsertAt(i, i); // inserts at the end

    a1.RemoveAtAndCopy(9);
    a1.RemoveAtAndCopy(7);
    a1.RemoveAtAndCopy(5);
    a1.RemoveAtAndCopy(3);
    a1.RemoveAtAndCopy(1);

    XII_TEST_INT(a1.GetCount(), 5);

    for (xiiInt32 i = 0; i < 5; ++i)
      XII_TEST_INT(a1[i], i * 2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveAtAndSwap")
  {
    xiiHybridArray<xiiInt32, 16> a1;

    for (xiiInt32 i = 0; i < 10; ++i)
      a1.InsertAt(i, i); // inserts at the end

    a1.RemoveAtAndSwap(9);
    a1.RemoveAtAndSwap(7);
    a1.RemoveAtAndSwap(5);
    a1.RemoveAtAndSwap(3);
    a1.RemoveAtAndSwap(1);

    XII_TEST_INT(a1.GetCount(), 5);

    for (xiiInt32 i = 0; i < 5; ++i)
      XII_TEST_BOOL(xiiMath::IsEven(a1[i]));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PushBack / PopBack / PeekBack")
  {
    xiiHybridArray<xiiInt32, 16> a1;

    for (xiiInt32 i = 0; i < 10; ++i)
    {
      a1.PushBack(i);
      XII_TEST_INT(a1.PeekBack(), i);
    }

    for (xiiInt32 i = 9; i >= 0; --i)
    {
      XII_TEST_INT(a1.PeekBack(), i);
      a1.PopBack();
    }

    a1.PushBack(23);
    a1.PushBack(2);
    a1.PushBack(3);

    a1.PopBack(2);
    XII_TEST_INT(a1.PeekBack(), 23);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExpandAndGetRef")
  {
    xiiHybridArray<xiiInt32, 16> a1;

    for (xiiInt32 i = 0; i < 20; ++i)
    {
      xiiInt32& intRef = a1.ExpandAndGetRef();
      intRef           = i * 5;
    }


    XII_TEST_BOOL(a1.GetCount() == 20);

    for (xiiInt32 i = 0; i < 20; ++i)
    {
      XII_TEST_INT(a1[i], i * 5);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Construction / Destruction")
  {
    {
      XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

      xiiHybridArray<xiiConstructionCounter, 16> a1;
      xiiHybridArray<xiiConstructionCounter, 16> a2;

      XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
      XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

      a1.PushBack(xiiConstructionCounter(1));
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a1.InsertAt(0, xiiConstructionCounter(2));
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a2 = a1;
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 0)); // two copies

      a1.Clear();
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 2));

      a1.PushBack(xiiConstructionCounter(3));
      a1.PushBack(xiiConstructionCounter(4));
      a1.PushBack(xiiConstructionCounter(5));
      a1.PushBack(xiiConstructionCounter(6));

      XII_TEST_BOOL(xiiConstructionCounter::HasDone(8, 4)); // four temporaries

      a1.RemoveAndCopy(xiiConstructionCounter(3));
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(1, 2)); // one temporary, one destroyed

      a1.RemoveAndCopy(xiiConstructionCounter(3));
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(1, 1)); // one temporary, none destroyed

      a1.RemoveAtAndCopy(0);
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 1)); // one destroyed

      a1.RemoveAtAndSwap(0);
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 1)); // one destroyed
    }

    // tests the destructor of a2 and a1
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compact")
  {
    xiiHybridArray<xiiInt32, 16> a;

    for (xiiInt32 i = 0; i < 1008; ++i)
    {
      a.PushBack(i);
      XII_TEST_INT(a.GetCount(), i + 1);
    }

    XII_TEST_BOOL(a.GetHeapMemoryUsage() > 0);
    a.Compact();
    XII_TEST_BOOL(a.GetHeapMemoryUsage() > 0);

    for (xiiInt32 i = 0; i < 1008; ++i)
      XII_TEST_INT(a[i], i);

    // this tests whether the static array is reused properly (not the case anymore with new implementation that derives from xiiDynamicArray)
    a.SetCount(15);
    a.Compact();
    // XII_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
    XII_TEST_BOOL(a.GetHeapMemoryUsage() > 0);

    for (xiiInt32 i = 0; i < 15; ++i)
      XII_TEST_INT(a[i], i);

    a.Clear();
    a.Compact();
    XII_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SortingPrimitives")
  {
    xiiHybridArray<xiiUInt32, 16> list;

    list.Sort();

    for (xiiUInt32 i = 0; i < 45; i++)
    {
      list.PushBack(std::rand());
    }
    list.Sort();

    xiiUInt32 last = 0;
    for (xiiUInt32 i = 0; i < list.GetCount(); i++)
    {
      XII_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SortingObjects")
  {
    xiiHybridArray<HybridArrayTestDetail::Dummy, 16> list;
    list.Reserve(128);

    for (xiiUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(HybridArrayTestDetail::Dummy(rand()));
    }
    list.Sort();

    HybridArrayTestDetail::Dummy last = 0;
    for (xiiUInt32 i = 0; i < list.GetCount(); i++)
    {
      XII_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Various")
  {
    xiiHybridArray<HybridArrayTestDetail::Dummy, 16> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    list.InsertAt(3, 4);
    list.InsertAt(1, 0);
    list.InsertAt(5, 0);

    XII_TEST_BOOL(list[0].a == 1);
    XII_TEST_BOOL(list[1].a == 0);
    XII_TEST_BOOL(list[2].a == 2);
    XII_TEST_BOOL(list[3].a == 3);
    XII_TEST_BOOL(list[4].a == 4);
    XII_TEST_BOOL(list[5].a == 0);
    XII_TEST_BOOL(list.GetCount() == 6);

    list.RemoveAtAndCopy(3);
    list.RemoveAtAndSwap(2);

    XII_TEST_BOOL(list[0].a == 1);
    XII_TEST_BOOL(list[1].a == 0);
    XII_TEST_BOOL(list[2].a == 0);
    XII_TEST_BOOL(list[3].a == 4);
    XII_TEST_BOOL(list.GetCount() == 4);
    XII_TEST_BOOL(list.IndexOf(0) == 1);
    XII_TEST_BOOL(list.LastIndexOf(0) == 2);

    list.PushBack(5);
    XII_TEST_BOOL(list[4].a == 5);
    HybridArrayTestDetail::Dummy d = list.PeekBack();
    list.PopBack();
    XII_TEST_BOOL(d.a == 5);
    XII_TEST_BOOL(list.GetCount() == 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Assignment")
  {
    xiiHybridArray<HybridArrayTestDetail::Dummy, 16> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(HybridArrayTestDetail::Dummy(rand()));
    }

    xiiHybridArray<HybridArrayTestDetail::Dummy, 16> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(HybridArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    XII_TEST_BOOL(list.GetCount() == list2.GetCount());

    list2.Clear();
    XII_TEST_BOOL(list2.GetCount() == 0);

    list2 = list;
    XII_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    XII_TEST_BOOL(list == list2);

    for (int i = 0; i < 16; i++)
    {
      list2.PushBack(HybridArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    XII_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    XII_TEST_BOOL(list == list2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Count")
  {
    xiiHybridArray<HybridArrayTestDetail::Dummy, 16> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(HybridArrayTestDetail::Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(4);

    list.Compact();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reserve")
  {
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    xiiHybridArray<xiiConstructionCounter, 16> a;

    XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    a.Reserve(100);

    XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    a.SetCount(10);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(10, 0));

    a.Reserve(100);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 0));

    a.SetCount(100);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(90, 0));

    a.Reserve(200);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(100, 100)); // had to copy some elements over

    a.SetCount(200);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(100, 0));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compact")
  {
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    xiiHybridArray<xiiConstructionCounter, 16> a;

    XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    a.SetCount(100);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(100, 0));

    a.SetCount(200);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(200, 100));

    a.SetCount(10);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 190));

    // no reallocations and copying, if the memory is already available
    a.SetCount(200);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(190, 0));

    a.SetCount(10);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 190));

    // now we remove the spare memory
    a.Compact();
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(10, 10));

    // this time the array needs to be relocated, and thus the already present elements need to be copied
    a.SetCount(200);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(200, 10));

    // this does not deallocate memory
    a.Clear();
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 200));

    a.SetCount(100);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(100, 0));

    // therefore no object relocation
    a.SetCount(200);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(100, 0));

    a.Clear();
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 200));

    // this will deallocate ALL memory
    a.Compact();

    a.SetCount(100);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(100, 0));

    // this time objects need to be relocated
    a.SetCount(200);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(200, 100));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "STL Iterator")
  {
    xiiHybridArray<xiiInt32, 16> a1;

    for (xiiInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    // STL sort
    std::sort(begin(a1), end(a1));

    for (xiiInt32 i = 1; i < 1000; ++i)
    {
      XII_TEST_BOOL(a1[i - 1] <= a1[i]);
    }

    // foreach
    xiiUInt32 prev = 0;
    for (xiiUInt32 val : a1)
    {
      XII_TEST_BOOL(prev <= val);
      prev = val;
    }

    // const array
    const xiiHybridArray<xiiInt32, 16>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(begin(a2), end(a2), 400);
    XII_TEST_BOOL(*lb == a2[400]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "STL Reverse Iterator")
  {
    xiiHybridArray<xiiInt32, 16> a1;

    for (xiiInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    // STL sort
    std::sort(rbegin(a1), rend(a1));

    for (xiiInt32 i = 1; i < 1000; ++i)
    {
      XII_TEST_BOOL(a1[i - 1] >= a1[i]);
    }

    // foreach
    xiiUInt32 prev = 1000;
    for (xiiUInt32 val : a1)
    {
      XII_TEST_BOOL(prev >= val);
      prev = val;
    }

    // const array
    const xiiHybridArray<xiiInt32, 16>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(a2), rend(a2), 400);
    XII_TEST_BOOL(*lb == a2[1000 - 400 - 1]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swap")
  {

    xiiInt32 content1[]     = {1, 2, 3, 4};
    xiiInt32 content2[]     = {5, 6, 7, 8, 9};
    xiiInt32 contentHeap1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    xiiInt32 contentHeap2[] = {11, 12, 13, 14, 15, 16, 17, 18, 19, 110, 111, 112, 113};

    {
      // local <-> local
      xiiHybridArray<xiiInt32, 8>  a1;
      xiiHybridArray<xiiInt32, 16> a2;
      a1 = xiiMakeArrayPtr(content1);
      a2 = xiiMakeArrayPtr(content2);

      xiiInt32* a1Ptr = a1.GetData();
      xiiInt32* a2Ptr = a2.GetData();

      a1.Swap(a2);

      // Because the data points to the internal storage the pointers shouldn't change when swapping
      XII_TEST_BOOL(a1Ptr == a1.GetData());
      XII_TEST_BOOL(a2Ptr == a2.GetData());

      // The data however should be swapped
      XII_TEST_BOOL(a1.GetArrayPtr() == xiiMakeArrayPtr(content2));
      XII_TEST_BOOL(a2.GetArrayPtr() == xiiMakeArrayPtr(content1));

      XII_TEST_INT(a1.GetCapacity(), 8);
      XII_TEST_INT(a2.GetCapacity(), 16);
    }

    {
      // local <-> heap
      xiiHybridArray<xiiInt32, 8> a1;
      xiiDynamicArray<xiiInt32>   a2;
      a1              = xiiMakeArrayPtr(content1);
      a2              = xiiMakeArrayPtr(contentHeap1);
      xiiInt32* a1Ptr = a1.GetData();
      xiiInt32* a2Ptr = a2.GetData();
      a1.Swap(a2);
      XII_TEST_BOOL(a1Ptr != a1.GetData());
      XII_TEST_BOOL(a2Ptr != a2.GetData());
      XII_TEST_BOOL(a1.GetArrayPtr() == xiiMakeArrayPtr(contentHeap1));
      XII_TEST_BOOL(a2.GetArrayPtr() == xiiMakeArrayPtr(content1));

      XII_TEST_INT(a1.GetCapacity(), 16);
      XII_TEST_INT(a2.GetCapacity(), 16);
    }

    {
      // heap <-> local
      xiiHybridArray<xiiInt32, 8> a1;
      xiiHybridArray<xiiInt32, 7> a2;
      a1              = xiiMakeArrayPtr(content1);
      a2              = xiiMakeArrayPtr(contentHeap1);
      xiiInt32* a1Ptr = a1.GetData();
      xiiInt32* a2Ptr = a2.GetData();
      a2.Swap(a1); // Swap is opposite direction as before
      XII_TEST_BOOL(a1Ptr != a1.GetData());
      XII_TEST_BOOL(a2Ptr != a2.GetData());
      XII_TEST_BOOL(a1.GetArrayPtr() == xiiMakeArrayPtr(contentHeap1));
      XII_TEST_BOOL(a2.GetArrayPtr() == xiiMakeArrayPtr(content1));

      XII_TEST_INT(a1.GetCapacity(), 16);
      XII_TEST_INT(a2.GetCapacity(), 16);
    }

    {
      // heap <-> heap
      xiiDynamicArray<xiiInt32>   a1;
      xiiHybridArray<xiiInt32, 8> a2;
      a1              = xiiMakeArrayPtr(contentHeap1);
      a2              = xiiMakeArrayPtr(contentHeap2);
      xiiInt32* a1Ptr = a1.GetData();
      xiiInt32* a2Ptr = a2.GetData();
      a2.Swap(a1);
      XII_TEST_BOOL(a1Ptr != a1.GetData());
      XII_TEST_BOOL(a2Ptr != a2.GetData());
      XII_TEST_BOOL(a1.GetArrayPtr() == xiiMakeArrayPtr(contentHeap2));
      XII_TEST_BOOL(a2.GetArrayPtr() == xiiMakeArrayPtr(contentHeap1));

      XII_TEST_INT(a1.GetCapacity(), 16);
      XII_TEST_INT(a2.GetCapacity(), 16);
    }

    {
      // empty <-> local
      xiiHybridArray<xiiInt32, 8> a1, a2;
      a2 = xiiMakeArrayPtr(content2);
      a1.Swap(a2);
      XII_TEST_BOOL(a1.GetArrayPtr() == xiiMakeArrayPtr(content2));
      XII_TEST_BOOL(a2.IsEmpty());

      XII_TEST_INT(a1.GetCapacity(), 8);
      XII_TEST_INT(a2.GetCapacity(), 8);
    }

    {
      // empty <-> empty
      xiiHybridArray<xiiInt32, 8> a1, a2;
      a1.Swap(a2);
      XII_TEST_BOOL(a1.IsEmpty());
      XII_TEST_BOOL(a2.IsEmpty());

      XII_TEST_INT(a1.GetCapacity(), 8);
      XII_TEST_INT(a2.GetCapacity(), 8);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move")
  {
    int counter = 0;
    {
      xiiHybridArray<HybridArrayTestDetail::ExternalCounter, 2> a, b;
      XII_TEST_BOOL(counter == 0);

      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      XII_TEST_BOOL(counter == 1);

      b = std::move(a);
      XII_TEST_BOOL(counter == 1);
    }
    XII_TEST_BOOL(counter == 2);

    counter = 0;
    {
      xiiHybridArray<HybridArrayTestDetail::ExternalCounter, 2> a, b;
      XII_TEST_BOOL(counter == 0);

      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      XII_TEST_BOOL(counter == 4);

      b = std::move(a);
      XII_TEST_BOOL(counter == 4);
    }
    XII_TEST_BOOL(counter == 8);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Use xiiHybridArray with xiiDynamicArray")
  {
    xiiHybridArray<int, 16> a;

    TakesDynamicArray(a, 4, a.GetCount());
    XII_TEST_INT(a.GetCount(), 4);
    XII_TEST_INT(a.GetCapacity(), 16);

    for (int i = 0; i < (int)a.GetCount(); ++i)
    {
      XII_TEST_INT(a[i], i);
    }

    TakesDynamicArray(a, 12, a.GetCount());
    XII_TEST_INT(a.GetCount(), 16);
    XII_TEST_INT(a.GetCapacity(), 16);

    for (int i = 0; i < (int)a.GetCount(); ++i)
    {
      XII_TEST_INT(a[i], i);
    }

    TakesDynamicArray(a, 8, a.GetCount());
    XII_TEST_INT(a.GetCount(), 24);
    XII_TEST_INT(a.GetCapacity(), 32);

    for (int i = 0; i < (int)a.GetCount(); ++i)
    {
      XII_TEST_INT(a[i], i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Nested arrays")
  {
    xiiDynamicArray<xiiHybridArray<HybridArrayTestDetail::NonMovableClass, 4>> a;

    for (int i = 0; i < 100; ++i)
    {
      xiiHybridArray<HybridArrayTestDetail::NonMovableClass, 4> b;
      b.PushBack(HybridArrayTestDetail::NonMovableClass(i));

      a.PushBack(std::move(b));
    }

    for (int i = 0; i < 100; ++i)
    {
      auto& nonMoveable = a[i][0];

      XII_TEST_INT(nonMoveable.m_val, i);
      XII_TEST_BOOL(nonMoveable.m_pVal == &nonMoveable.m_val);
    }
  }
}

void TakesDynamicArray(xiiDynamicArray<int>& ref_ar, int iNum, int iStart)
{
  for (int i = 0; i < iNum; ++i)
  {
    ref_ar.PushBack(iStart + i);
  }
}
