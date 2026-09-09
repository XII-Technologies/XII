/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Random.h>
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetCount / SetBit / FlipBit / ClearBit / SetBitValue / SetCountUninitialized")
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

    for (xiiUInt32 i = 0; i < bf.GetCount(); ++i)
    {
      bf.SetBitValue(i, (i % 3) == 0);
    }

    for (xiiUInt32 i = 0; i < bf.GetCount(); ++i)
    {
      XII_TEST_BOOL(bf.IsBitSet(i) == ((i % 3) == 0));
    }

    for (xiiUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      bf.FlipBit(i);
    }

    for (xiiUInt32 i = 0; i < bf.GetCount(); ++i)
    {
      XII_TEST_BOOL(bf.IsBitSet(i) == (((0b011100 >> (i % 6)) & 1) == 1));
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FlipBitRange")
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

      bf.FlipBitRange(uiStart, uiEnd - uiStart + 1);

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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Swap")
  {
    xiiHybridBitfield<512> bf0; // using a hybrid array
    xiiHybridBitfield<512> bf1; // using a hybrid array

    xiiUInt32 bitFieldCount0 = 100;
    xiiUInt32 bitFieldCount1 = 999;
    xiiUInt32 bitIndexSet0   = 2;
    xiiUInt32 bitIndexSet1   = 555;

    bf0.SetCount(bitFieldCount0, false);
    bf1.SetCount(bitFieldCount1, false);
    bf0.SetBit(bitIndexSet0);
    bf1.SetBit(bitIndexSet1);

    XII_TEST_BOOL(bitFieldCount0 == bf0.GetCount());
    for (xiiUInt32 i = 0; i < bf0.GetCount(); ++i)
    {
      XII_TEST_BOOL(bf0.IsBitSet(i) == (bitIndexSet0 == i));
    }

    XII_TEST_BOOL(bitFieldCount1 == bf1.GetCount());
    for (xiiUInt32 i = 0; i < bf1.GetCount(); ++i)
    {
      XII_TEST_BOOL(bf1.IsBitSet(i) == (bitIndexSet1 == i));
    }

    bf0.Swap(bf1);
    xiiMath::Swap(bitIndexSet0, bitIndexSet1);
    xiiMath::Swap(bitFieldCount0, bitFieldCount1);

    XII_TEST_BOOL(bitFieldCount0 == bf0.GetCount());
    for (xiiUInt32 i = 0; i < bf0.GetCount(); ++i)
    {
      XII_TEST_BOOL(bf0.IsBitSet(i) == (bitIndexSet0 == i));
    }

    XII_TEST_BOOL(bitFieldCount1 == bf1.GetCount());
    for (xiiUInt32 i = 0; i < bf1.GetCount(); ++i)
    {
      XII_TEST_BOOL(bf1.IsBitSet(i) == (bitIndexSet1 == i));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Iterator")
  {
    {
      // Check empty bitfields of varying sizes.
      for (xiiUInt32 uiNumBits = 0; uiNumBits <= 65; ++uiNumBits)
      {
        xiiHybridBitfield<128> bitfield;
        bitfield.SetCount(uiNumBits, true);
        for (xiiUInt32 b = 0; b < uiNumBits; ++b)
        {
          bitfield.ClearBit(b);
        }
        for (xiiUInt32 uiBit : bitfield)
        {
          XII_TEST_BOOL_MSG(false, "No bit should be set");
        }

        for (auto it = bitfield.GetIterator(); it.IsValid(); it.Next())
        {
          XII_TEST_BOOL_MSG(false, "No bit should be set");
        }
        XII_TEST_BOOL(bitfield.GetIterator() == bitfield.GetEndIterator());
        XII_TEST_BOOL(!bitfield.GetIterator().IsValid());
        XII_TEST_BOOL(!bitfield.GetEndIterator().IsValid());
      }
    }

    {
      // Full bits.
      for (xiiUInt32 uiNumBits = 0; uiNumBits <= 65; ++uiNumBits)
      {
        xiiHybridBitfield<128> bitfield;
        bitfield.SetCount(uiNumBits, true);
        xiiUInt32 uiNextBit = 0;
        for (xiiUInt32 uiBit : bitfield)
        {
          XII_TEST_INT(uiBit, uiNextBit);
          uiNextBit++;
        }
        XII_TEST_INT(uiNumBits, uiNextBit);

        uiNextBit = 0;
        for (auto it = bitfield.GetIterator(); it.IsValid(); ++it)
        {
          XII_TEST_INT(it.Value(), uiNextBit);
          XII_TEST_INT(*it, uiNextBit);
          XII_TEST_BOOL(it.IsValid());
          uiNextBit++;
        }
        XII_TEST_INT(uiNumBits, uiNextBit);
      }
    }

    {
      // Partial bits set.
      xiiRandom rnd;
      rnd.Initialize(42);

      for (xiiUInt32 uiNumBits = 2; uiNumBits <= 65; ++uiNumBits)
      {
        xiiHybridBitfield<128> bitfield;
        bitfield.SetCount(uiNumBits, false);

        // Add some random bits and ensure they appear in the iterator in order.
        xiiHybridArray<xiiUInt32, 3> bits;
        for (int i = 0; i < uiNumBits / 2; ++i)
        {
          xiiUInt32 bit = (xiiUInt32)rnd.IntMinMax(0, uiNumBits - 1);
          if (!bitfield.IsBitSet(bit))
          {
            bits.PushBack(bit);
            bitfield.SetBit(bit);
          }
        }
        bits.Sort();

        for (xiiUInt32 uiBit : bitfield)
        {
          XII_TEST_INT(uiBit, bits[0]);
          bits.RemoveAtAndCopy(0);
        }
        XII_TEST_BOOL(bits.IsEmpty());
      }
    }
  }
}


XII_CREATE_SIMPLE_TEST(Containers, StaticBitfield)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetAllBits / ClearAllBits")
  {
    xiiStaticBitfield64 bf;

    for (xiiUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
      XII_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetAllBits();

    for (xiiUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
      XII_TEST_BOOL(bf.IsBitSet(i));

    bf.ClearAllBits();

    for (xiiUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
      XII_TEST_BOOL(!bf.IsBitSet(i));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetBit / ClearBit / SetBitValue")
  {
    xiiStaticBitfield32 bf;

    for (xiiUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
      XII_TEST_BOOL(!bf.IsBitSet(i));

    for (xiiUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
      bf.SetBit(i);

    for (xiiUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
    {
      XII_TEST_BOOL(bf.IsBitSet(i));
      XII_TEST_BOOL(!bf.IsBitSet(i + 1));
    }

    for (xiiUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
    {
      bf.ClearBit(i);
      bf.SetBit(i + 1);
    }

    for (xiiUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
    {
      XII_TEST_BOOL(!bf.IsBitSet(i));
      XII_TEST_BOOL(bf.IsBitSet(i + 1));
    }

    for (xiiUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
    {
      bf.SetBitValue(i, (i % 3) == 0);
    }

    for (xiiUInt32 i = 0; i < bf.GetStorageTypeBitCount(); ++i)
    {
      XII_TEST_BOOL(bf.IsBitSet(i) == ((i % 3) == 0));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetBitRange")
  {
    for (xiiUInt32 uiStart = 0; uiStart < 61; ++uiStart)
    {
      xiiStaticBitfield64 bf;

      for (xiiUInt32 count = 0; count < bf.GetStorageTypeBitCount(); ++count)
        XII_TEST_BOOL(!bf.IsBitSet(count));

      xiiUInt32 uiEnd = uiStart + 3;

      bf.SetBitRange(uiStart, uiEnd - uiStart + 1);

      for (xiiUInt32 count = 0; count < uiStart; ++count)
        XII_TEST_BOOL(!bf.IsBitSet(count));
      for (xiiUInt32 count = uiStart; count <= uiEnd; ++count)
        XII_TEST_BOOL(bf.IsBitSet(count));
      for (xiiUInt32 count = uiEnd + 1; count < bf.GetStorageTypeBitCount(); ++count)
        XII_TEST_BOOL(!bf.IsBitSet(count));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ClearBitRange")
  {
    for (xiiUInt32 uiStart = 0; uiStart < 61; ++uiStart)
    {
      xiiStaticBitfield64 bf;
      bf.SetAllBits();

      for (xiiUInt32 count = 0; count < bf.GetStorageTypeBitCount(); ++count)
        XII_TEST_BOOL(bf.IsBitSet(count));

      xiiUInt32 uiEnd = uiStart + 3;

      bf.ClearBitRange(uiStart, uiEnd - uiStart + 1);

      for (xiiUInt32 count = 0; count < uiStart; ++count)
        XII_TEST_BOOL(bf.IsBitSet(count));
      for (xiiUInt32 count = uiStart; count <= uiEnd; ++count)
        XII_TEST_BOOL(!bf.IsBitSet(count));
      for (xiiUInt32 count = uiEnd + 1; count < bf.GetStorageTypeBitCount(); ++count)
        XII_TEST_BOOL(bf.IsBitSet(count));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsAnyBitSet / IsNoBitSet / AreAllBitsSet")
  {
    xiiStaticBitfield8 bf;

    XII_TEST_BOOL(bf.IsAnyBitSet() == false); // empty
    XII_TEST_BOOL(bf.IsNoBitSet() == true);
    XII_TEST_BOOL(bf.AreAllBitsSet() == false); // empty

    for (xiiUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i += 2)
      bf.SetBit(i);

    XII_TEST_BOOL(bf.IsAnyBitSet() == true);
    XII_TEST_BOOL(bf.IsNoBitSet() == false);
    XII_TEST_BOOL(bf.AreAllBitsSet() == false);

    for (xiiUInt32 i = 0; i < bf.GetStorageTypeBitCount(); i++)
      bf.SetBit(i);

    XII_TEST_BOOL(bf.IsAnyBitSet() == true);
    XII_TEST_BOOL(bf.IsNoBitSet() == false);
    XII_TEST_BOOL(bf.AreAllBitsSet() == true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetNumBitsSet")
  {
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0).GetNumBitsSet(), 0);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0xff).GetNumBitsSet(), 8);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0xffff).GetNumBitsSet(), 16);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0xffffffffu).GetNumBitsSet(), 32);
    XII_TEST_INT(xiiStaticBitfield64::MakeFromMask(0).GetNumBitsSet(), 0);
    XII_TEST_INT(xiiStaticBitfield64::MakeFromMask(0xff).GetNumBitsSet(), 8);
    XII_TEST_INT(xiiStaticBitfield64::MakeFromMask(0xffff).GetNumBitsSet(), 16);
    XII_TEST_INT(xiiStaticBitfield64::MakeFromMask(0xffffffffu).GetNumBitsSet(), 32);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetLowestBitSet")
  {
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0u).GetLowestBitSet(), 32);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(1u).GetLowestBitSet(), 0);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0xffu).GetLowestBitSet(), 0);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0xff00u).GetLowestBitSet(), 8);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0xff0000u).GetLowestBitSet(), 16);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0xff000000u).GetLowestBitSet(), 24);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0x80000000u).GetLowestBitSet(), 31);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0xffffffffu).GetLowestBitSet(), 0);
    XII_TEST_INT(xiiStaticBitfield64::MakeFromMask(0xffffffffffffffffull).GetLowestBitSet(), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetHighestBitSet")
  {
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0u).GetHighestBitSet(), 32);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(1u).GetHighestBitSet(), 0);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0xffu).GetHighestBitSet(), 7);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0xff00u).GetHighestBitSet(), 15);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0xff0000u).GetHighestBitSet(), 23);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0xff000000u).GetHighestBitSet(), 31);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0x80000000u).GetHighestBitSet(), 31);
    XII_TEST_INT(xiiStaticBitfield32::MakeFromMask(0xffffffffu).GetHighestBitSet(), 31);
    XII_TEST_INT(xiiStaticBitfield64::MakeFromMask(0xffffffffffffffffull).GetHighestBitSet(), 63);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Iterator")
  {
    {
      // Empty bitfield
      xiiStaticBitfield32 bitfield = xiiStaticBitfield32::MakeFromMask(0u);
      for (xiiUInt32 uiBit : bitfield)
      {
        XII_TEST_BOOL_MSG(false, "No bit should be set");
      }
      for (auto it = bitfield.GetIterator(); it.IsValid(); it.Next())
      {
        XII_TEST_BOOL_MSG(false, "No bit should be set");
      }
      XII_TEST_BOOL(bitfield.GetIterator() == bitfield.GetEndIterator());
      XII_TEST_BOOL(!bitfield.GetIterator().IsValid());
      XII_TEST_BOOL(!bitfield.GetEndIterator().IsValid());

      xiiStaticBitfield64 bitfield64 = xiiStaticBitfield64::MakeFromMask(0u);
      for (xiiUInt32 uiBit : bitfield64)
      {
        XII_TEST_BOOL_MSG(false, "No bit should be set");
      }
      for (auto it = bitfield64.GetIterator(); it.IsValid(); it.Next())
      {
        XII_TEST_BOOL_MSG(false, "No bit should be set");
      }
      XII_TEST_BOOL(bitfield64.GetIterator() == bitfield64.GetEndIterator());
      XII_TEST_BOOL(!bitfield64.GetIterator().IsValid());
      XII_TEST_BOOL(!bitfield64.GetEndIterator().IsValid());
    }

    {
      // Full 32 bits
      xiiStaticBitfield32 bitfield  = xiiStaticBitfield32::MakeFromMask(0xffffffffu);
      xiiUInt32           uiNextBit = 0;
      for (xiiUInt32 uiBit : bitfield)
      {
        XII_TEST_INT(uiBit, uiNextBit);
        uiNextBit++;
      }
      XII_TEST_INT(32, uiNextBit);

      uiNextBit = 0;
      for (auto it = bitfield.GetIterator(); it.IsValid(); ++it)
      {
        XII_TEST_INT(it.Value(), uiNextBit);
        XII_TEST_INT(*it, uiNextBit);
        XII_TEST_BOOL(it.IsValid());
        uiNextBit++;
      }
      XII_TEST_INT(32, uiNextBit);
    }

    {
      // Full 64 bits
      xiiStaticBitfield64 bitfield  = xiiStaticBitfield64::MakeFromMask(0xffffffffffffffffull);
      xiiUInt32           uiNextBit = 0;
      for (xiiUInt32 uiBit : bitfield)
      {
        XII_TEST_INT(uiBit, uiNextBit);
        uiNextBit++;
      }
      XII_TEST_INT(64, uiNextBit);

      uiNextBit = 0;
      for (auto it = bitfield.GetIterator(); it.IsValid(); ++it)
      {
        XII_TEST_INT(it.Value(), uiNextBit);
        XII_TEST_INT(*it, uiNextBit);
        XII_TEST_BOOL(it.IsValid());
        uiNextBit++;
      }
      XII_TEST_INT(64, uiNextBit);
    }

    {
      // Partial bits set 32 bit.
      xiiRandom rnd;
      rnd.Initialize(42);

      for (xiiUInt32 uiNumBits = 2; uiNumBits <= 32; ++uiNumBits)
      {
        // Add some random bits and ensure they appear in the iterator in order.
        xiiHybridArray<xiiUInt32, 3> bits;
        xiiUInt32                    uiBits = 0;
        for (int i = 0; i < uiNumBits; ++i)
        {
          const xiiUInt32 bit = (xiiUInt32)rnd.IntMinMax(0, 31);
          if (!bits.Contains(bit))
          {
            bits.PushBack(bit);
            uiBits |= XII_BIT(bit);
          }
        }
        bits.Sort();

        xiiStaticBitfield32 bitfield = xiiStaticBitfield32::MakeFromMask(uiBits);

        for (xiiUInt32 uiBit : bitfield)
        {
          XII_TEST_INT(uiBit, bits[0]);
          bits.RemoveAtAndCopy(0);
        }
        XII_TEST_BOOL(bits.IsEmpty());
      }
    }

    {
      // Partial bits set 64 bit.
      xiiRandom rnd;
      rnd.Initialize(42);

      for (xiiUInt32 uiNumBits = 2; uiNumBits <= 63; ++uiNumBits)
      {
        // Add some random bits and ensure they appear in the iterator in order.
        xiiHybridArray<xiiUInt32, 3> bits;
        xiiUInt64                    uiBits = 0;
        for (int i = 0; i < uiNumBits; ++i)
        {
          const xiiUInt32 bit = (xiiUInt32)rnd.IntMinMax(0, 63);
          if (!bits.Contains(bit))
          {
            bits.PushBack(bit);
            uiBits |= XII_BIT(bit);
          }
        }
        bits.Sort();

        xiiStaticBitfield64 bitfield = xiiStaticBitfield64::MakeFromMask(uiBits);

        for (xiiUInt32 uiBit : bitfield)
        {
          XII_TEST_INT(uiBit, bits[0]);
          bits.RemoveAtAndCopy(0);
        }
        XII_TEST_BOOL(bits.IsEmpty());
      }
    }
  }
}
