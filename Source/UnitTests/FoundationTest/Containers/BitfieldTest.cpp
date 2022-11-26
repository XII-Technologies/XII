#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>

XII_CREATE_SIMPLE_TEST(Containers, Bitfield)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetCount / IsEmpty / Clear")
  {
    xiiDynamicBitfield bf; // using a dynamic array

    XII_TEST_INT(bf.GetCount(), 0);
    XII_TEST_BOOL(bf.IsEmpty());

    bf.SetCount(15, false);

    XII_TEST_INT(bf.GetCount(), 15);
    XII_TEST_BOOL(!bf.IsEmpty());

    bf.Clear();

    XII_TEST_INT(bf.GetCount(), 0);
    XII_TEST_BOOL(bf.IsEmpty());

    bf.SetCount(37, false);

    XII_TEST_INT(bf.GetCount(), 37);
    XII_TEST_BOOL(!bf.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetCount / SetAllBits / ClearAllBits")
  {
    xiiHybridBitfield<512> bf; // using a hybrid array

    bf.SetCount(249, false);
    XII_TEST_INT(bf.GetCount(), 249);

    for (xiiUInt32 i = 0; i < bf.GetCount(); ++i)
      XII_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetAllBits();
    XII_TEST_INT(bf.GetCount(), 249);

    for (xiiUInt32 i = 0; i < bf.GetCount(); ++i)
      XII_TEST_BOOL(bf.IsBitSet(i));

    bf.ClearAllBits();
    XII_TEST_INT(bf.GetCount(), 249);

    for (xiiUInt32 i = 0; i < bf.GetCount(); ++i)
      XII_TEST_BOOL(!bf.IsBitSet(i));


    bf.SetCount(349, true);
    XII_TEST_INT(bf.GetCount(), 349);

    for (xiiUInt32 i = 0; i < 249; ++i)
      XII_TEST_BOOL(!bf.IsBitSet(i));

    for (xiiUInt32 i = 249; i < bf.GetCount(); ++i)
      XII_TEST_BOOL(bf.IsBitSet(i));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetCount / SetBit / ClearBit / SetCountUninitialized")
  {
    xiiHybridBitfield<512> bf; // using a hybrid array

    bf.SetCount(100, false);
    XII_TEST_INT(bf.GetCount(), 100);

    for (xiiUInt32 i = 0; i < bf.GetCount(); ++i)
      XII_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetCount(200, true);
    XII_TEST_INT(bf.GetCount(), 200);

    for (xiiUInt32 i = 100; i < bf.GetCount(); ++i)
      XII_TEST_BOOL(bf.IsBitSet(i));

    bf.SetCountUninitialized(250);
    XII_TEST_INT(bf.GetCount(), 250);

    bf.ClearAllBits();

    for (xiiUInt32 i = 0; i < bf.GetCount(); i += 2)
      bf.SetBit(i);

    for (xiiUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      XII_TEST_BOOL(bf.IsBitSet(i));
      XII_TEST_BOOL(!bf.IsBitSet(i + 1));
    }

    for (xiiUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      bf.ClearBit(i);
      bf.SetBit(i + 1);
    }

    for (xiiUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      XII_TEST_BOOL(!bf.IsBitSet(i));
      XII_TEST_BOOL(bf.IsBitSet(i + 1));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetBitRange")
  {
    for (xiiUInt32 size = 1; size < 1024; ++size)
    {
      xiiBitfield<xiiDeque<xiiUInt32>> bf; // using a deque
      bf.SetCount(size, false);

      XII_TEST_INT(bf.GetCount(), size);

      for (xiiUInt32 count = 0; count < bf.GetCount(); ++count)
        XII_TEST_BOOL(!bf.IsBitSet(count));

      xiiUInt32 uiStart = size / 2;
      xiiUInt32 uiEnd   = xiiMath::Min(uiStart + (size / 3 * 2), size - 1);

      bf.SetBitRange(uiStart, uiEnd - uiStart + 1);

      for (xiiUInt32 count = 0; count < uiStart; ++count)
        XII_TEST_BOOL(!bf.IsBitSet(count));
      for (xiiUInt32 count = uiStart; count <= uiEnd; ++count)
        XII_TEST_BOOL(bf.IsBitSet(count));
      for (xiiUInt32 count = uiEnd + 1; count < bf.GetCount(); ++count)
        XII_TEST_BOOL(!bf.IsBitSet(count));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ClearBitRange")
  {
    for (xiiUInt32 size = 1; size < 1024; ++size)
    {
      xiiBitfield<xiiDeque<xiiUInt32>> bf; // using a deque
      bf.SetCount(size, true);

      XII_TEST_INT(bf.GetCount(), size);

      for (xiiUInt32 count = 0; count < bf.GetCount(); ++count)
        XII_TEST_BOOL(bf.IsBitSet(count));

      xiiUInt32 uiStart = size / 2;
      xiiUInt32 uiEnd   = xiiMath::Min(uiStart + (size / 3 * 2), size - 1);

      bf.ClearBitRange(uiStart, uiEnd - uiStart + 1);

      for (xiiUInt32 count = 0; count < uiStart; ++count)
        XII_TEST_BOOL(bf.IsBitSet(count));
      for (xiiUInt32 count = uiStart; count <= uiEnd; ++count)
        XII_TEST_BOOL(!bf.IsBitSet(count));
      for (xiiUInt32 count = uiEnd + 1; count < bf.GetCount(); ++count)
        XII_TEST_BOOL(bf.IsBitSet(count));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsAnyBitSet / IsNoBitSet / AreAllBitsSet")
  {
    xiiHybridBitfield<512> bf; // using a hybrid array

    XII_TEST_BOOL(bf.IsEmpty() == true);
    XII_TEST_BOOL(bf.IsAnyBitSet() == false); // empty
    XII_TEST_BOOL(bf.IsNoBitSet() == true);
    XII_TEST_BOOL(bf.AreAllBitsSet() == false); // empty

    bf.SetCount(250, false);

    XII_TEST_BOOL(bf.IsEmpty() == false);
    XII_TEST_BOOL(bf.IsAnyBitSet() == false);
    XII_TEST_BOOL(bf.IsNoBitSet() == true);
    XII_TEST_BOOL(bf.AreAllBitsSet() == false);

    for (xiiUInt32 i = 0; i < bf.GetCount(); i += 2)
      bf.SetBit(i);

    XII_TEST_BOOL(bf.IsEmpty() == false);
    XII_TEST_BOOL(bf.IsAnyBitSet() == true);
    XII_TEST_BOOL(bf.IsNoBitSet() == false);
    XII_TEST_BOOL(bf.AreAllBitsSet() == false);

    for (xiiUInt32 i = 0; i < bf.GetCount(); i++)
      bf.SetBit(i);

    XII_TEST_BOOL(bf.IsAnyBitSet() == true);
    XII_TEST_BOOL(bf.IsNoBitSet() == false);
    XII_TEST_BOOL(bf.AreAllBitsSet() == true);
  }
}


XII_CREATE_SIMPLE_TEST(Containers, StaticBitfield)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetAllBits / ClearAllBits")
  {
    xiiStaticBitfield64 bf;

    for (xiiUInt32 i = 0; i < bf.GetNumBits(); ++i)
      XII_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetAllBits();

    for (xiiUInt32 i = 0; i < bf.GetNumBits(); ++i)
      XII_TEST_BOOL(bf.IsBitSet(i));

    bf.ClearAllBits();

    for (xiiUInt32 i = 0; i < bf.GetNumBits(); ++i)
      XII_TEST_BOOL(!bf.IsBitSet(i));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetBit / ClearBit")
  {
    xiiStaticBitfield32 bf;

    for (xiiUInt32 i = 0; i < bf.GetNumBits(); ++i)
      XII_TEST_BOOL(!bf.IsBitSet(i));

    for (xiiUInt32 i = 0; i < bf.GetNumBits(); i += 2)
      bf.SetBit(i);

    for (xiiUInt32 i = 0; i < bf.GetNumBits(); i += 2)
    {
      XII_TEST_BOOL(bf.IsBitSet(i));
      XII_TEST_BOOL(!bf.IsBitSet(i + 1));
    }

    for (xiiUInt32 i = 0; i < bf.GetNumBits(); i += 2)
    {
      bf.ClearBit(i);
      bf.SetBit(i + 1);
    }

    for (xiiUInt32 i = 0; i < bf.GetNumBits(); i += 2)
    {
      XII_TEST_BOOL(!bf.IsBitSet(i));
      XII_TEST_BOOL(bf.IsBitSet(i + 1));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetBitRange")
  {
    for (xiiUInt32 uiStart = 0; uiStart < 61; ++uiStart)
    {
      xiiStaticBitfield64 bf;

      for (xiiUInt32 count = 0; count < bf.GetNumBits(); ++count)
        XII_TEST_BOOL(!bf.IsBitSet(count));

      xiiUInt32 uiEnd = uiStart + 3;

      bf.SetBitRange(uiStart, uiEnd - uiStart + 1);

      for (xiiUInt32 count = 0; count < uiStart; ++count)
        XII_TEST_BOOL(!bf.IsBitSet(count));
      for (xiiUInt32 count = uiStart; count <= uiEnd; ++count)
        XII_TEST_BOOL(bf.IsBitSet(count));
      for (xiiUInt32 count = uiEnd + 1; count < bf.GetNumBits(); ++count)
        XII_TEST_BOOL(!bf.IsBitSet(count));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ClearBitRange")
  {
    for (xiiUInt32 uiStart = 0; uiStart < 61; ++uiStart)
    {
      xiiStaticBitfield64 bf;
      bf.SetAllBits();

      for (xiiUInt32 count = 0; count < bf.GetNumBits(); ++count)
        XII_TEST_BOOL(bf.IsBitSet(count));

      xiiUInt32 uiEnd = uiStart + 3;

      bf.ClearBitRange(uiStart, uiEnd - uiStart + 1);

      for (xiiUInt32 count = 0; count < uiStart; ++count)
        XII_TEST_BOOL(bf.IsBitSet(count));
      for (xiiUInt32 count = uiStart; count <= uiEnd; ++count)
        XII_TEST_BOOL(!bf.IsBitSet(count));
      for (xiiUInt32 count = uiEnd + 1; count < bf.GetNumBits(); ++count)
        XII_TEST_BOOL(bf.IsBitSet(count));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsAnyBitSet / IsNoBitSet / AreAllBitsSet")
  {
    xiiStaticBitfield8 bf;

    XII_TEST_BOOL(bf.IsAnyBitSet() == false); // empty
    XII_TEST_BOOL(bf.IsNoBitSet() == true);
    XII_TEST_BOOL(bf.AreAllBitsSet() == false); // empty

    for (xiiUInt32 i = 0; i < bf.GetNumBits(); i += 2)
      bf.SetBit(i);

    XII_TEST_BOOL(bf.IsAnyBitSet() == true);
    XII_TEST_BOOL(bf.IsNoBitSet() == false);
    XII_TEST_BOOL(bf.AreAllBitsSet() == false);

    for (xiiUInt32 i = 0; i < bf.GetNumBits(); i++)
      bf.SetBit(i);

    XII_TEST_BOOL(bf.IsAnyBitSet() == true);
    XII_TEST_BOOL(bf.IsNoBitSet() == false);
    XII_TEST_BOOL(bf.AreAllBitsSet() == true);
  }
}
