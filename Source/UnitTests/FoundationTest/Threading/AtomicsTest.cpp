#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/VariantType.h>

namespace
{
  template <typename T>
  struct AtomicType
  {
    xiiAtomicInteger<T> m_IncVariable = 0;
    xiiAtomicInteger<T> m_DecVariable = 0;

    xiiAtomicInteger<T> m_AddVariable = 0;
    xiiAtomicInteger<T> m_SubVariable = 0;

    xiiAtomicInteger<T> m_AndVariable = static_cast<T>(0xFF);
    xiiAtomicInteger<T> m_OrVariable  = 1;
    xiiAtomicInteger<T> m_XorVariable = 3;

    xiiAtomicInteger<T> m_MinVariable = 100;
    xiiAtomicInteger<T> m_MaxVariable = 0;

    xiiAtomicInteger<T> m_SetVariable = 0;

    xiiAtomicInteger<T>        m_TestAndSetVariable = 0;
    xii_atomic_underlying_t<T> m_TestAndSetCounter  = 0;

    xiiAtomicInteger<T>        m_CompareAndSwapVariable = 0;
    xii_atomic_underlying_t<T> m_CompareAndSwapCounter  = 0;
  };

  AtomicType<xiiInt8>   g_AtomicVariables8;
  AtomicType<xiiUInt8>  g_AtomicVariablesU8;
  AtomicType<xiiInt16>  g_AtomicVariables16;
  AtomicType<xiiUInt16> g_AtomicVariablesU16;
  AtomicType<xiiInt32>  g_AtomicVariables32;
  AtomicType<xiiUInt32> g_AtomicVariablesU32;
  AtomicType<xiiInt64>  g_AtomicVariables64;
  AtomicType<xiiUInt64> g_AtomicVariablesU64;

  void*    g_pTestAndSetPointer        = nullptr;
  xiiInt32 g_iTestAndSetPointerCounter = 0;

  xiiAtomicInteger<xiiVariantType::Enum> g_EnumSet = xiiVariantType::Bool;

  xiiAtomicInteger<xiiVariantType::Enum> g_EnumCompareAndSwap         = xiiVariantType::Bool;
  xiiInt32                               g_iCompareAndSwapCounterEnum = 0;

  xiiAtomicInteger<xiiVariantType::Enum> g_EnumTestAndSetVariable = xiiVariantType::Bool;
  xiiInt32                               g_iTestAndSetCounterEnum = 0;

  class AtomicsTestThread : public xiiThread
  {
  public:
    AtomicsTestThread(xiiInt32 iIndex) :
      xiiThread("Test Thread"), m_iIndex(iIndex)
    {
    }

