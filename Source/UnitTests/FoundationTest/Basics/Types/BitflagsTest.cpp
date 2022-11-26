#include <FoundationTest/FoundationTestPCH.h>

namespace
{
  // declare bitflags using macro magic
  XII_DECLARE_FLAGS(xiiUInt32, AutoFlags, Bit1, Bit2, Bit3, Bit4);

  // declare bitflags manually
  struct ManualFlags
  {
    typedef xiiUInt32 StorageType;

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

    flags.Add(AutoFlags::Bit3);
    XII_TEST_BOOL(flags.IsSet(AutoFlags::Bit3));

    flags.Remove(AutoFlags::Bit1);
    XII_TEST_BOOL(!flags.IsSet(AutoFlags::Bit1));

    flags.Toggle(AutoFlags::Bit4);
    XII_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit3));

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
    xiiTypelessBitflags<xiiUInt32> flags = TypelessFlags1::Bit1 | TypelessFlags2::Bit4;

    XII_TEST_BOOL(flags.IsAnySet(TypelessFlags2::Bit4));
    XII_TEST_BOOL(flags.AreAllSet(TypelessFlags1::Bit1 | TypelessFlags2::Bit4));
    XII_TEST_BOOL(flags.IsAnySet(TypelessFlags1::Bit1 | TypelessFlags1::Bit2));
    XII_TEST_BOOL(!flags.IsAnySet(TypelessFlags1::Bit2 | TypelessFlags2::Bit3));
    XII_TEST_BOOL(flags.AreNoneSet(TypelessFlags1::Bit2 | TypelessFlags2::Bit3));
    XII_TEST_BOOL(!flags.AreNoneSet(TypelessFlags1::Bit2 | TypelessFlags2::Bit4));

    flags.Add(TypelessFlags2::Bit3);
    XII_TEST_BOOL(flags.IsAnySet(TypelessFlags2::Bit3));

    flags.Remove(TypelessFlags1::Bit1);
    XII_TEST_BOOL(!flags.IsAnySet(TypelessFlags1::Bit1));

    flags.Toggle(TypelessFlags2::Bit4);
    XII_TEST_BOOL(flags.AreAllSet(TypelessFlags2::Bit3));

    flags.AddOrRemove(TypelessFlags1::Bit2, true);
    flags.AddOrRemove(TypelessFlags2::Bit3, false);
    XII_TEST_BOOL(flags.AreAllSet(TypelessFlags1::Bit2));

    XII_TEST_BOOL(!flags.IsNoFlagSet());
    flags.Clear();
    XII_TEST_BOOL(flags.IsNoFlagSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator&")
  {
    xiiTypelessBitflags<xiiUInt32> flags2 = TypelessFlags1::Bit1 & TypelessFlags2::Bit4;
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
    xiiTypelessBitflags<xiiUInt32> f = TypelessFlags1::Bit1 | TypelessFlags1::Bit2;
    f |= TypelessFlags2::Bit3;

    XII_TEST_BOOL(f.GetValue() == (TypelessFlags1::Bit1 | TypelessFlags1::Bit2 | TypelessFlags2::Bit3));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator&=")
  {
    xiiTypelessBitflags<xiiUInt32> f = TypelessFlags1::Bit1 | TypelessFlags1::Bit2 | TypelessFlags2::Bit3;
    f &= TypelessFlags2::Bit3;

    XII_TEST_BOOL(f.GetValue() == TypelessFlags2::Bit3);
  }
}
