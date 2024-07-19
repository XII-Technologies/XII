#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Types/UniquePtr.h>

static xiiInt32 iCallPodConstructor    = 0;
static xiiInt32 iCallPodDestructor     = 0;
static xiiInt32 iCallNonPodConstructor = 0;
static xiiInt32 iCallNonPodDestructor  = 0;

namespace DynamicArrayTestDetail
{
  using st = xiiConstructionCounter;

  static int g_iDummyCounter = 0;

  class Dummy
  {
  public:
    int         a;
    int         b;
    std::string s;

    Dummy() :
      a(0), b(g_iDummyCounter++), s("Test")
    {
    }
    Dummy(int a) :
      a(a), b(g_iDummyCounter++), s("Test")
    {
    }

    bool operator<=(const Dummy& dummy) const { return a <= dummy.a; }
    bool operator>=(const Dummy& dummy) const { return a >= dummy.a; }
    bool operator>(const Dummy& dummy) const { return a > dummy.a; }
    bool operator<(const Dummy& dummy) const { return a < dummy.a; }
    bool operator==(const Dummy& dummy) const { return a == dummy.a; }
  };

  xiiAllocatorBase* g_pTestAllocator;

  struct xiiTestAllocatorWrapper
  {
    static xiiAllocatorBase* GetAllocator() { return g_pTestAllocator; }
  };

  template <typename T = st, typename AllocatorWrapper = xiiTestAllocatorWrapper>
  static xiiDynamicArray<T, AllocatorWrapper> CreateArray(xiiUInt32 uiSize, xiiUInt32 uiOffset)
  {
    xiiDynamicArray<T, AllocatorWrapper> a;
    a.SetCount(uiSize);

    for (xiiUInt32 i = 0; i < uiSize; ++i)
      a[i] = T(uiOffset + i);

    return a;
  }
} // namespace DynamicArrayTestDetail

#if XII_ENABLED(XII_PLATFORM_64BIT)
static_assert(sizeof(xiiDynamicArray<xiiInt32>) == 24);
#else
static_assert(sizeof(xiiDynamicArray<xiiInt32>) == 16);
#endif

XII_CREATE_SIMPLE_TEST_GROUP(Containers);

