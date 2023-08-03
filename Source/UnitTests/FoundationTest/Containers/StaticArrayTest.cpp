#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/StaticArray.h>

namespace StaticArrayTestDetail
{
  class Dummy
  {
  public:
    int         a;
    std::string s;

    Dummy() :
      a(0), s("Test")
    {
    }
    Dummy(int a) :
      a(a), s("Test")
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
} // namespace StaticArrayTestDetail

#if XII_ENABLED(XII_PLATFORM_64BIT)
static_assert(sizeof(xiiStaticArray<xiiInt32, 1>) == 24);
#else
static_assert(sizeof(xiiStaticArray<xiiInt32, 1>) == 16);
#endif

static_assert(xiiGetTypeClass<xiiStaticArray<xiiInt32, 1>>::value == xiiTypeIsMemRelocatable::value);
static_assert(xiiGetTypeClass<xiiStaticArray<StaticArrayTestDetail::Dummy, 1>>::value == xiiTypeIsClass::value);

XII_CREATE_SIMPLE_TEST(Containers, StaticArray)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiStaticArray<xiiInt32, 32>               a1;
    xiiStaticArray<xiiConstructionCounter, 32> a2;

    XII_TEST_BOOL(a1.GetCount() == 0);
    XII_TEST_BOOL(a2.GetCount() == 0);
    XII_TEST_BOOL(a1.IsEmpty());
    XII_TEST_BOOL(a2.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy Constructor")
  {
    xiiStaticArray<xiiInt32, 32> a1;

    for (xiiInt32 i = 0; i < 32; ++i)
      a1.PushBack(rand() % 100000);

    xiiStaticArray<xiiInt32, 64> a2 = a1;
    xiiStaticArray<xiiInt32, 32> a3(a1);

    XII_TEST_BOOL(a1.GetArrayPtr() == a2.GetArrayPtr());
    XII_TEST_BOOL(a1 == a3);
    XII_TEST_BOOL(a2.GetArrayPtr() == a3.GetArrayPtr());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Convert to ArrayPtr")
  {
    xiiStaticArray<xiiInt32, 128> a1;

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
    xiiStaticArray<xiiInt32, 128> a1, a2;

    for (xiiInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    a2 = a1;

    XII_TEST_BOOL(a1 == a2);

    xiiArrayPtr<xiiInt32> arrayPtr(a1);

    a2 = arrayPtr;

    XII_TEST_BOOL(a2.GetArrayPtr() == arrayPtr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator == / !=")
  {
    xiiStaticArray<xiiInt32, 128> a1, a2;

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
    xiiStaticArray<xiiInt32, 128> a1;
    a1.SetCountUninitialized(100);

    for (xiiInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (xiiInt32 i = 0; i < 100; ++i)
      XII_TEST_INT(a1[i], i);

    const xiiStaticArray<xiiInt32, 128> ca1 = a1;

    for (xiiInt32 i = 0; i < 100; ++i)
      XII_TEST_INT(ca1[i], i);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    xiiStaticArray<xiiInt32, 128> a1;

    XII_TEST_BOOL(a1.IsEmpty());

    for (xiiInt32 i = 0; i < 128; ++i)
    {
      a1.SetCount(i + 1);
      XII_TEST_INT(a1[i], 0);
      a1[i] = i;

      XII_TEST_INT((int)a1.GetCount(), i + 1);
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
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear")
  {
    xiiStaticArray<xiiInt32, 128> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    XII_TEST_BOOL(a1.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    xiiStaticArray<xiiInt32, 128> a1;

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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert")
  {
    xiiStaticArray<xiiInt32, 128> a1;

    // always inserts at the front
    for (xiiInt32 i = 0; i < 100; ++i)
      a1.Insert(i, 0);

    for (xiiInt32 i = 0; i < 100; ++i)
      XII_TEST_INT(a1[i], 99 - i);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RemoveAndCopy")
  {
    xiiStaticArray<xiiInt32, 128> a1;

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
    xiiStaticArray<xiiInt32, 128> a1;

    for (xiiInt32 i = 0; i < 10; ++i)
      a1.Insert(i, i); // inserts at the end

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
    xiiStaticArray<xiiInt32, 128> a1;

    for (xiiInt32 i = 0; i < 10; ++i)
      a1.Insert(i, i); // inserts at the end

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
    xiiStaticArray<xiiInt32, 128> a1;

    for (xiiInt32 i = 0; i < 10; ++i)
      a1.Insert(i, i); // inserts at the end

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
    xiiStaticArray<xiiInt32, 128> a1;

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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Construction / Destruction")
  {
    {
      XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

      xiiStaticArray<xiiConstructionCounter, 128> a1;
      xiiStaticArray<xiiConstructionCounter, 100> a2;

      XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
      XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

      a1.PushBack(xiiConstructionCounter(1));
      XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a1.Insert(xiiConstructionCounter(2), 0);
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SortingPrimitives")
  {
    xiiStaticArray<xiiUInt32, 128> list;

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
    xiiStaticArray<StaticArrayTestDetail::Dummy, 128> list;

    for (xiiUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }
    list.Sort();

    StaticArrayTestDetail::Dummy last = 0;
    for (xiiUInt32 i = 0; i < list.GetCount(); i++)
    {
      XII_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Various")
  {
    xiiStaticArray<StaticArrayTestDetail::Dummy, 32> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    list.Insert(4, 3);
    list.Insert(0, 1);
    list.Insert(0, 5);

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
    StaticArrayTestDetail::Dummy d = list.PeekBack();
    list.PopBack();
    XII_TEST_BOOL(d.a == 5);
    XII_TEST_BOOL(list.GetCount() == 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Assignment")
  {
    xiiStaticArray<StaticArrayTestDetail::Dummy, 32> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }

    xiiStaticArray<StaticArrayTestDetail::Dummy, 32> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(StaticArrayTestDetail::Dummy(rand()));
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
      list2.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    XII_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    XII_TEST_BOOL(list == list2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Count")
  {
    xiiStaticArray<StaticArrayTestDetail::Dummy, 32> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "STL Iterator")
  {
    xiiStaticArray<xiiInt32, 1024> a1;

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
    const xiiStaticArray<xiiInt32, 1024>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(begin(a2), end(a2), 400);
    XII_TEST_BOOL(*lb == a2[400]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "STL Reverse Iterator")
  {
    xiiStaticArray<xiiInt32, 1024> a1;

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
    const xiiStaticArray<xiiInt32, 1024>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(a2), rend(a2), 400);
    XII_TEST_BOOL(*lb == a2[1000 - 400 - 1]);
  }
}