    virtual xiiUInt32 Run()
    {
      // xiiInt8
      {
        xiiInt8 iIndex = static_cast<xiiInt8>(m_iIndex);

        g_AtomicVariables8.m_IncVariable.Increment();
        g_AtomicVariables8.m_DecVariable.Decrement();

        g_AtomicVariables8.m_AddVariable.Add(iIndex);
        g_AtomicVariables8.m_SubVariable.Subtract(iIndex);

        const xiiInt8 iBit = 1 << iIndex;

        g_AtomicVariables8.m_AndVariable.And(iBit);
        g_AtomicVariables8.m_OrVariable.Or(iBit);
        g_AtomicVariables8.m_XorVariable.Xor(iBit);

        g_AtomicVariables8.m_MinVariable.Min(iIndex);
        g_AtomicVariables8.m_MaxVariable.Max(iIndex);

        g_AtomicVariables8.m_SetVariable.Set(iIndex);

        if (g_AtomicVariables8.m_TestAndSetVariable.TestAndSet(0, iIndex))
        {
          ++g_AtomicVariables8.m_TestAndSetCounter;
        }

        if (g_AtomicVariables8.m_CompareAndSwapVariable.CompareAndSwap(0, iIndex) == 0)
        {
          ++g_AtomicVariables8.m_CompareAndSwapCounter;
        }
      }

      // xiiUInt8
      {
        xiiUInt8 uiIndex = static_cast<xiiUInt8>(m_iIndex);

        g_AtomicVariablesU8.m_IncVariable.Increment();
        g_AtomicVariablesU8.m_DecVariable.Decrement();

        g_AtomicVariablesU8.m_AddVariable.Add(uiIndex);
        g_AtomicVariablesU8.m_SubVariable.Subtract(uiIndex);

        const xiiUInt8 uiBit = 1 << uiIndex;

        g_AtomicVariablesU8.m_AndVariable.And(uiBit);
        g_AtomicVariablesU8.m_OrVariable.Or(uiBit);
        g_AtomicVariablesU8.m_XorVariable.Xor(uiBit);

        g_AtomicVariablesU8.m_MinVariable.Min(uiIndex);
        g_AtomicVariablesU8.m_MaxVariable.Max(uiIndex);

        g_AtomicVariablesU8.m_SetVariable.Set(uiIndex);

        if (g_AtomicVariablesU8.m_TestAndSetVariable.TestAndSet(0, uiIndex))
        {
          ++g_AtomicVariablesU8.m_TestAndSetCounter;
        }

        if (g_AtomicVariablesU8.m_CompareAndSwapVariable.CompareAndSwap(0, uiIndex) == 0)
        {
          ++g_AtomicVariablesU8.m_CompareAndSwapCounter;
        }
      }

      // xiiInt16
      {
        xiiInt16 iIndex = static_cast<xiiInt16>(m_iIndex);

        g_AtomicVariables16.m_IncVariable.Increment();
        g_AtomicVariables16.m_DecVariable.Decrement();

        g_AtomicVariables16.m_AddVariable.Add(iIndex);
        g_AtomicVariables16.m_SubVariable.Subtract(iIndex);

        const xiiInt16 iBit = 1 << iIndex;

        g_AtomicVariables16.m_AndVariable.And(iBit);
        g_AtomicVariables16.m_OrVariable.Or(iBit);
        g_AtomicVariables16.m_XorVariable.Xor(iBit);

        g_AtomicVariables16.m_MinVariable.Min(iIndex);
        g_AtomicVariables16.m_MaxVariable.Max(iIndex);

        g_AtomicVariables16.m_SetVariable.Set(iIndex);

        if (g_AtomicVariables16.m_TestAndSetVariable.TestAndSet(0, iIndex))
        {
          ++g_AtomicVariables16.m_TestAndSetCounter;
        }

        if (g_AtomicVariables16.m_CompareAndSwapVariable.CompareAndSwap(0, iIndex) == 0)
        {
          ++g_AtomicVariables16.m_CompareAndSwapCounter;
        }
      }

      // xiiUInt16
      {
        xiiUInt16 uiIndex = static_cast<xiiUInt16>(m_iIndex);

        g_AtomicVariablesU16.m_IncVariable.Increment();
        g_AtomicVariablesU16.m_DecVariable.Decrement();

        g_AtomicVariablesU16.m_AddVariable.Add(uiIndex);
        g_AtomicVariablesU16.m_SubVariable.Subtract(uiIndex);

        const xiiUInt16 uiBit = 1 << uiIndex;

        g_AtomicVariablesU16.m_AndVariable.And(uiBit);
        g_AtomicVariablesU16.m_OrVariable.Or(uiBit);
        g_AtomicVariablesU16.m_XorVariable.Xor(uiBit);

        g_AtomicVariablesU16.m_MinVariable.Min(uiIndex);
        g_AtomicVariablesU16.m_MaxVariable.Max(uiIndex);

        g_AtomicVariablesU16.m_SetVariable.Set(uiIndex);

        if (g_AtomicVariablesU16.m_TestAndSetVariable.TestAndSet(0, uiIndex))
        {
          ++g_AtomicVariablesU16.m_TestAndSetCounter;
        }

        if (g_AtomicVariablesU16.m_CompareAndSwapVariable.CompareAndSwap(0, uiIndex) == 0)
        {
          ++g_AtomicVariablesU16.m_CompareAndSwapCounter;
        }
      }

      // xiiInt32
      {
        xiiInt32 iIndex = static_cast<xiiInt32>(m_iIndex);

        g_AtomicVariables32.m_IncVariable.Increment();
        g_AtomicVariables32.m_DecVariable.Decrement();

        g_AtomicVariables32.m_AddVariable.Add(iIndex);
        g_AtomicVariables32.m_SubVariable.Subtract(iIndex);

        const xiiInt32 iBit = 1 << iIndex;

        g_AtomicVariables32.m_AndVariable.And(iBit);
        g_AtomicVariables32.m_OrVariable.Or(iBit);
        g_AtomicVariables32.m_XorVariable.Xor(iBit);

        g_AtomicVariables32.m_MinVariable.Min(iIndex);
        g_AtomicVariables32.m_MaxVariable.Max(iIndex);

        g_AtomicVariables32.m_SetVariable.Set(iIndex);

        if (g_AtomicVariables32.m_TestAndSetVariable.TestAndSet(0, iIndex))
        {
          ++g_AtomicVariables32.m_TestAndSetCounter;
        }

        if (g_AtomicVariables32.m_CompareAndSwapVariable.CompareAndSwap(0, iIndex) == 0)
        {
          ++g_AtomicVariables32.m_CompareAndSwapCounter;
        }
      }

      // xiiUInt32
      {
        xiiUInt32 uiIndex = static_cast<xiiUInt32>(m_iIndex);

        g_AtomicVariablesU32.m_IncVariable.Increment();
        g_AtomicVariablesU32.m_DecVariable.Decrement();

        g_AtomicVariablesU32.m_AddVariable.Add(uiIndex);
        g_AtomicVariablesU32.m_SubVariable.Subtract(uiIndex);

        const xiiUInt32 uiBit = 1 << uiIndex;

        g_AtomicVariablesU32.m_AndVariable.And(uiBit);
        g_AtomicVariablesU32.m_OrVariable.Or(uiBit);
        g_AtomicVariablesU32.m_XorVariable.Xor(uiBit);

        g_AtomicVariablesU32.m_MinVariable.Min(uiIndex);
        g_AtomicVariablesU32.m_MaxVariable.Max(uiIndex);

        g_AtomicVariablesU32.m_SetVariable.Set(uiIndex);

        if (g_AtomicVariablesU32.m_TestAndSetVariable.TestAndSet(0, uiIndex))
        {
          ++g_AtomicVariablesU32.m_TestAndSetCounter;
        }

        if (g_AtomicVariablesU32.m_CompareAndSwapVariable.CompareAndSwap(0, uiIndex) == 0)
        {
          ++g_AtomicVariablesU32.m_CompareAndSwapCounter;
        }
      }

      // xiiInt64
      {
        xiiInt64 iIndex = static_cast<xiiInt64>(m_iIndex);

        g_AtomicVariables64.m_IncVariable.Increment();
        g_AtomicVariables64.m_DecVariable.Decrement();

        g_AtomicVariables64.m_AddVariable.Add(iIndex);
        g_AtomicVariables64.m_SubVariable.Subtract(iIndex);

        const xiiInt64 iBit = 1LL << iIndex;

        g_AtomicVariables64.m_AndVariable.And(iBit);
        g_AtomicVariables64.m_OrVariable.Or(iBit);
        g_AtomicVariables64.m_XorVariable.Xor(iBit);

        g_AtomicVariables64.m_MinVariable.Min(iIndex);
        g_AtomicVariables64.m_MaxVariable.Max(iIndex);

        g_AtomicVariables64.m_SetVariable.Set(iIndex);

        if (g_AtomicVariables64.m_TestAndSetVariable.TestAndSet(0, iIndex))
        {
          ++g_AtomicVariables64.m_TestAndSetCounter;
        }

        if (g_AtomicVariables64.m_CompareAndSwapVariable.CompareAndSwap(0, iIndex) == 0)
        {
          ++g_AtomicVariables64.m_CompareAndSwapCounter;
        }
      }

      // xiiUInt64
      {
        xiiUInt64 uiIndex = static_cast<xiiUInt64>(m_iIndex);

        g_AtomicVariablesU64.m_IncVariable.Increment();
        g_AtomicVariablesU64.m_DecVariable.Decrement();

        g_AtomicVariablesU64.m_AddVariable.Add(uiIndex);
        g_AtomicVariablesU64.m_SubVariable.Subtract(uiIndex);

        const xiiUInt64 uiBit = 1ULL << uiIndex;

        g_AtomicVariablesU64.m_AndVariable.And(uiBit);
        g_AtomicVariablesU64.m_OrVariable.Or(uiBit);
        g_AtomicVariablesU64.m_XorVariable.Xor(uiBit);

        g_AtomicVariablesU64.m_MinVariable.Min(uiIndex);
        g_AtomicVariablesU64.m_MaxVariable.Max(uiIndex);

        g_AtomicVariablesU64.m_SetVariable.Set(uiIndex);

        if (g_AtomicVariablesU64.m_TestAndSetVariable.TestAndSet(0, uiIndex))
        {
          ++g_AtomicVariablesU64.m_TestAndSetCounter;
        }

        if (g_AtomicVariablesU64.m_CompareAndSwapVariable.CompareAndSwap(0, uiIndex) == 0)
        {
          ++g_AtomicVariablesU64.m_CompareAndSwapCounter;
        }
      }

      if (xiiAtomicUtils::CompareExchangePointer(&g_pTestAndSetPointer, nullptr, this))
      {
        ++g_iTestAndSetPointerCounter;
      }

      const xiiVariantType::Enum targetEnum = m_iIndex == 1 ? xiiVariantType::Float : xiiVariantType::Color;
      g_EnumSet.Set(targetEnum);

      if (g_EnumTestAndSetVariable.TestAndSet(xiiVariantType::Bool, targetEnum))
      {
        ++g_iTestAndSetCounterEnum;
      }

      if (g_EnumCompareAndSwap.CompareAndSwap(xiiVariantType::Bool, targetEnum) == xiiVariantType::Bool)
      {
        ++g_iCompareAndSwapCounterEnum;
      }

      return 0;
    }

