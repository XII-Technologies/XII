#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/StaticRingBuffer.h>

typedef xiiConstructionCounter cc;

XII_CREATE_SIMPLE_TEST(Containers, StaticRingBuffer)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    {
      xiiStaticRingBuffer<xiiInt32, 32> r1;
      xiiStaticRingBuffer<xiiInt32, 16> r2;
      xiiStaticRingBuffer<cc, 2>        r3;
    }

    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy Constructor / Operator=")
  {
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    {
      xiiStaticRingBuffer<cc, 16> r1;

      for (xiiUInt32 i = 0; i < 16; ++i)
        r1.PushBack(cc(i));

      xiiStaticRingBuffer<cc, 16> r2(r1);

      for (xiiUInt32 i = 0; i < 16; ++i)
        XII_TEST_BOOL(r2[i] == cc(i));

      xiiStaticRingBuffer<cc, 16> r3;
      r3 = r1;

      for (xiiUInt32 i = 0; i < 16; ++i)
        XII_TEST_BOOL(r3[i] == cc(i));
    }

    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Operator==")
  {
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    {
      xiiStaticRingBuffer<cc, 16> r1;

      for (xiiUInt32 i = 0; i < 16; ++i)
        r1.PushBack(cc(i));

      xiiStaticRingBuffer<cc, 16> r2(r1);
      xiiStaticRingBuffer<cc, 16> r3(r1);
      r3.PeekFront() = cc(3);

      XII_TEST_BOOL(r1 == r1);
      XII_TEST_BOOL(r2 == r2);
      XII_TEST_BOOL(r3 == r3);

      XII_TEST_BOOL(r1 == r2);
      XII_TEST_BOOL(r1 != r3);
      XII_TEST_BOOL(r2 != r3);
    }

    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PushBack / operator[] / CanAppend")
  {
    xiiStaticRingBuffer<xiiInt32, 16> r;

    for (xiiUInt32 i = 0; i < 16; ++i)
    {
      XII_TEST_BOOL(r.CanAppend());
      r.PushBack(i);
    }

    XII_TEST_BOOL(!r.CanAppend());

    for (xiiUInt32 i = 0; i < 16; ++i)
      XII_TEST_INT(r[i], i);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetCount / IsEmpty")
  {
    xiiStaticRingBuffer<xiiInt32, 16> r;

    XII_TEST_BOOL(r.IsEmpty());

    for (xiiUInt32 i = 0; i < 16; ++i)
    {
      XII_TEST_INT(r.GetCount(), i);
      r.PushBack(i);
      XII_TEST_INT(r.GetCount(), i + 1);

      XII_TEST_BOOL(!r.IsEmpty());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear / IsEmpty")
  {
    xiiStaticRingBuffer<xiiInt32, 16> r;

    XII_TEST_BOOL(r.IsEmpty());

    for (xiiUInt32 i = 0; i < 16; ++i)
      r.PushBack(i);

    XII_TEST_BOOL(!r.IsEmpty());

    r.Clear();

    XII_TEST_BOOL(r.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Cycle Items / PeekFront")
  {
    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());

    {
      xiiStaticRingBuffer<xiiConstructionCounter, 16> r;

      for (xiiUInt32 i = 0; i < 16; ++i)
      {
        r.PushBack(xiiConstructionCounter(i));
        XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 1)); // one temporary
      }

      for (xiiUInt32 i = 16; i < 1000; ++i)
      {
        XII_TEST_BOOL(r.PeekFront() == xiiConstructionCounter(i - 16));
        XII_TEST_BOOL(xiiConstructionCounter::HasDone(1, 1)); // one temporary

        XII_TEST_BOOL(!r.CanAppend());

        r.PopFront();
        XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 1));

        XII_TEST_BOOL(r.CanAppend());

        r.PushBack(xiiConstructionCounter(i));
        XII_TEST_BOOL(xiiConstructionCounter::HasDone(2, 1)); // one temporary
      }

      for (xiiUInt32 i = 1000; i < 1016; ++i)
      {
        XII_TEST_BOOL(r.PeekFront() == xiiConstructionCounter(i - 16));
        XII_TEST_BOOL(xiiConstructionCounter::HasDone(1, 1)); // one temporary

        r.PopFront();
        XII_TEST_BOOL(xiiConstructionCounter::HasDone(0, 1)); // one temporary
      }

      XII_TEST_BOOL(r.IsEmpty());
    }

    XII_TEST_BOOL(xiiConstructionCounter::HasAllDestructed());
  }
}