XII_CREATE_SIMPLE_TEST(Containers, DynamicArray)
{
  iCallPodConstructor    = 0;
  iCallPodDestructor     = 0;
  iCallNonPodConstructor = 0;
  iCallNonPodDestructor  = 0;

  xiiProxyAllocator proxy("DynamicArrayTestAllocator", xiiFoundation::GetDefaultAllocator());
  DynamicArrayTestDetail::g_pTestAllocator = &proxy;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiDynamicArray<xiiInt32>                   a1;
    xiiDynamicArray<DynamicArrayTestDetail::st> a2;

    XII_TEST_BOOL(a1.GetCount() == 0);
    XII_TEST_BOOL(a2.GetCount() == 0);
    XII_TEST_BOOL(a1.IsEmpty());
    XII_TEST_BOOL(a2.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy Constructor")
  {
    xiiDynamicArray<xiiInt32, DynamicArrayTestDetail::xiiTestAllocatorWrapper> a1;

    XII_TEST_BOOL(a1.GetHeapMemoryUsage() == 0);

    for (xiiInt32 i = 0; i < 32; ++i)
      a1.PushBack(rand() % 100000);

    XII_TEST_BOOL(a1.GetHeapMemoryUsage() >= 32 * sizeof(xiiInt32));

    xiiDynamicArray<xiiInt32> a2 = a1;
    xiiDynamicArray<xiiInt32> a3(a1);

    XII_TEST_BOOL(a1 == a2);
    XII_TEST_BOOL(a1 == a3);
    XII_TEST_BOOL(a2 == a3);

    xiiInt32              test[] = {1, 2, 3, 4};
    xiiArrayPtr<xiiInt32> aptr(test);

    xiiDynamicArray<xiiInt32> a4(aptr);

    XII_TEST_BOOL(a4 == aptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move Constructor / Operator")
  {
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    {
      // move constructor
      xiiDynamicArray<DynamicArrayTestDetail::st, DynamicArrayTestDetail::xiiTestAllocatorWrapper> a1(DynamicArrayTestDetail::CreateArray(100, 20));

      XII_TEST_INT(a1.GetCount(), 100);
      for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
        XII_TEST_INT(a1[i].m_iData, 20 + i);

      // move operator
      a1 = DynamicArrayTestDetail::CreateArray(200, 50);

      XII_TEST_INT(a1.GetCount(), 200);
      for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
        XII_TEST_INT(a1[i].m_iData, 50 + i);
    }

    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    {
      // move assignment with different allocators
      xiiConstructionCounterRelocatable::Reset();
      xiiProxyAllocator proxyAllocator("test allocator", xiiFoundation::GetDefaultAllocator());
      {
        xiiDynamicArray<xiiConstructionCounterRelocatable> a1(&proxyAllocator);

        a1 = DynamicArrayTestDetail::CreateArray<xiiConstructionCounterRelocatable, xiiDefaultAllocatorWrapper>(8, 70);
        XII_TEST_BOOL(xiiConstructionCounterRelocatable::HasDone(8, 0));
        XII_TEST_BOOL(a1.GetAllocator() == &proxyAllocator); // allocator must not change

        XII_TEST_INT(a1.GetCount(), 8);
        for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
          XII_TEST_INT(a1[i].m_iData, 70 + i);

        a1 = DynamicArrayTestDetail::CreateArray<xiiConstructionCounterRelocatable, xiiDefaultAllocatorWrapper>(32, 100);
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
    xiiDynamicArray<xiiInt32> a1;

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
    xiiDynamicArray<xiiInt32, DynamicArrayTestDetail::xiiTestAllocatorWrapper> a1;
    xiiDynamicArray<xiiInt32>                                                  a2;

    for (xiiInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    a2 = a1;

    XII_TEST_BOOL(a1 == a2);

    xiiArrayPtr<xiiInt32> arrayPtr(a1);

    a2 = arrayPtr;

    XII_TEST_BOOL(a2 == arrayPtr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator == / !=/ <")
  {
    xiiDynamicArray<xiiInt32> a1, a2;

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

    XII_TEST_BOOL((a1 < a2) == false);
    a2.PushBack(100);
    XII_TEST_BOOL(a1 < a2);
    a1.PushBack(99);
    XII_TEST_BOOL(a1 < a2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Index operator")
  {
    xiiDynamicArray<xiiInt32> a1;
    a1.SetCountUninitialized(100);

    for (xiiInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (xiiInt32 i = 0; i < 100; ++i)
      XII_TEST_INT(a1[i], i);

    xiiDynamicArray<xiiInt32> ca1;
    ca1 = a1;

    for (xiiInt32 i = 0; i < 100; ++i)
      XII_TEST_INT(ca1[i], i);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    xiiDynamicArray<xiiInt32> a1;

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
      xiiDynamicArray<xiiInt32> a2;
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
    xiiDynamicArray<xiiInt32> a2;
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "EnsureCount")
  {
    xiiDynamicArray<xiiInt32> a1;

    XII_TEST_INT(a1.GetCount(), 0);

    a1.EnsureCount(0);
    XII_TEST_INT(a1.GetCount(), 0);

    a1.EnsureCount(1);
    XII_TEST_INT(a1.GetCount(), 1);

    a1.EnsureCount(2);
    XII_TEST_INT(a1.GetCount(), 2);

    a1.EnsureCount(1);
    XII_TEST_INT(a1.GetCount(), 2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear")
  {
    xiiDynamicArray<xiiInt32> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    XII_TEST_BOOL(a1.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    xiiDynamicArray<xiiInt32> a1;

    for (xiiInt32 i = -100; i < 100; ++i)
      XII_TEST_BOOL(!a1.Contains(i));

    for (xiiInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);
    for (xiiInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    for (xiiInt32 i = 0; i < 100; ++i)
    {
      XII_TEST_BOOL(a1.Contains(i));
      XII_TEST_INT(a1.IndexOf(i), i);
      XII_TEST_INT(a1.IndexOf(i, 100), i + 100);
      XII_TEST_INT(a1.LastIndexOf(i), i + 100);
      XII_TEST_INT(a1.LastIndexOf(i, 100), i);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PushBackUnchecked / PushBackRange")
  {
    xiiDynamicArray<xiiInt32> a1;
    a1.Reserve(100);

    for (xiiInt32 i = 0; i < 100; ++i)
      a1.PushBackUnchecked(i);

    for (xiiInt32 i = 0; i < 100; ++i)
      XII_TEST_INT(a1[i], i);

    xiiInt32              temp[] = {100, 101, 102, 103, 104};
    xiiArrayPtr<xiiInt32> range(temp);

    a1.PushBackRange(range);

    XII_TEST_INT(a1.GetCount(), 105);
    for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
      XII_TEST_INT(a1[i], i);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert")
  {
    xiiDynamicArray<xiiInt32> a1;

    // always inserts at the front
    for (xiiInt32 i = 0; i < 100; ++i)
      a1.InsertAt(0, i);

    for (xiiInt32 i = 0; i < 100; ++i)
      XII_TEST_INT(a1[i], 99 - i);

    xiiUniquePtr<DynamicArrayTestDetail::st> ptr = XII_DEFAULT_NEW(DynamicArrayTestDetail::st);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasConstructed(1));

    {
      xiiDynamicArray<xiiUniquePtr<DynamicArrayTestDetail::st>> a2;
      for (xiiUInt32 i = 0; i < 10; ++i)
        a2.InsertAt(0, xiiUniquePtr<DynamicArrayTestDetail::st>());

      a2.InsertAt(0, std::move(ptr));
      XII_TEST_BOOL(ptr == nullptr);
      XII_TEST_BOOL(a2[0] != nullptr);

      for (xiiUInt32 i = 1; i < a2.GetCount(); ++i)
        XII_TEST_BOOL(a2[i] == nullptr);
    }

    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "InsertRange")
  {
    // Pod element tests
    xiiDynamicArray<xiiInt32> intTestRange;
    xiiDynamicArray<xiiInt32> a1;

    xiiInt32              intTemp1[] = {91, 92, 93, 94, 95};
    xiiArrayPtr<xiiInt32> intRange1(intTemp1);

    xiiInt32              intTemp2[] = {96, 97, 98, 99, 100};
    xiiArrayPtr<xiiInt32> intRange2(intTemp2);

    xiiInt32              intTemp3[] = {100, 101, 102, 103, 104};
    xiiArrayPtr<xiiInt32> intRange3(intTemp3);

    {
      intTestRange.PushBackRange(intRange3);

      a1.InsertRange(intRange3, 0);

      XII_TEST_INT(a1.GetCount(), 5);

      for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
        XII_TEST_INT(a1[i], intTestRange[i]);
    }

    {
      intTestRange.Clear();
      intTestRange.PushBackRange(intRange1);
      intTestRange.PushBackRange(intRange3);

      a1.InsertRange(intRange1, 0);

      XII_TEST_INT(a1.GetCount(), 10);
      for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
        XII_TEST_INT(a1[i], intTestRange[i]);
    }

    {
      intTestRange.Clear();
      intTestRange.PushBackRange(intRange1);
      intTestRange.PushBackRange(intRange2);
      intTestRange.PushBackRange(intRange3);

      a1.InsertRange(intRange2, 5);

      XII_TEST_INT(a1.GetCount(), 15);
      for (xiiUInt32 i = 0; i < a1.GetCount(); ++i)
        XII_TEST_INT(a1[i], intTestRange[i]);
    }

    // Class element tests
    xiiDynamicArray<xiiDeque<xiiString>> classTestRange;
    xiiDynamicArray<xiiDeque<xiiString>> a2;

    xiiDeque<xiiString> strTemp1[4];
    {
      strTemp1[0].PushBack("One");
      strTemp1[1].PushBack("Two");
      strTemp1[2].PushBack("Three");
      strTemp1[3].PushBack("Four");
    }
    xiiArrayPtr<xiiDeque<xiiString>> classRange1(strTemp1);

    xiiDeque<xiiString> strTemp2[3];
    {
      strTemp2[0].PushBack("Five");
      strTemp2[1].PushBack("Six");
      strTemp2[2].PushBack("Seven");
    }
    xiiArrayPtr<xiiDeque<xiiString>> classRange2(strTemp2);

    xiiDeque<xiiString> strTemp3[3];
    {
      strTemp3[0].PushBack("Eight");
      strTemp3[1].PushBack("Nine");
      strTemp3[2].PushBack("Ten");
    }
    xiiArrayPtr<xiiDeque<xiiString>> classRange3(strTemp3);

    {
      classTestRange.PushBackRange(classRange3);

      a2.InsertRange(classRange3, 0);

      XII_TEST_INT(a2.GetCount(), 3);

      for (xiiUInt32 i = 0; i < a2.GetCount(); ++i)
        XII_TEST_STRING(a2[i].PeekFront(), classTestRange[i].PeekFront());
    }

    {
      classTestRange.Clear();
      classTestRange.PushBackRange(classRange1);
      classTestRange.PushBackRange(classRange3);

      a2.InsertRange(classRange1, 0);

      XII_TEST_INT(a2.GetCount(), 7);
      for (xiiUInt32 i = 0; i < a2.GetCount(); ++i)
        XII_TEST_STRING(a2[i].PeekFront(), classTestRange[i].PeekFront());
    }

    {
      classTestRange.Clear();
      classTestRange.PushBackRange(classRange1);
      classTestRange.PushBackRange(classRange2);
      classTestRange.PushBackRange(classRange3);

      a2.InsertRange(classRange2, 4);

      XII_TEST_INT(a2.GetCount(), 10);
      for (xiiUInt32 i = 0; i < a2.GetCount(); ++i)
        XII_TEST_STRING(a2[i].PeekFront(), classTestRange[i].PeekFront());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveAndCopy")
  {
    xiiDynamicArray<xiiInt32> a1;

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
    xiiDynamicArray<xiiInt32> a1;

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
    xiiDynamicArray<xiiInt32> a1;

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

    xiiUniquePtr<DynamicArrayTestDetail::st> ptr = XII_DEFAULT_NEW(DynamicArrayTestDetail::st);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasConstructed(1));

    {
      xiiDynamicArray<xiiUniquePtr<DynamicArrayTestDetail::st>> a2;
      for (xiiUInt32 i = 0; i < 10; ++i)
        a2.InsertAt(0, xiiUniquePtr<DynamicArrayTestDetail::st>());

      a2.PushBack(std::move(ptr));
      XII_TEST_BOOL(ptr == nullptr);
      XII_TEST_BOOL(a2[10] != nullptr);

      a2.RemoveAtAndCopy(0);
      XII_TEST_BOOL(a2[9] != nullptr);
      XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0));
    }

    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveAtAndSwap")
  {
    xiiDynamicArray<xiiInt32> a1;

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

    xiiUniquePtr<DynamicArrayTestDetail::st> ptr = XII_DEFAULT_NEW(DynamicArrayTestDetail::st);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasConstructed(1));

    {
      xiiDynamicArray<xiiUniquePtr<DynamicArrayTestDetail::st>> a2;
      for (xiiUInt32 i = 0; i < 10; ++i)
        a2.InsertAt(0, xiiUniquePtr<DynamicArrayTestDetail::st>());

      a2.PushBack(std::move(ptr));
      XII_TEST_BOOL(ptr == nullptr);
      XII_TEST_BOOL(a2[10] != nullptr);

      a2.RemoveAtAndSwap(0);
      XII_TEST_BOOL(a2[0] != nullptr);
    }

    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PushBack / PopBack / PeekBack")
  {
    xiiDynamicArray<xiiInt32> a1;

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
    xiiDynamicArray<xiiInt32> a1;

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
      XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

      xiiDynamicArray<DynamicArrayTestDetail::st> a1;
      xiiDynamicArray<DynamicArrayTestDetail::st> a2;

      XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
      XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

      a1.PushBack(DynamicArrayTestDetail::st(1));
      XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a1.InsertAt(0, DynamicArrayTestDetail::st(2));
      XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a2 = a1;
      XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(2, 0)); // two copies

      a1.Clear();
      XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 2));

      a1.PushBack(DynamicArrayTestDetail::st(3));
      a1.PushBack(DynamicArrayTestDetail::st(4));
      a1.PushBack(DynamicArrayTestDetail::st(5));
      a1.PushBack(DynamicArrayTestDetail::st(6));

      XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(8, 4)); // four temporaries

      a1.RemoveAndCopy(DynamicArrayTestDetail::st(3));
      XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(1, 2)); // one temporary, one destroyed

      a1.RemoveAndCopy(DynamicArrayTestDetail::st(3));
      XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(1, 1)); // one temporary, none destroyed

      a1.RemoveAtAndCopy(0);
      XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 1)); // one destroyed

      a1.RemoveAtAndSwap(0);
      XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 1)); // one destroyed
    }

    // tests the destructor of a2 and a1
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SortingPrimitives")
  {
    xiiDynamicArray<xiiUInt32> list;

    list.Sort();

    for (xiiUInt32 i = 0; i < 450; i++)
    {
      list.PushBack(std::rand());
    }
    list.Sort();

    for (xiiUInt32 i = 1; i < list.GetCount(); i++)
    {
      XII_TEST_BOOL(list[i - 1] <= list[i]);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SortingObjects")
  {
    xiiDynamicArray<DynamicArrayTestDetail::Dummy> list;
    list.Reserve(128);

    for (xiiUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }
    list.Sort();

    for (xiiUInt32 i = 1; i < list.GetCount(); i++)
    {
      XII_TEST_BOOL(list[i - 1] <= list[i]);
      XII_TEST_BOOL(list[i].s == "Test");
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SortingMovableObjects")
  {
    {
      xiiDynamicArray<xiiUniquePtr<DynamicArrayTestDetail::st>> list;
      list.Reserve(128);

      for (xiiUInt32 i = 0; i < 100; i++)
      {
        list.PushBack(XII_DEFAULT_NEW(DynamicArrayTestDetail::st));
      }
      list.Sort();

      for (xiiUInt32 i = 1; i < list.GetCount(); i++)
      {
        XII_TEST_BOOL(list[i - 1] <= list[i]);
      }
    }

    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Various")
  {
    xiiDynamicArray<DynamicArrayTestDetail::Dummy> list;
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
    DynamicArrayTestDetail::Dummy d = list.PeekBack();
    list.PopBack();
    XII_TEST_BOOL(d.a == 5);
    XII_TEST_BOOL(list.GetCount() == 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Assignment")
  {
    xiiDynamicArray<DynamicArrayTestDetail::Dummy> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }

    xiiDynamicArray<DynamicArrayTestDetail::Dummy> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(DynamicArrayTestDetail::Dummy(rand()));
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
      list2.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    XII_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    XII_TEST_BOOL(list == list2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Count")
  {
    xiiDynamicArray<DynamicArrayTestDetail::Dummy> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(16);

    list.Compact();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reserve")
  {
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    xiiDynamicArray<DynamicArrayTestDetail::st> a;

    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    a.Reserve(100);

    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    a.SetCount(10);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(10, 0));

    a.Reserve(100);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0));

    a.SetCount(100);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(90, 0));

    a.Reserve(200);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 100)); // had to copy some elements over

    a.SetCount(200);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compact")
  {
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    xiiDynamicArray<DynamicArrayTestDetail::st> a;

    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    a.SetCount(100);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    a.SetCount(200);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(200, 100));

    a.SetCount(10);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 190));

    // no reallocations and copying, if the memory is already available
    a.SetCount(200);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(190, 0));

    a.SetCount(10);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 190));

    // now we remove the spare memory
    a.Compact();
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(10, 10));

    // this time the array needs to be relocated, and thus the already present elements need to be copied
    a.SetCount(200);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(200, 10));

    // this does not deallocate memory
    a.Clear();
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 200));

    a.SetCount(100);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    // therefore no object relocation
    a.SetCount(200);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    a.Clear();
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 200));

    // this will deallocate ALL memory
    XII_TEST_BOOL(a.GetHeapMemoryUsage() > 0);
    a.Compact();
    XII_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    a.SetCount(100);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    // this time objects need to be relocated
    a.SetCount(200);
    XII_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(200, 100));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "STL Iterator")
  {
    xiiDynamicArray<xiiInt32> a1;

    for (xiiInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    // STL sort
    std::sort(begin(a1), end(a1));

    for (xiiInt32 i = 1; i < 1000; ++i)
    {
      XII_TEST_BOOL(a1[i - 1] <= a1[i]);
    }

    // foreach
    xiiInt32 prev = 0;
    xiiInt32 sum1 = 0;
    for (xiiInt32 val : a1)
    {
      XII_TEST_BOOL(prev <= val);
      prev = val;
      sum1 += val;
    }

    prev             = 1000;
    const auto endIt = rend(a1);
    xiiInt32   sum2  = 0;
    for (auto it = rbegin(a1); it != endIt; ++it)
    {
      XII_TEST_BOOL(prev > (*it));
      prev = (*it);
      sum2 += (*it);
    }

    XII_TEST_BOOL(sum1 == sum2);

    // const array
    const xiiDynamicArray<xiiInt32>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(begin(a2), end(a2), 400);
    XII_TEST_BOOL(*lb == a2[400]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "STL Reverse Iterator")
  {
    xiiDynamicArray<xiiInt32> a1;

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
    const xiiDynamicArray<xiiInt32>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(a2), rend(a2), 400);
    XII_TEST_BOOL(*lb == a2[1000 - 400 - 1]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetArrayPtr")
  {
    xiiDynamicArray<xiiInt32> a1;
    a1.SetCountUninitialized(10);

    XII_TEST_BOOL(a1.GetArrayPtr().GetCount() == 10);
    XII_TEST_BOOL(a1.GetArrayPtr().GetPtr() == a1.GetData());

    const xiiDynamicArray<xiiInt32>& a1ref = a1;

    XII_TEST_BOOL(a1ref.GetArrayPtr().GetCount() == 10);
    XII_TEST_BOOL(a1ref.GetArrayPtr().GetPtr() == a1ref.GetData());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swap")
  {
    xiiDynamicArray<xiiInt32> a1, a2;

    xiiInt32 content1[] = {1, 2, 3, 4};
    xiiInt32 content2[] = {5, 6, 7, 8, 9};

    a1 = xiiMakeArrayPtr(content1);
    a2 = xiiMakeArrayPtr(content2);

    xiiInt32* a1Ptr = a1.GetData();
    xiiInt32* a2Ptr = a2.GetData();

    a1.Swap(a2);

    // The pointers should be simply swapped
    XII_TEST_BOOL(a2Ptr == a1.GetData());
    XII_TEST_BOOL(a1Ptr == a2.GetData());

    // The data should be swapped
    XII_TEST_BOOL(a1.GetArrayPtr() == xiiMakeArrayPtr(content2));
    XII_TEST_BOOL(a2.GetArrayPtr() == xiiMakeArrayPtr(content1));
  }

#if XII_ENABLED(XII_PLATFORM_64BIT)

  // disabled, because this is a very slow test
  XII_TEST_BLOCK(xiiTestBlock::DisabledNoWarning, "Large Allocation")
  {
    const xiiUInt32 uiMaxNumElements = 0xFFFFFFFF - 16; // max supported elements due to alignment restrictions

    // this will allocate about 16 GB memory, the pure allocation is really fast
    xiiDynamicArray<xiiUInt32> byteArray;
    byteArray.SetCountUninitialized(uiMaxNumElements);

    const xiiUInt32 uiCheckElements = byteArray.GetCount();
    const xiiUInt32 uiSkipElements  = 1024;

    // this will touch the memory and thus enforce that it is indeed made available by the OS
    // this takes a while
    for (xiiUInt64 i = 0; i < uiCheckElements; i += uiSkipElements)
    {
      const xiiUInt32 idx = i & 0xFFFFFFFF;
      byteArray[idx]      = idx;
    }

    // check that the assigned values are all correct
    // again, this takes quite a while
    for (xiiUInt64 i = 0; i < uiCheckElements; i += uiSkipElements)
    {
      const xiiUInt32 idx = i & 0xFFFFFFFF;
      XII_TEST_INT(byteArray[idx], idx);
    }
  }
#endif


  const xiiUInt32 uiNumSortItems = 1'000'000;

  struct Item
  {
    bool operator<(const Item& rhs) const { return m_iKey < rhs.m_iKey; }

    xiiInt32 m_iKey   = 0;
    xiiInt32 m_iIndex = 0;
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SortLargeArray (XII-Sort)")
  {
    xiiDynamicArray<Item> list;
    list.Reserve(uiNumSortItems);

    for (xiiUInt32 i = 0; i < uiNumSortItems; i++)
    {
      auto& item    = list.ExpandAndGetRef();
      item.m_iIndex = i;
      item.m_iKey   = std::rand();
    }

    xiiStopwatch sw;
    list.Sort();

    xiiTime          t = sw.GetRunningTotal();
    xiiStringBuilder s;
    s.SetFormat("XII-Sort (random keys): {}", t);
    xiiTestFramework::Output(xiiTestOutput::Details, s);

    for (xiiUInt32 i = 1; i < list.GetCount(); i++)
    {
      XII_TEST_BOOL(list[i - 1].m_iKey <= list[i].m_iKey);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SortLargeArray (std::sort)")
  {
    xiiDynamicArray<Item> list;
    list.Reserve(uiNumSortItems);

    for (xiiUInt32 i = 0; i < uiNumSortItems; i++)
    {
      auto& item    = list.ExpandAndGetRef();
      item.m_iIndex = i;
      item.m_iKey   = std::rand();
    }

    xiiStopwatch sw;
    std::sort(begin(list), end(list));

    xiiTime          t = sw.GetRunningTotal();
    xiiStringBuilder s;
    s.SetFormat("std::sort (random keys): {}", t);
    xiiTestFramework::Output(xiiTestOutput::Details, s);

    for (xiiUInt32 i = 1; i < list.GetCount(); i++)
    {
      XII_TEST_BOOL(list[i - 1].m_iKey <= list[i].m_iKey);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SortLargeArray (equal keys) (XII-Sort)")
  {
    xiiDynamicArray<Item> list;
    list.Reserve(uiNumSortItems);

    for (xiiUInt32 i = 0; i < uiNumSortItems; i++)
    {
      auto& item    = list.ExpandAndGetRef();
      item.m_iIndex = i;
      item.m_iKey   = 42;
    }

    xiiStopwatch sw;
    list.Sort();

    xiiTime          t = sw.GetRunningTotal();
    xiiStringBuilder s;
    s.SetFormat("XII-Sort (equal keys): {}", t);
    xiiTestFramework::Output(xiiTestOutput::Details, s);

    for (xiiUInt32 i = 1; i < list.GetCount(); i++)
    {
      XII_TEST_BOOL(list[i - 1].m_iKey <= list[i].m_iKey);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SortLargeArray (equal keys) (std::sort)")
  {
    xiiDynamicArray<Item> list;
    list.Reserve(uiNumSortItems);

    for (xiiUInt32 i = 0; i < uiNumSortItems; i++)
    {
      auto& item    = list.ExpandAndGetRef();
      item.m_iIndex = i;
      item.m_iKey   = 42;
    }

    xiiStopwatch sw;
    std::sort(begin(list), end(list));

    xiiTime          t = sw.GetRunningTotal();
    xiiStringBuilder s;
    s.SetFormat("std::sort (equal keys): {}", t);
    xiiTestFramework::Output(xiiTestOutput::Details, s);

    for (xiiUInt32 i = 1; i < list.GetCount(); i++)
    {
      XII_TEST_BOOL(list[i - 1].m_iKey <= list[i].m_iKey);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetCountUninitialized")
  {
    struct POD
    {
      XII_DECLARE_POD_TYPE();

      xiiUInt32 a = 2;
      xiiUInt32 b = 4;

      POD()
      {
        iCallPodConstructor++;
      }

      // This is not allowed anymore in types that use XII_DECLARE_POD_TYPE
      // unfortunately that means we cannot do this kind of check either
      // ~POD()
      // {
      //   iCallPodDestructor++;
      // }
    };

    static_assert(std::is_trivial<POD>::value == 0);
    static_assert(xiiIsPodType<POD>::value == 1);

    struct NonPOD
    {
      xiiUInt32 a = 3;
      xiiUInt32 b = 5;

      NonPOD()
      {
        iCallNonPodConstructor++;
      }

      ~NonPOD()
      {
        iCallNonPodDestructor++;
      }
    };

    static_assert(std::is_trivial<NonPOD>::value == 0);
    static_assert(xiiIsPodType<NonPOD>::value == 0);

    // check that SetCountUninitialized doesn't construct and Clear doesn't destruct POD types
    {
      xiiDynamicArray<POD> s1a;

      s1a.SetCountUninitialized(16);
      XII_TEST_INT(iCallPodConstructor, 0);
      XII_TEST_INT(iCallPodDestructor, 0);

      s1a.Clear();
      XII_TEST_INT(iCallPodConstructor, 0);
      XII_TEST_INT(iCallPodDestructor, 0);
    }

    // check that SetCount constructs and Clear destructs Non-POD types
    {
      xiiDynamicArray<NonPOD> s2a;

      s2a.SetCount(16);
      XII_TEST_INT(iCallNonPodConstructor, 16);
      XII_TEST_INT(iCallNonPodDestructor, 0);

      s2a.Clear();
      XII_TEST_INT(iCallNonPodConstructor, 16);
      XII_TEST_INT(iCallNonPodDestructor, 16);
    }
  }
}