  private:
    xiiInt32 m_iIndex;
  };

  template <typename T>
  struct AtomicPostOperations
  {
    xiiAtomicInteger<T>                  m_PostIncVariable = 0;
    xiiAtomicInteger<T>                  m_PostDecVariable = 0;
    xiiDynamicArray<xiiAtomicInteger<T>> m_PostIncValues;
    xiiDynamicArray<xiiAtomicInteger<T>> m_PostDecValues;
  };

  AtomicPostOperations<xiiInt8>            g_AtomicPostOperations8;
  AtomicPostOperations<xiiUInt8>           g_AtomicPostOperationsU8;
  AtomicPostOperations<xiiInt16>           g_AtomicPostOperations16;
  AtomicPostOperations<xiiUInt16>          g_AtomicPostOperationsU16;
  AtomicPostOperations<xiiInt32>           g_AtomicPostOperations32;
  AtomicPostOperations<xiiUInt32>          g_AtomicPostOperationsU32;
  AtomicPostOperations<xiiInt64>           g_AtomicPostOperations64;
  AtomicPostOperations<xiiUInt64>          g_AtomicPostOperationsU64;
  xiiDynamicArray<xiiUniquePtr<xiiThread>> g_PostIncDecThreads;

  class PostIncDecThread : public xiiThread
  {
  public:
    PostIncDecThread(xiiInt32 iIndex) :
      xiiThread("Test Thread"), m_iIndex(iIndex)
    {
    }

    virtual xiiUInt32 Run()
    {
      // xiiInt8
      {
        g_AtomicPostOperations8.m_PostIncValues[g_AtomicPostOperations8.m_PostIncVariable.PostIncrement()].Increment();
        g_AtomicPostOperations8.m_PostDecValues[g_AtomicPostOperations8.m_PostDecVariable.PostDecrement()].Increment();
      }

      // xiiUInt8
      {
        g_AtomicPostOperationsU8.m_PostIncValues[g_AtomicPostOperationsU8.m_PostIncVariable.PostIncrement()].Increment();
        g_AtomicPostOperationsU8.m_PostDecValues[g_AtomicPostOperationsU8.m_PostDecVariable.PostDecrement()].Increment();
      }

      // xiiInt16
      {
        g_AtomicPostOperations16.m_PostIncValues[g_AtomicPostOperations16.m_PostIncVariable.PostIncrement()].Increment();
        g_AtomicPostOperations16.m_PostDecValues[g_AtomicPostOperations16.m_PostDecVariable.PostDecrement()].Increment();
      }

      // xiiUInt16
      {
        g_AtomicPostOperationsU16.m_PostIncValues[g_AtomicPostOperationsU16.m_PostIncVariable.PostIncrement()].Increment();
        g_AtomicPostOperationsU16.m_PostDecValues[g_AtomicPostOperationsU16.m_PostDecVariable.PostDecrement()].Increment();
      }

      // xiiInt32
      {
        g_AtomicPostOperations32.m_PostIncValues[g_AtomicPostOperations32.m_PostIncVariable.PostIncrement()].Increment();
        g_AtomicPostOperations32.m_PostDecValues[g_AtomicPostOperations32.m_PostDecVariable.PostDecrement()].Increment();
      }

      // xiiUInt32
      {
        g_AtomicPostOperationsU32.m_PostIncValues[g_AtomicPostOperationsU32.m_PostIncVariable.PostIncrement()].Increment();
        g_AtomicPostOperationsU32.m_PostDecValues[g_AtomicPostOperationsU32.m_PostDecVariable.PostDecrement()].Increment();
      }

      // xiiInt64
      {
        g_AtomicPostOperations64.m_PostIncValues[static_cast<xiiInt32>(g_AtomicPostOperations64.m_PostIncVariable.PostIncrement())].Increment();
        g_AtomicPostOperations64.m_PostDecValues[static_cast<xiiInt32>(g_AtomicPostOperations64.m_PostDecVariable.PostDecrement())].Increment();
      }

      // xiiUInt64
      {
        g_AtomicPostOperationsU64.m_PostIncValues[static_cast<xiiUInt32>(g_AtomicPostOperationsU64.m_PostIncVariable.PostIncrement())].Increment();
        g_AtomicPostOperationsU64.m_PostDecValues[static_cast<xiiUInt32>(g_AtomicPostOperationsU64.m_PostDecVariable.PostDecrement())].Increment();
      }

      return 0;
    }

