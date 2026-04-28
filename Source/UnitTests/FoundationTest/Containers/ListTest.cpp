/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/List.h>

XII_CREATE_SIMPLE_TEST(Containers, List)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiList<xiiInt32> l;
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PushBack() / PeekBack")
  {
    xiiList<xiiInt32> l;
    xiiInt32&         val = l.PushBack();

    XII_TEST_INT(val, 0);
    XII_TEST_INT(l.GetCount(), 1);
    XII_TEST_INT(l.PeekBack(), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PushBack(i) / GetCount")
  {
    xiiList<xiiInt32> l;
    XII_TEST_BOOL(l.GetHeapMemoryUsage() == 0);

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      l.PushBack(i);

      XII_TEST_INT(l.GetCount(), i + 1);
      XII_TEST_INT(l.PeekBack(), i);
    }

    XII_TEST_BOOL(l.GetHeapMemoryUsage() >= sizeof(xiiInt32) * 1000);

    xiiUInt32 i = 0;
    for (xiiList<xiiInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      XII_TEST_INT(*it, i);
      ++i;
    }

    XII_TEST_INT(i, 1000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PopBack()")
  {
    xiiList<xiiInt32> l;

    xiiInt32 i = 0;
    for (; i < 1000; ++i)
      l.PushBack(i);

    while (!l.IsEmpty())
    {
      --i;
      XII_TEST_INT(l.PeekBack(), i);
      l.PopBack();
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PushFront() / PeekFront")
  {
    xiiList<xiiInt32> l;
    xiiInt32&         val = l.PushFront();

    XII_TEST_INT(val, 0);
    XII_TEST_INT(l.GetCount(), 1);
    XII_TEST_INT(l.PeekFront(), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PushFront(i) / PeekFront")
  {
    xiiList<xiiInt32> l;

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      l.PushFront(i);

      XII_TEST_INT(l.GetCount(), i + 1);
      XII_TEST_INT(l.PeekFront(), i);
    }

    xiiUInt32 i2 = 1000;
    for (xiiList<xiiInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      --i2;
      XII_TEST_INT(*it, i2);
    }

    XII_TEST_INT(i2, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PopFront()")
  {
    xiiList<xiiInt32> l;

    xiiInt32 i = 0;
    for (; i < 1000; ++i)
      l.PushFront(i);

    while (!l.IsEmpty())
    {
      --i;
      XII_TEST_INT(l.PeekFront(), i);
      l.PopFront();
    }
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear / IsEmpty")
  {
    xiiList<xiiInt32> l;

    XII_TEST_BOOL(l.IsEmpty());

    for (xiiUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    XII_TEST_BOOL(!l.IsEmpty());

    l.Clear();
    XII_TEST_BOOL(l.IsEmpty());

    for (xiiUInt32 i = 0; i < 1000; ++i)
    {
      l.PushBack(i);
      XII_TEST_BOOL(!l.IsEmpty());

      l.Clear();
      XII_TEST_BOOL(l.IsEmpty());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator=")
  {
    xiiList<xiiInt32> l, l2;

    for (xiiUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    l2 = l;

    xiiUInt32 i = 0;
    for (xiiList<xiiInt32>::Iterator it = l2.GetIterator(); it != l2.GetEndIterator(); ++it)
    {
      XII_TEST_INT(*it, i);
      ++i;
    }

    XII_TEST_INT(i, 1000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy Constructor")
  {
    xiiList<xiiInt32> l;

    for (xiiUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    xiiList<xiiInt32> l2(l);

    xiiUInt32 i = 0;
    for (xiiList<xiiInt32>::Iterator it = l2.GetIterator(); it != l2.GetEndIterator(); ++it)
    {
      XII_TEST_INT(*it, i);
      ++i;
    }

    XII_TEST_INT(i, 1000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetCount")
  {
    xiiList<xiiInt32> l;
    l.SetCount(1000);
    XII_TEST_INT(l.GetCount(), 1000);

    xiiInt32 i = 1;
    for (xiiList<xiiInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      XII_TEST_INT(*it, 0);
      *it = i;
      ++i;
    }

    l.SetCount(2000);
    i = 1;
    for (xiiList<xiiInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      if (i > 1000)
        XII_TEST_INT(*it, 0);
      else
        XII_TEST_INT(*it, i);

      ++i;
    }

    l.SetCount(500);
    i = 1;
    for (xiiList<xiiInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      XII_TEST_INT(*it, i);
      ++i;
    }

    XII_TEST_INT(i, 501);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Insert(item)")
  {
    xiiList<xiiInt32> l;

    for (xiiUInt32 i = 1; i < 1000; ++i)
      l.PushBack(i);

    // create an interleaved array of values of i and i+10000
    for (xiiList<xiiInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      // insert before this element
      l.Insert(it, *it + 10000);
    }

    xiiInt32 i = 1;
    for (xiiList<xiiInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      XII_TEST_INT(*it, i + 10000);
      ++it;

      XII_TEST_BOOL(it.IsValid());
      XII_TEST_INT(*it, i);

      ++i;
    }

    XII_TEST_INT(i, 1000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Remove(item)")
  {
    xiiList<xiiInt32> l;

    xiiUInt32 i = 1;
    for (; i < 1000; ++i)
      l.PushBack(i);

    // create an interleaved array of values of i and i+10000
    for (xiiList<xiiInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      // insert before this element
      l.Insert(it, *it + 10000);
    }

    i = 1;

    // now remove every second element and only keep the larger values
    for (xiiList<xiiInt32>::Iterator it = l.GetIterator(); it.IsValid();)
    {
      XII_TEST_INT(*it, i + 10000);

      ++it;
      it = l.Remove(it);
      ++i;
    }

    i = 1;
    for (xiiList<xiiInt32>::Iterator it = l.GetIterator(); it.IsValid(); ++it)
    {
      XII_TEST_INT(*it, i + 10000);
      ++i;
    }

    XII_TEST_INT(i, 1000);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Iterator::IsValid")
  {
    xiiList<xiiInt32> l;

    for (xiiUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    xiiUInt32 i = 0;
    for (xiiList<xiiInt32>::Iterator it = l.GetIterator(); it.IsValid(); ++it)
    {
      XII_TEST_INT(*it, i);
      ++i;
    }

    XII_TEST_BOOL(!l.GetEndIterator().IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Element Constructions / Destructions")
  {
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    xiiList<xiiConstructionCounter> l;

    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    l.PushBack();
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(1, 0));

    l.PushBack(xiiConstructionCounter(1));
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 1));

    l.SetCount(4);
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 0));

    l.Clear();
    XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 4));

    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator == / !=")
  {
    xiiList<xiiInt32> l, l2;

    XII_TEST_BOOL(l == l2);

    xiiInt32 i = 0;
    for (; i < 1000; ++i)
      l.PushBack(i);

    XII_TEST_BOOL(l != l2);

    l2 = l;

    XII_TEST_BOOL(l == l2);
  }
}
