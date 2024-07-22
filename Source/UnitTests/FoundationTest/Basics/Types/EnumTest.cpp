#include <FoundationTest/FoundationTestPCH.h>

//////////////////////////////////////////////////////////////////////
// Start of the definition of a example Enum
// It takes quite some lines of code to define a enum,
// but it could be encapsulated into an preprocessor macro if wanted
struct xiiTestEnumBase
{
  using StorageType = xiiUInt8; // The storage type for the enum

  enum Enum
  {
    No      = 0,
    Yes     = 1,
    Default = No // Default initialization
  };
};

using xiiTestEnum = xiiEnum<xiiTestEnumBase>; // The name of the final enum
// End of the definition of a example enum
///////////////////////////////////////////////////////////////////////

struct xiiTestEnum2Base
{
  using StorageType = xiiUInt16;

  enum Enum
  {
    Bit1    = XII_BIT(0),
    Bit2    = XII_BIT(1),
    Default = Bit1
  };
};

using xiiTestEnum2 = xiiEnum<xiiTestEnum2Base>;

// Test if the type actually has the requested size
static_assert(sizeof(xiiTestEnum) == sizeof(xiiUInt8));
static_assert(sizeof(xiiTestEnum2) == sizeof(xiiUInt16));

XII_CREATE_SIMPLE_TEST_GROUP(Basics);

// This takes a c++ enum. Tests the implict conversion
void TakeEnum1(xiiTestEnum::Enum value) {}

// This takes our own enum type
void TakeEnum2(xiiTestEnum value) {}

XII_CREATE_SIMPLE_TEST(Basics, Enum)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Default initialized enum")
  {
    xiiTestEnum e1;
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Enum with explicit initialization")
  {
    xiiTestEnum e2(xiiTestEnum::Yes);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "This tests if the default initialization works and if the implicit conversion works")
  {
    xiiTestEnum e1;
    xiiTestEnum e2(xiiTestEnum::Yes);

    XII_TEST_BOOL(e1 == xiiTestEnum::No);
    XII_TEST_BOOL(e2 == xiiTestEnum::Yes);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Function call tests")
  {
    xiiTestEnum e1;

    TakeEnum1(e1);
    TakeEnum2(e1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetValue and SetValue")
  {
    xiiTestEnum e1;
    XII_TEST_INT(e1.GetValue(), 0);
    e1.SetValue(17);
    XII_TEST_INT(e1.GetValue(), 17);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Assignment of different values")
  {
    xiiTestEnum e1, e2;

    e1 = xiiTestEnum::Yes;
    e2 = xiiTestEnum::No;
    XII_TEST_BOOL(e1 == xiiTestEnum::Yes);
    XII_TEST_BOOL(e2 == xiiTestEnum::No);

    e1 = e2;
    XII_TEST_BOOL(e1 == xiiTestEnum::No);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Test the | operator")
  {
    xiiTestEnum2 e3(xiiTestEnum2::Bit1);
    xiiTestEnum2 e4(xiiTestEnum2::Bit2);
    xiiUInt16    uiBits = (e3 | e4).GetValue();
    XII_TEST_BOOL(uiBits == (xiiTestEnum2::Bit1 | xiiTestEnum2::Bit2));
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Test the & operator")
  {
    xiiTestEnum2 e3(xiiTestEnum2::Bit1);
    xiiTestEnum2 e4(xiiTestEnum2::Bit2);
    xiiUInt16    uiBits = ((e3 | e4) & e4).GetValue();
    XII_TEST_BOOL(uiBits == xiiTestEnum2::Bit2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Test conversion to int")
  {
    xiiTestEnum e1;
    int         iTest = e1.GetValue();
    XII_TEST_BOOL(iTest == xiiTestEnum::No);
  }
}
