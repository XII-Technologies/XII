#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/IterateBits.h>

namespace
{
  // declare bitflags using macro magic
  XII_DECLARE_FLAGS(xiiUInt32, AutoFlags, Bit1, Bit2, Bit3, Bit4);

  // declare bitflags manually
  struct ManualFlags
  {
    using StorageType = xiiUInt32;

    enum Enum
    {
      Bit1 = XII_BIT(0),
      Bit2 = XII_BIT(1),
      Bit3 = XII_BIT(2),
      Bit4 = XII_BIT(3),

      Default = Bit1 | Bit2
    };

    struct Bits
    {
      StorageType Bit1 : 1;
      StorageType Bit2 : 1;
      StorageType Bit3 : 1;
      StorageType Bit4 : 1;
    };
  };

  XII_DECLARE_FLAGS_OPERATORS(ManualFlags);
} // namespace

XII_DEFINE_AS_POD_TYPE(AutoFlags::Enum);
XII_CHECK_AT_COMPILETIME(sizeof(xiiBitflags<AutoFlags>) == 4);

XII_CREATE_SIMPLE_TEST(Basics, Bitflags)
{
  XII_TEST_BOOL(AutoFlags::Count == 4);

  {
    xiiBitflags<AutoFlags> flags = AutoFlags::Bit1 | AutoFlags::Bit4;

    XII_TEST_BOOL(flags.IsSet(AutoFlags::Bit4));
    XII_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit1 | AutoFlags::Bit4));
    XII_TEST_BOOL(flags.IsAnySet(AutoFlags::Bit1 | AutoFlags::Bit2));
    XII_TEST_BOOL(!flags.IsAnySet(AutoFlags::Bit2 | AutoFlags::Bit3));
    XII_TEST_BOOL(flags.AreNoneSet(AutoFlags::Bit2 | AutoFlags::Bit3));
    XII_TEST_BOOL(!flags.AreNoneSet(AutoFlags::Bit2 | AutoFlags::Bit4));
    XII_TEST_BOOL(flags.IsStrictlyAnySet(AutoFlags::Bit1 | AutoFlags::Bit4));
    XII_TEST_BOOL(flags.IsStrictlyAnySet(AutoFlags::Bit1 | AutoFlags::Bit2 | AutoFlags::Bit4));
    XII_TEST_BOOL(!flags.IsStrictlyAnySet(AutoFlags::Bit1));
    XII_TEST_BOOL(!flags.IsStrictlyAnySet(AutoFlags::Bit2 | AutoFlags::Bit4));

    flags.Add(AutoFlags::Bit3);
    XII_TEST_BOOL(flags.IsSet(AutoFlags::Bit3));
    XII_TEST_BOOL(flags.IsStrictlyAnySet(AutoFlags::Bit1 | AutoFlags::Bit3 | AutoFlags::Bit4));
    XII_TEST_BOOL(!flags.IsStrictlyAnySet(AutoFlags::Bit1 | AutoFlags::Bit4));

    flags.Remove(AutoFlags::Bit1);
    XII_TEST_BOOL(!flags.IsSet(AutoFlags::Bit1));
    XII_TEST_BOOL(flags.IsStrictlyAnySet(AutoFlags::Bit1 | AutoFlags::Bit3 | AutoFlags::Bit4));

    flags.Toggle(AutoFlags::Bit4);
    XII_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit3));
    XII_TEST_BOOL(flags.IsStrictlyAnySet(AutoFlags::Bit1 | AutoFlags::Bit3 | AutoFlags::Bit4));

    flags.AddOrRemove(AutoFlags::Bit2, true);
    flags.AddOrRemove(AutoFlags::Bit3, false);
    XII_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit2));

    flags.Add(AutoFlags::Bit1);

    xiiBitflags<ManualFlags> manualFlags = ManualFlags::Default;
    XII_TEST_BOOL(manualFlags.AreAllSet(ManualFlags::Bit1 | ManualFlags::Bit2));
    XII_TEST_BOOL(manualFlags.GetValue() == flags.GetValue());
    XII_TEST_BOOL(manualFlags.AreAllSet(ManualFlags::Default & ManualFlags::Bit2));

    XII_TEST_BOOL(flags.IsAnyFlagSet());
    flags.Clear();
    XII_TEST_BOOL(flags.IsNoFlagSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator&")
  {
    xiiBitflags<AutoFlags> flags2 = AutoFlags::Bit1 & AutoFlags::Bit4;
    XII_TEST_BOOL(flags2.GetValue() == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetValue")
  {
    xiiBitflags<AutoFlags> flags;
    flags.SetValue(17);
    XII_TEST_BOOL(flags.GetValue() == 17);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator|=")
  {
    xiiBitflags<AutoFlags> f = AutoFlags::Bit1 | AutoFlags::Bit2;
    f |= AutoFlags::Bit3;

    XII_TEST_BOOL(f.GetValue() == (AutoFlags::Bit1 | AutoFlags::Bit2 | AutoFlags::Bit3).GetValue());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator&=")
  {
    xiiBitflags<AutoFlags> f = AutoFlags::Bit1 | AutoFlags::Bit2 | AutoFlags::Bit3;
    f &= AutoFlags::Bit3;

    XII_TEST_BOOL(f.GetValue() == AutoFlags::Bit3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Iterator")
  {
    {
      // Empty
      xiiBitflags<AutoFlags> f;
      auto                   it = f.GetIterator();
      XII_TEST_BOOL(it == f.GetEndIterator());
      XII_TEST_BOOL(!it.IsValid());

      for (AutoFlags::Enum flag : f)
      {
        XII_TEST_BOOL_MSG(false, "No bit should be set");
      }
    }

    {
      // All flags
      xiiBitflags<AutoFlags>             f = AutoFlags::Bit1 | AutoFlags::Bit2 | AutoFlags::Bit3 | AutoFlags::Bit4;
      xiiHybridArray<AutoFlags::Enum, 4> flags;
      flags.PushBack(AutoFlags::Bit1);
      flags.PushBack(AutoFlags::Bit2);
      flags.PushBack(AutoFlags::Bit3);
      flags.PushBack(AutoFlags::Bit4);

      xiiUInt32 uiIndex = 0;
      // Iterator
      for (auto it = f.GetIterator(); it.IsValid(); ++it)
      {
        XII_TEST_INT(*it, flags[uiIndex]);
        XII_TEST_INT(it.Value(), flags[uiIndex]);
        XII_TEST_BOOL(it.IsValid());
        ++uiIndex;
      }
      XII_TEST_INT(uiIndex, 4);

      // Range-base for loop
      uiIndex = 0;
      for (AutoFlags::Enum flag : f)
      {
        XII_TEST_INT(flag, flags[uiIndex]);
        ++uiIndex;
      }
      XII_TEST_INT(uiIndex, 4);
    }
  }
}


//////////////////////////////////////////////////////////////////////////

namespace
{
  struct TypelessFlags1
  {
    enum Enum
    {
      Bit1 = XII_BIT(0),
      Bit2 = XII_BIT(1),
    };
  };

  struct TypelessFlags2
  {
    enum Enum
    {
      Bit3 = XII_BIT(2),
      Bit4 = XII_BIT(3),
    };
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Basics, TypelessBitflags)
{
  {
    xiiTypelessBitflags<xiiUInt32> flags = static_cast<xiiUInt32>(TypelessFlags1::Bit1) | static_cast<xiiUInt32>(TypelessFlags2::Bit4);

    XII_TEST_BOOL(flags.IsAnySet(static_cast<xiiUInt32>(TypelessFlags2::Bit4)));
    XII_TEST_BOOL(flags.AreAllSet(static_cast<xiiUInt32>(TypelessFlags1::Bit1) | static_cast<xiiUInt32>(TypelessFlags2::Bit4)));
    XII_TEST_BOOL(flags.IsAnySet(static_cast<xiiUInt32>(TypelessFlags1::Bit1) | static_cast<xiiUInt32>(TypelessFlags1::Bit2)));
    XII_TEST_BOOL(!flags.IsAnySet(static_cast<xiiUInt32>(TypelessFlags1::Bit2) | static_cast<xiiUInt32>(TypelessFlags2::Bit3)));
    XII_TEST_BOOL(flags.AreNoneSet(static_cast<xiiUInt32>(TypelessFlags1::Bit2) | static_cast<xiiUInt32>(TypelessFlags2::Bit3)));
    XII_TEST_BOOL(!flags.AreNoneSet(static_cast<xiiUInt32>(TypelessFlags1::Bit2) | static_cast<xiiUInt32>(TypelessFlags2::Bit4)));

    flags.Add(static_cast<xiiUInt32>(TypelessFlags2::Bit3));
    XII_TEST_BOOL(flags.IsAnySet(static_cast<xiiUInt32>(TypelessFlags2::Bit3)));

    flags.Remove(static_cast<xiiUInt32>(TypelessFlags1::Bit1));
    XII_TEST_BOOL(!flags.IsAnySet(static_cast<xiiUInt32>(TypelessFlags1::Bit1)));

    flags.Toggle(static_cast<xiiUInt32>(TypelessFlags2::Bit4));
    XII_TEST_BOOL(flags.AreAllSet(static_cast<xiiUInt32>(TypelessFlags2::Bit3)));

    flags.AddOrRemove(static_cast<xiiUInt32>(TypelessFlags1::Bit2), true);
    flags.AddOrRemove(static_cast<xiiUInt32>(TypelessFlags2::Bit3), false);
    XII_TEST_BOOL(flags.AreAllSet(static_cast<xiiUInt32>(TypelessFlags1::Bit2)));

    XII_TEST_BOOL(!flags.IsNoFlagSet());
    flags.Clear();
    XII_TEST_BOOL(flags.IsNoFlagSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator&")
  {
    xiiTypelessBitflags<xiiUInt32> flags2 = static_cast<xiiUInt32>(TypelessFlags1::Bit1) & static_cast<xiiUInt32>(TypelessFlags2::Bit4);
    XII_TEST_BOOL(flags2.GetValue() == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetValue")
  {
    xiiTypelessBitflags<xiiUInt32> flags;
    XII_TEST_BOOL(flags.IsNoFlagSet());
    XII_TEST_BOOL(flags.GetValue() == 0);
    flags.SetValue(17);
    XII_TEST_BOOL(flags.GetValue() == 17);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator|=")
  {
    xiiTypelessBitflags<xiiUInt32> f = static_cast<xiiUInt32>(TypelessFlags1::Bit1) | static_cast<xiiUInt32>(TypelessFlags1::Bit2);
    f |= static_cast<xiiUInt32>(TypelessFlags2::Bit3);

    XII_TEST_BOOL(f.GetValue() == (static_cast<xiiUInt32>(TypelessFlags1::Bit1) | static_cast<xiiUInt32>(TypelessFlags1::Bit2) | static_cast<xiiUInt32>(TypelessFlags2::Bit3)));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator&=")
  {
    xiiTypelessBitflags<xiiUInt32> f = static_cast<xiiUInt32>(TypelessFlags1::Bit1) | static_cast<xiiUInt32>(TypelessFlags1::Bit2) | static_cast<xiiUInt32>(TypelessFlags2::Bit3);
    f &= static_cast<xiiUInt32>(TypelessFlags2::Bit3);

    XII_TEST_BOOL(f.GetValue() == static_cast<xiiUInt32>(TypelessFlags2::Bit3));
  }
}
