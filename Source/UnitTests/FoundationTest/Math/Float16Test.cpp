#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Strings/String.h>

XII_CREATE_SIMPLE_TEST(Math, Float16)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "From float and back")
  {
    // default constructor
    XII_TEST_BOOL(static_cast<float>(xiiFloat16()) == 0.0f);

    // Border cases - exact matching needed.
    XII_TEST_FLOAT(static_cast<float>(xiiFloat16(1.0f)), 1.0f, 0);
    XII_TEST_FLOAT(static_cast<float>(xiiFloat16(-1.0f)), -1.0f, 0);
    XII_TEST_FLOAT(static_cast<float>(xiiFloat16(0.0f)), 0.0f, 0);
    XII_TEST_FLOAT(static_cast<float>(xiiFloat16(-0.0f)), -0.0f, 0);
    XII_TEST_BOOL(static_cast<float>(xiiFloat16(xiiMath::Infinity<float>())) == xiiMath::Infinity<float>());
    XII_TEST_BOOL(static_cast<float>(xiiFloat16(-xiiMath::Infinity<float>())) == -xiiMath::Infinity<float>());
    XII_TEST_BOOL(xiiMath::IsNaN(static_cast<float>(xiiFloat16(xiiMath::NaN<float>()))));

    // Some random values.
    XII_TEST_FLOAT(static_cast<float>(xiiFloat16(42.0f)), 42.0f, xiiMath::LargeEpsilon<float>());
    XII_TEST_FLOAT(static_cast<float>(xiiFloat16(1.e3f)), 1.e3f, xiiMath::LargeEpsilon<float>());
    XII_TEST_FLOAT(static_cast<float>(xiiFloat16(-1230.0f)), -1230.0f, xiiMath::LargeEpsilon<float>());
    XII_TEST_FLOAT(static_cast<float>(xiiFloat16(xiiMath::Pi<float>())), xiiMath::Pi<float>(), xiiMath::HugeEpsilon<float>());

    // Denormalized float.
    XII_TEST_FLOAT(static_cast<float>(xiiFloat16(1.e-40f)), 0.0f, 0);
    XII_TEST_FLOAT(static_cast<float>(xiiFloat16(1.e-44f)), 0.0f, 0);

    // Clamping of too large/small values
    // Half only supports 2^-14 to 2^14 (in 10^x this is roughly 4.51) (see Wikipedia)
    XII_TEST_FLOAT(static_cast<float>(xiiFloat16(1.e-10f)), 0.0f, 0);
    XII_TEST_BOOL(static_cast<float>(xiiFloat16(1.e5f)) == xiiMath::Infinity<float>());
    XII_TEST_BOOL(static_cast<float>(xiiFloat16(-1.e5f)) == -xiiMath::Infinity<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator ==")
  {
    XII_TEST_BOOL(xiiFloat16(1.0f) == xiiFloat16(1.0f));
    XII_TEST_BOOL(xiiFloat16(10000000.0f) == xiiFloat16(10000000.0f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator !=")
  {
    XII_TEST_BOOL(xiiFloat16(1.0f) != xiiFloat16(-1.0f));
    XII_TEST_BOOL(xiiFloat16(10000000.0f) != xiiFloat16(10000.0f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRawData / SetRawData")
  {
    xiiFloat16 f;
    f.SetRawData(23);

    XII_TEST_INT(f.GetRawData(), 23);
  }
}