  private:
    xiiInt32 m_iIndex;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Threading, Atomics)
{
  // Initialization
  {
    g_AtomicVariables8   = AtomicType<xiiInt8>();
    g_AtomicVariablesU8  = AtomicType<xiiUInt8>();
    g_AtomicVariables16  = AtomicType<xiiInt16>();
    g_AtomicVariablesU16 = AtomicType<xiiUInt16>();
    g_AtomicVariables32  = AtomicType<xiiInt32>();
    g_AtomicVariablesU32 = AtomicType<xiiUInt32>();
    g_AtomicVariables64  = AtomicType<xiiInt64>();
    g_AtomicVariablesU64 = AtomicType<xiiUInt64>();

    g_pTestAndSetPointer        = nullptr;
    g_iTestAndSetPointerCounter = 0;

    g_EnumSet                    = xiiVariantType::Bool;
    g_EnumTestAndSetVariable     = xiiVariantType::Bool;
    g_EnumCompareAndSwap         = xiiVariantType::Bool;
    g_iTestAndSetCounterEnum     = 0;
    g_iCompareAndSwapCounterEnum = 0;
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Thread")
  {
    AtomicsTestThread* pTestThread  = nullptr;
    AtomicsTestThread* pTestThread2 = nullptr;

    /// the try-catch is necessary to quiet the static code analysis
    try
    {
      pTestThread  = new AtomicsTestThread(1);
      pTestThread2 = new AtomicsTestThread(2);
    }
    catch (...)
    {
    }

    XII_TEST_BOOL(pTestThread != nullptr);
    XII_TEST_BOOL(pTestThread2 != nullptr);

    // Both thread will increment via atomic operations the global variable
    pTestThread->Start();
    pTestThread2->Start();

    // Join with both threads
    pTestThread->Join();
    pTestThread2->Join();

    // Test deletion
    delete pTestThread;
    delete pTestThread2;

    // xiiInt8
    {
      XII_TEST_INT(g_AtomicVariables8.m_IncVariable, 2);
      XII_TEST_INT(g_AtomicVariables8.m_DecVariable, -2);

      XII_TEST_INT(g_AtomicVariables8.m_AddVariable, 3);
      XII_TEST_INT(g_AtomicVariables8.m_SubVariable, -3);

      XII_TEST_INT(g_AtomicVariables8.m_AndVariable, 0);
      XII_TEST_INT(g_AtomicVariables8.m_OrVariable, 7);
      XII_TEST_INT(g_AtomicVariables8.m_XorVariable, 5);

      XII_TEST_INT(g_AtomicVariables8.m_MinVariable, 1);
      XII_TEST_INT(g_AtomicVariables8.m_MaxVariable, 2);

      XII_TEST_BOOL(g_AtomicVariables8.m_SetVariable > 0);

      XII_TEST_BOOL(g_AtomicVariables8.m_TestAndSetVariable > 0);
      XII_TEST_INT(g_AtomicVariables8.m_TestAndSetCounter, 1); // Only one thread should have set the variable.

      XII_TEST_BOOL(g_AtomicVariables8.m_CompareAndSwapVariable > 0);
      XII_TEST_INT(g_AtomicVariables8.m_CompareAndSwapCounter, 1); // Only one thread should have set the variable.

      g_AtomicVariables8.m_DecVariable = 0;
      XII_TEST_INT(g_AtomicVariables8.m_DecVariable.Decrement(), -1);
    }

    // xiiUInt8
    {
      XII_TEST_INT(g_AtomicVariablesU8.m_IncVariable, 2);
      XII_TEST_INT(g_AtomicVariablesU8.m_DecVariable, 254);

      XII_TEST_INT(g_AtomicVariablesU8.m_AddVariable, 3);
      XII_TEST_INT(g_AtomicVariablesU8.m_SubVariable, 253);

      XII_TEST_INT(g_AtomicVariablesU8.m_AndVariable, 0);
      XII_TEST_INT(g_AtomicVariablesU8.m_OrVariable, 7);
      XII_TEST_INT(g_AtomicVariablesU8.m_XorVariable, 5);

      XII_TEST_INT(g_AtomicVariablesU8.m_MinVariable, 1);
      XII_TEST_INT(g_AtomicVariablesU8.m_MaxVariable, 2);

      XII_TEST_BOOL(g_AtomicVariablesU8.m_SetVariable > 0);

      XII_TEST_BOOL(g_AtomicVariablesU8.m_TestAndSetVariable > 0);
      XII_TEST_INT(g_AtomicVariablesU8.m_TestAndSetCounter, 1); // Only one thread should have set the variable.

      XII_TEST_BOOL(g_AtomicVariablesU8.m_CompareAndSwapVariable > 0);
      XII_TEST_INT(g_AtomicVariablesU8.m_CompareAndSwapCounter, 1); // Only one thread should have set the variable.

      g_AtomicVariablesU8.m_DecVariable = 0;
      XII_TEST_INT(g_AtomicVariablesU8.m_DecVariable.Decrement(), xiiMath::MaxValue<xiiUInt8>());
    }

    // xiiInt16
    {
      XII_TEST_INT(g_AtomicVariables16.m_IncVariable, 2);
      XII_TEST_INT(g_AtomicVariables16.m_DecVariable, -2);

      XII_TEST_INT(g_AtomicVariables16.m_AddVariable, 3);
      XII_TEST_INT(g_AtomicVariables16.m_SubVariable, -3);

      XII_TEST_INT(g_AtomicVariables16.m_AndVariable, 0);
      XII_TEST_INT(g_AtomicVariables16.m_OrVariable, 7);
      XII_TEST_INT(g_AtomicVariables16.m_XorVariable, 5);

      XII_TEST_INT(g_AtomicVariables16.m_MinVariable, 1);
      XII_TEST_INT(g_AtomicVariables16.m_MaxVariable, 2);

      XII_TEST_BOOL(g_AtomicVariables16.m_SetVariable > 0);

      XII_TEST_BOOL(g_AtomicVariables16.m_TestAndSetVariable > 0);
      XII_TEST_INT(g_AtomicVariables16.m_TestAndSetCounter, 1); // Only one thread should have set the variable.

      XII_TEST_BOOL(g_AtomicVariables16.m_CompareAndSwapVariable > 0);
      XII_TEST_INT(g_AtomicVariables16.m_CompareAndSwapCounter, 1); // Only one thread should have set the variable.

      g_AtomicVariables16.m_DecVariable = 0;
      XII_TEST_INT(g_AtomicVariables16.m_DecVariable.Decrement(), -1);
    }

    // xiiUInt16
    {
      XII_TEST_INT(g_AtomicVariablesU16.m_IncVariable, 2);
      XII_TEST_INT(g_AtomicVariablesU16.m_DecVariable, 65534);

      XII_TEST_INT(g_AtomicVariablesU16.m_AddVariable, 3);
      XII_TEST_INT(g_AtomicVariablesU16.m_SubVariable, 65533);

      XII_TEST_INT(g_AtomicVariablesU16.m_AndVariable, 0);
      XII_TEST_INT(g_AtomicVariablesU16.m_OrVariable, 7);
      XII_TEST_INT(g_AtomicVariablesU16.m_XorVariable, 5);

      XII_TEST_INT(g_AtomicVariablesU16.m_MinVariable, 1);
      XII_TEST_INT(g_AtomicVariablesU16.m_MaxVariable, 2);

      XII_TEST_BOOL(g_AtomicVariablesU16.m_SetVariable > 0);

      XII_TEST_BOOL(g_AtomicVariablesU16.m_TestAndSetVariable > 0);
      XII_TEST_INT(g_AtomicVariablesU16.m_TestAndSetCounter, 1); // Only one thread should have set the variable.

      XII_TEST_BOOL(g_AtomicVariablesU16.m_CompareAndSwapVariable > 0);
      XII_TEST_INT(g_AtomicVariablesU16.m_CompareAndSwapCounter, 1); // Only one thread should have set the variable.

      g_AtomicVariablesU16.m_DecVariable = 0;
      XII_TEST_INT(g_AtomicVariablesU16.m_DecVariable.Decrement(), xiiMath::MaxValue<xiiUInt16>());
    }

    // xiiInt32
    {
      XII_TEST_INT(g_AtomicVariables32.m_IncVariable, 2);
      XII_TEST_INT(g_AtomicVariables32.m_DecVariable, -2);

      XII_TEST_INT(g_AtomicVariables32.m_AddVariable, 3);
      XII_TEST_INT(g_AtomicVariables32.m_SubVariable, -3);

      XII_TEST_INT(g_AtomicVariables32.m_AndVariable, 0);
      XII_TEST_INT(g_AtomicVariables32.m_OrVariable, 7);
      XII_TEST_INT(g_AtomicVariables32.m_XorVariable, 5);

      XII_TEST_INT(g_AtomicVariables32.m_MinVariable, 1);
      XII_TEST_INT(g_AtomicVariables32.m_MaxVariable, 2);

      XII_TEST_BOOL(g_AtomicVariables32.m_SetVariable > 0);

      XII_TEST_BOOL(g_AtomicVariables32.m_TestAndSetVariable > 0);
      XII_TEST_INT(g_AtomicVariables32.m_TestAndSetCounter, 1); // Only one thread should have set the variable.

      XII_TEST_BOOL(g_AtomicVariables32.m_CompareAndSwapVariable > 0);
      XII_TEST_INT(g_AtomicVariables32.m_CompareAndSwapCounter, 1); // Only one thread should have set the variable.

      g_AtomicVariables32.m_DecVariable = 0;
      XII_TEST_INT(g_AtomicVariables32.m_DecVariable.Decrement(), -1);
    }

    // xiiUInt32
    {
      XII_TEST_INT(g_AtomicVariablesU32.m_IncVariable, 2);
      XII_TEST_INT(g_AtomicVariablesU32.m_DecVariable, 4294967294);

      XII_TEST_INT(g_AtomicVariablesU32.m_AddVariable, 3);
      XII_TEST_INT(g_AtomicVariablesU32.m_SubVariable, 4294967293);

      XII_TEST_INT(g_AtomicVariablesU32.m_AndVariable, 0);
      XII_TEST_INT(g_AtomicVariablesU32.m_OrVariable, 7);
      XII_TEST_INT(g_AtomicVariablesU32.m_XorVariable, 5);

      XII_TEST_INT(g_AtomicVariablesU32.m_MinVariable, 1);
      XII_TEST_INT(g_AtomicVariablesU32.m_MaxVariable, 2);

      XII_TEST_BOOL(g_AtomicVariablesU32.m_SetVariable > 0);

      XII_TEST_BOOL(g_AtomicVariablesU32.m_TestAndSetVariable > 0);
      XII_TEST_INT(g_AtomicVariablesU32.m_TestAndSetCounter, 1); // Only one thread should have set the variable.

      XII_TEST_BOOL(g_AtomicVariablesU32.m_CompareAndSwapVariable > 0);
      XII_TEST_INT(g_AtomicVariablesU32.m_CompareAndSwapCounter, 1); // Only one thread should have set the variable.

      g_AtomicVariablesU32.m_DecVariable = 0;
      XII_TEST_INT(g_AtomicVariablesU32.m_DecVariable.Decrement(), xiiMath::MaxValue<xiiUInt32>());
    }

    // xiiInt64
    {
      XII_TEST_INT(g_AtomicVariables64.m_IncVariable, 2);
      XII_TEST_INT(g_AtomicVariables64.m_DecVariable, -2);

      XII_TEST_INT(g_AtomicVariables64.m_AddVariable, 3);
      XII_TEST_INT(g_AtomicVariables64.m_SubVariable, -3);

      XII_TEST_INT(g_AtomicVariables64.m_AndVariable, 0);
      XII_TEST_INT(g_AtomicVariables64.m_OrVariable, 7);
      XII_TEST_INT(g_AtomicVariables64.m_XorVariable, 5);

      XII_TEST_INT(g_AtomicVariables64.m_MinVariable, 1);
      XII_TEST_INT(g_AtomicVariables64.m_MaxVariable, 2);

      XII_TEST_BOOL(g_AtomicVariables64.m_SetVariable > 0);

      XII_TEST_BOOL(g_AtomicVariables64.m_TestAndSetVariable > 0);
      XII_TEST_INT(g_AtomicVariables64.m_TestAndSetCounter, 1); // Only one thread should have set the variable.

      XII_TEST_BOOL(g_AtomicVariables64.m_CompareAndSwapVariable > 0);
      XII_TEST_INT(g_AtomicVariables64.m_CompareAndSwapCounter, 1); // Only one thread should have set the variable.

      g_AtomicVariables64.m_DecVariable = 0;
      XII_TEST_INT(g_AtomicVariables64.m_DecVariable.Decrement(), -1);
    }

    // xiiUInt64
    {
      XII_TEST_INT(g_AtomicVariablesU64.m_IncVariable, 2);
      XII_TEST_INT(g_AtomicVariablesU64.m_DecVariable, 18446744073709551614);

      XII_TEST_INT(g_AtomicVariablesU64.m_AddVariable, 3);
      XII_TEST_INT(g_AtomicVariablesU64.m_SubVariable, 18446744073709551613);

      XII_TEST_INT(g_AtomicVariablesU64.m_AndVariable, 0);
      XII_TEST_INT(g_AtomicVariablesU64.m_OrVariable, 7);
      XII_TEST_INT(g_AtomicVariablesU64.m_XorVariable, 5);

      XII_TEST_INT(g_AtomicVariablesU64.m_MinVariable, 1);
      XII_TEST_INT(g_AtomicVariablesU64.m_MaxVariable, 2);

      XII_TEST_BOOL(g_AtomicVariablesU64.m_SetVariable > 0);

      XII_TEST_BOOL(g_AtomicVariablesU64.m_TestAndSetVariable > 0);
      XII_TEST_INT(g_AtomicVariablesU64.m_TestAndSetCounter, 1); // Only one thread should have set the variable.

      XII_TEST_BOOL(g_AtomicVariablesU64.m_CompareAndSwapVariable > 0);
      XII_TEST_INT(g_AtomicVariablesU64.m_CompareAndSwapCounter, 1); // Only one thread should have set the variable.

      g_AtomicVariablesU64.m_DecVariable = 0;
      XII_TEST_INT(g_AtomicVariablesU64.m_DecVariable.Decrement(), xiiMath::MaxValue<xiiUInt64>());
    }

    XII_TEST_BOOL(g_pTestAndSetPointer != nullptr);
    XII_TEST_INT(g_iTestAndSetPointerCounter, 1); // Only one thread should have set the variable.

    XII_TEST_BOOL(g_EnumSet == xiiVariantType::Float || g_EnumSet == xiiVariantType::Color);
    XII_TEST_BOOL(g_EnumTestAndSetVariable == xiiVariantType::Float || g_EnumTestAndSetVariable == xiiVariantType::Color);
    XII_TEST_INT(g_iTestAndSetCounterEnum, 1);
    XII_TEST_BOOL(g_EnumCompareAndSwap == xiiVariantType::Float || g_EnumCompareAndSwap == xiiVariantType::Color);
    XII_TEST_INT(g_iCompareAndSwapCounterEnum, 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Post Increment Atomics (basics)")
  {
    // xiiInt8
    {
      g_AtomicPostOperations8.m_PostIncVariable = 0;

      XII_TEST_INT(g_AtomicPostOperations8.m_PostIncVariable.PostIncrement(), 0);
      XII_TEST_INT(g_AtomicPostOperations8.m_PostIncVariable, 1);

      g_AtomicPostOperations8.m_PostDecVariable = 0;

      XII_TEST_INT(g_AtomicPostOperations8.m_PostDecVariable.PostDecrement(), 0);
      XII_TEST_INT(g_AtomicPostOperations8.m_PostDecVariable, -1);
    }

    // xiiUInt8
    {
      g_AtomicPostOperationsU8.m_PostIncVariable = 0;

      XII_TEST_INT(g_AtomicPostOperationsU8.m_PostIncVariable.PostIncrement(), 0);
      XII_TEST_INT(g_AtomicPostOperationsU8.m_PostIncVariable, 1);

      g_AtomicPostOperationsU8.m_PostDecVariable = 0;

      XII_TEST_INT(g_AtomicPostOperationsU8.m_PostDecVariable.PostDecrement(), 0);
      XII_TEST_INT(g_AtomicPostOperationsU8.m_PostDecVariable, xiiMath::MaxValue<xiiUInt8>());
    }

    // xiiInt16
    {
      g_AtomicPostOperations16.m_PostIncVariable = 0;

      XII_TEST_INT(g_AtomicPostOperations16.m_PostIncVariable.PostIncrement(), 0);
      XII_TEST_INT(g_AtomicPostOperations16.m_PostIncVariable, 1);

      g_AtomicPostOperations16.m_PostDecVariable = 0;

      XII_TEST_INT(g_AtomicPostOperations16.m_PostDecVariable.PostDecrement(), 0);
      XII_TEST_INT(g_AtomicPostOperations16.m_PostDecVariable, -1);
    }

    // xiiUInt16
    {
      g_AtomicPostOperationsU16.m_PostIncVariable = 0;

      XII_TEST_INT(g_AtomicPostOperationsU16.m_PostIncVariable.PostIncrement(), 0);
      XII_TEST_INT(g_AtomicPostOperationsU16.m_PostIncVariable, 1);

      g_AtomicPostOperationsU16.m_PostDecVariable = 0;

      XII_TEST_INT(g_AtomicPostOperationsU16.m_PostDecVariable.PostDecrement(), 0);
      XII_TEST_INT(g_AtomicPostOperationsU16.m_PostDecVariable, xiiMath::MaxValue<xiiUInt16>());
    }

    // xiiInt32
    {
      g_AtomicPostOperations32.m_PostIncVariable = 0;

      XII_TEST_INT(g_AtomicPostOperations32.m_PostIncVariable.PostIncrement(), 0);
      XII_TEST_INT(g_AtomicPostOperations32.m_PostIncVariable, 1);

      g_AtomicPostOperations32.m_PostDecVariable = 0;

      XII_TEST_INT(g_AtomicPostOperations32.m_PostDecVariable.PostDecrement(), 0);
      XII_TEST_INT(g_AtomicPostOperations32.m_PostDecVariable, -1);
    }

    // xiiUInt32
    {
      g_AtomicPostOperationsU32.m_PostIncVariable = 0;

      XII_TEST_INT(g_AtomicPostOperationsU32.m_PostIncVariable.PostIncrement(), 0);
      XII_TEST_INT(g_AtomicPostOperationsU32.m_PostIncVariable, 1);

      g_AtomicPostOperationsU32.m_PostDecVariable = 0;

      XII_TEST_INT(g_AtomicPostOperationsU32.m_PostDecVariable.PostDecrement(), 0);
      XII_TEST_INT(g_AtomicPostOperationsU32.m_PostDecVariable, xiiMath::MaxValue<xiiUInt32>());
    }

    // xiiInt64
    {
      g_AtomicPostOperations64.m_PostIncVariable = 0;

      XII_TEST_INT(g_AtomicPostOperations64.m_PostIncVariable.PostIncrement(), 0);
      XII_TEST_INT(g_AtomicPostOperations64.m_PostIncVariable, 1);

      g_AtomicPostOperations64.m_PostDecVariable = 0;

      XII_TEST_INT(g_AtomicPostOperations64.m_PostDecVariable.PostDecrement(), 0);
      XII_TEST_INT(g_AtomicPostOperations64.m_PostDecVariable, -1);
    }

    // xiiUInt64
    {
      g_AtomicPostOperationsU64.m_PostIncVariable = 0;

      XII_TEST_INT(g_AtomicPostOperationsU64.m_PostIncVariable.PostIncrement(), 0);
      XII_TEST_INT(g_AtomicPostOperationsU64.m_PostIncVariable, 1);

      g_AtomicPostOperationsU64.m_PostDecVariable = 0;

      XII_TEST_INT(g_AtomicPostOperationsU64.m_PostDecVariable.PostDecrement(), 0);
      XII_TEST_INT(g_AtomicPostOperationsU64.m_PostDecVariable, xiiMath::MaxValue<xiiUInt64>());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Post Increment Atomics")
  {
    const xiiUInt32 uiThreadCount = 64U;

    // Used to check that every integer value in range [0; uiThreadCount] is returned exactly once.
    g_PostIncDecThreads.SetCount(uiThreadCount);

    // xiiInt8
    {
      // Used to check that every integer value in range [0; uiThreadCount] is returned exactly once.
      g_AtomicPostOperations8.m_PostIncValues.SetCount(uiThreadCount);
      g_AtomicPostOperations8.m_PostDecValues.SetCount(uiThreadCount);

      // Count up and down.
      g_AtomicPostOperations8.m_PostIncVariable = 0;
      g_AtomicPostOperations8.m_PostDecVariable = uiThreadCount - 1U;
    }

    // xiiUInt8
    {
      // Used to check that every integer value in range [0; uiThreadCount] is returned exactly once.
      g_AtomicPostOperationsU8.m_PostIncValues.SetCount(uiThreadCount);
      g_AtomicPostOperationsU8.m_PostDecValues.SetCount(uiThreadCount);

      // Count up and down.
      g_AtomicPostOperationsU8.m_PostIncVariable = 0;
      g_AtomicPostOperationsU8.m_PostDecVariable = uiThreadCount - 1U;
    }

    // xiiInt16
    {
      // Used to check that every integer value in range [0; uiThreadCount] is returned exactly once.
      g_AtomicPostOperations16.m_PostIncValues.SetCount(uiThreadCount);
      g_AtomicPostOperations16.m_PostDecValues.SetCount(uiThreadCount);

      // Count up and down.
      g_AtomicPostOperations16.m_PostIncVariable = 0;
      g_AtomicPostOperations16.m_PostDecVariable = uiThreadCount - 1U;
    }

    // xiiUInt16
    {
      // Used to check that every integer value in range [0; uiThreadCount] is returned exactly once.
      g_AtomicPostOperationsU16.m_PostIncValues.SetCount(uiThreadCount);
      g_AtomicPostOperationsU16.m_PostDecValues.SetCount(uiThreadCount);

      // Count up and down.
      g_AtomicPostOperationsU16.m_PostIncVariable = 0;
      g_AtomicPostOperationsU16.m_PostDecVariable = uiThreadCount - 1U;
    }

    // xiiInt32
    {
      // Used to check that every integer value in range [0; uiThreadCount] is returned exactly once.
      g_AtomicPostOperations32.m_PostIncValues.SetCount(uiThreadCount);
      g_AtomicPostOperations32.m_PostDecValues.SetCount(uiThreadCount);

      // Count up and down.
      g_AtomicPostOperations32.m_PostIncVariable = 0;
      g_AtomicPostOperations32.m_PostDecVariable = uiThreadCount - 1U;
    }

    // xiiUInt32
    {
      // Used to check that every integer value in range [0; uiThreadCount] is returned exactly once.
      g_AtomicPostOperationsU32.m_PostIncValues.SetCount(uiThreadCount);
      g_AtomicPostOperationsU32.m_PostDecValues.SetCount(uiThreadCount);

      // Count up and down.
      g_AtomicPostOperationsU32.m_PostIncVariable = 0;
      g_AtomicPostOperationsU32.m_PostDecVariable = uiThreadCount - 1U;
    }

    // xiiInt64
    {
      // Used to check that every integer value in range [0; uiThreadCount] is returned exactly once.
      g_AtomicPostOperations64.m_PostIncValues.SetCount(uiThreadCount);
      g_AtomicPostOperations64.m_PostDecValues.SetCount(uiThreadCount);

      // Count up and down.
      g_AtomicPostOperations64.m_PostIncVariable = 0;
      g_AtomicPostOperations64.m_PostDecVariable = uiThreadCount - 1U;
    }

    // xiiUInt64
    {
      // Used to check that every integer value in range [0; uiThreadCount] is returned exactly once.
      g_AtomicPostOperationsU64.m_PostIncValues.SetCount(uiThreadCount);
      g_AtomicPostOperationsU64.m_PostDecValues.SetCount(uiThreadCount);

      // Count up and down.
      g_AtomicPostOperationsU64.m_PostIncVariable = 0;
      g_AtomicPostOperationsU64.m_PostDecVariable = uiThreadCount - 1U;
    }

    for (xiiUInt32 t = 0; t < uiThreadCount; ++t)
    {
      g_PostIncDecThreads[t] = XII_DEFAULT_NEW(PostIncDecThread, t);
    }

    for (xiiUInt32 t = 0; t < uiThreadCount; ++t)
    {
      g_PostIncDecThreads[t]->Start();
    }

    for (xiiUInt32 t = 0; t < uiThreadCount; ++t)
    {
      g_PostIncDecThreads[t]->Join();
    }

    // check that every value was returned exactly once
    for (xiiUInt32 t = 0; t < uiThreadCount; ++t)
    {
      XII_TEST_INT(g_AtomicPostOperations8.m_PostIncValues[t], 1);
      XII_TEST_INT(g_AtomicPostOperations8.m_PostDecValues[t], 1);

      XII_TEST_INT(g_AtomicPostOperationsU8.m_PostIncValues[t], 1);
      XII_TEST_INT(g_AtomicPostOperationsU8.m_PostDecValues[t], 1);

      XII_TEST_INT(g_AtomicPostOperations16.m_PostIncValues[t], 1);
      XII_TEST_INT(g_AtomicPostOperations16.m_PostDecValues[t], 1);

      XII_TEST_INT(g_AtomicPostOperationsU16.m_PostIncValues[t], 1);
      XII_TEST_INT(g_AtomicPostOperationsU16.m_PostDecValues[t], 1);

      XII_TEST_INT(g_AtomicPostOperations32.m_PostIncValues[t], 1);
      XII_TEST_INT(g_AtomicPostOperations32.m_PostDecValues[t], 1);

      XII_TEST_INT(g_AtomicPostOperationsU32.m_PostIncValues[t], 1);
      XII_TEST_INT(g_AtomicPostOperationsU32.m_PostDecValues[t], 1);

      XII_TEST_INT(g_AtomicPostOperations64.m_PostIncValues[t], 1);
      XII_TEST_INT(g_AtomicPostOperations64.m_PostDecValues[t], 1);

      XII_TEST_INT(g_AtomicPostOperationsU64.m_PostIncValues[t], 1);
      XII_TEST_INT(g_AtomicPostOperationsU64.m_PostDecValues[t], 1);
    }

    g_PostIncDecThreads.Clear();
    g_AtomicPostOperations8.m_PostIncValues.Clear();
    g_AtomicPostOperations8.m_PostDecValues.Clear();
    g_AtomicPostOperationsU8.m_PostIncValues.Clear();
    g_AtomicPostOperationsU8.m_PostDecValues.Clear();
    g_AtomicPostOperations16.m_PostIncValues.Clear();
    g_AtomicPostOperations16.m_PostDecValues.Clear();
    g_AtomicPostOperationsU16.m_PostIncValues.Clear();
    g_AtomicPostOperationsU16.m_PostDecValues.Clear();
    g_AtomicPostOperations32.m_PostIncValues.Clear();
    g_AtomicPostOperations32.m_PostDecValues.Clear();
    g_AtomicPostOperationsU32.m_PostIncValues.Clear();
    g_AtomicPostOperationsU32.m_PostDecValues.Clear();
    g_AtomicPostOperations64.m_PostIncValues.Clear();
    g_AtomicPostOperations64.m_PostDecValues.Clear();
    g_AtomicPostOperationsU64.m_PostIncValues.Clear();
    g_AtomicPostOperationsU64.m_PostDecValues.Clear();
  }
}
