#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/Message.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>

/* Performance Statistics:

  AMD E-350 Processor 1.6 GHz ('Fusion'), 32 Bit, Debug Mode
    Virtual Function Calls:   ~60 ns
    Simple Function Calls:    ~27 ns
    Fastcall Function Calls:  ~27 ns
    Integer Division:         52 ns
    Integer Multiplication:   23 ns
    Float Division:           25 ns
    Float Multiplication:     25 ns

  AMD E-350 Processor 1.6 GHz ('Fusion'), 64 Bit, Debug Mode
    Virtual Function Calls:   ~80 ns
    Simple Function Calls:    ~55 ns
    Fastcall Function Calls:  ~55 ns
    Integer Division:         ~97 ns
    Integer Multiplication:   ~52 ns
    Float Division:           ~66 ns
    Float Multiplication:     ~58 ns

  AMD E-350 Processor 1.6 GHz ('Fusion'), 32 Bit, Release Mode
    Virtual Function Calls:   ~9 ns
    Simple Function Calls:    ~5 ns
    Fastcall Function Calls:  ~5 ns
    Integer Division:         35 ns
    Integer Multiplication:   3.78 ns
    Float Division:           10.7 ns
    Float Multiplication:     9.5 ns

  AMD E-350 Processor 1.6 GHz ('Fusion'), 64 Bit, Release Mode
    Virtual Function Calls:   ~10 ns
    Simple Function Calls:    ~5 ns
    Fastcall Function Calls:  ~5 ns
    Integer Division:         35 ns
    Integer Multiplication:   3.23 ns
    Float Division:           8.13 ns
    Float Multiplication:     4.13 ns

  Intel Core i7 3770 3.4 GHz, 64 Bit, Release Mode
    Virtual Function Calls:   ~3.8 ns
    Simple Function Calls:    ~4.4 ns
    Fastcall Function Calls:  ~4.0 ns
    Integer Division:         8.25 ns
    Integer Multiplication:   1.55 ns
    Float Division:           4.40 ns
    Float Multiplication:     1.87 ns

*/

XII_CREATE_SIMPLE_TEST_GROUP(Performance);

struct xiiMsgTest : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgTest, xiiMessage);
};

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgTest);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgTest, 1, xiiRTTIDefaultAllocator<xiiMsgTest>)
XII_END_DYNAMIC_REFLECTED_TYPE;


struct GetValueMessage : public xiiMsgTest
{
  XII_DECLARE_MESSAGE_TYPE(GetValueMessage, xiiMsgTest);

  xiiInt32 m_iValue;
};
XII_IMPLEMENT_MESSAGE_TYPE(GetValueMessage);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(GetValueMessage, 1, xiiRTTIDefaultAllocator<GetValueMessage>)
XII_END_DYNAMIC_REFLECTED_TYPE;



class Base : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(Base, xiiReflectedClass);

public:
  virtual ~Base() = default;

  virtual xiiInt32 Virtual() = 0;
};

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(Base, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  define XII_FASTCALL  __fastcall
#  define XII_NO_INLINE __declspec(noinline)
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX)
#  if XII_ENABLED(XII_PLATFORM_ARCH_X86) && XII_ENABLED(XII_PLATFORM_32BIT)
#    define XII_FASTCALL __attribute((fastcall)) // Fastcall only relevant on x86-32 and would otherwise generate warnings
#  else
#    define XII_FASTCALL
#  endif
#  define XII_NO_INLINE __attribute__((noinline))
#else
#  warning Unknown Platform.
#  define XII_FASTCALL
#  define XII_NO_INLINE __attribute__((noinline)) /* should work on GCC */
#endif

class Derived1 : public Base
{
  XII_ADD_DYNAMIC_REFLECTION(Derived1, Base);

public:
  XII_NO_INLINE xiiInt32 XII_FASTCALL FastCall() { return 1; }
  XII_NO_INLINE xiiInt32              NonVirtual() { return 1; }
  XII_NO_INLINE virtual xiiInt32      Virtual() override { return 1; }
  XII_NO_INLINE void                  OnGetValueMessage(GetValueMessage& ref_msg) { ref_msg.m_iValue = 1; }
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(Derived1, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(GetValueMessage, OnGetValueMessage),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class Derived2 : public Base
{
  XII_ADD_DYNAMIC_REFLECTION(Derived2, Base);

public:
  XII_NO_INLINE xiiInt32 XII_FASTCALL FastCall() { return 2; }
  XII_NO_INLINE xiiInt32              NonVirtual() { return 2; }
  XII_NO_INLINE virtual xiiInt32      Virtual() override { return 2; }
  XII_NO_INLINE void                  OnGetValueMessage(GetValueMessage& ref_msg) { ref_msg.m_iValue = 2; }
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(Derived2, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(GetValueMessage, OnGetValueMessage),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_CREATE_SIMPLE_TEST(Performance, Basics)
{
  const xiiInt32 iNumObjects = 1000000;
  const float    fNumObjects = (float)iNumObjects;

  xiiDynamicArray<Derived1> Der1;
  Der1.SetCount(iNumObjects / 2);

  xiiDynamicArray<Derived2> Der2;
  Der2.SetCount(iNumObjects / 2);

  xiiDynamicArray<Base*> Objects;
  Objects.SetCount(iNumObjects);

  for (xiiInt32 i = 0; i < iNumObjects; i += 2)
  {
    Objects[i]     = &Der1[i / 2];
    Objects[i + 1] = &Der2[i / 2];
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Dispatch Message")
  {
    xiiInt32 iResult = 0;

    // warm up
    for (xiiUInt32 i = 0; i < iNumObjects; ++i)
    {
      GetValueMessage msg;
      Objects[i]->GetDynamicRTTI()->DispatchMessage(Objects[i], msg);
      iResult += msg.m_iValue;
    }

    xiiTime t0 = xiiTime::Now();

    for (xiiUInt32 i = 0; i < iNumObjects; ++i)
    {
      GetValueMessage msg;
      Objects[i]->GetDynamicRTTI()->DispatchMessage(Objects[i], msg);
      iResult += msg.m_iValue;
    }

    xiiTime t1 = xiiTime::Now();

    XII_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    xiiTime tdiff = t1 - t0;
    double  tFC   = tdiff.GetNanoseconds() / (double)iNumObjects;

    xiiLog::Info("[test]Dispatch Message: {0}ns", xiiArgF(tFC, 2), iResult);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Virtual")
  {
    xiiInt32 iResult = 0;

    // warm up
    for (xiiUInt32 i = 0; i < iNumObjects; ++i)
      iResult += Objects[i]->Virtual();

    xiiTime t0 = xiiTime::Now();

    for (xiiUInt32 i = 0; i < iNumObjects; ++i)
      iResult += Objects[i]->Virtual();

    xiiTime t1 = xiiTime::Now();

    XII_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    xiiTime tdiff = t1 - t0;
    double  tFC   = tdiff.GetNanoseconds() / (double)iNumObjects;

    xiiLog::Info("[test]Virtual Function Calls: {0}ns", xiiArgF(tFC, 2), iResult);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "NonVirtual")
  {
    xiiInt32 iResult = 0;

    // warm up
    for (xiiUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*)Objects[i])->NonVirtual();
      iResult += ((Derived2*)Objects[i])->NonVirtual();
    }

    xiiTime t0 = xiiTime::Now();

    for (xiiUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*)Objects[i])->NonVirtual();
      iResult += ((Derived2*)Objects[i])->NonVirtual();
    }

    xiiTime t1 = xiiTime::Now();

    XII_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    xiiTime tdiff = t1 - t0;
    double  tFC   = tdiff.GetNanoseconds() / (double)iNumObjects;

    xiiLog::Info("[test]Non-Virtual Function Calls: {0}ns", xiiArgF(tFC, 2), iResult);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "FastCall")
  {
    xiiInt32 iResult = 0;

    // warm up
    for (xiiUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*)Objects[i])->FastCall();
      iResult += ((Derived2*)Objects[i])->FastCall();
    }

    xiiTime t0 = xiiTime::Now();

    for (xiiUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*)Objects[i])->FastCall();
      iResult += ((Derived2*)Objects[i])->FastCall();
    }

    xiiTime t1 = xiiTime::Now();

    XII_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    xiiTime tdiff = t1 - t0;
    double  tFC   = tdiff.GetNanoseconds() / (double)iNumObjects;

    xiiLog::Info("[test]FastCall Function Calls: {0}ns", xiiArgF(tFC, 2), iResult);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "32 Bit Integer Division")
  {
    xiiDynamicArray<xiiInt32> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (xiiInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = i * 100;

    xiiTime t0 = xiiTime::Now();

    xiiInt32 iResult = 0;

    for (xiiInt32 i = 1; i < iNumObjects; i += 1)
      iResult += Ints[i] / i;

    xiiTime t1 = xiiTime::Now();

    xiiTime tdiff = t1 - t0;
    double  t     = tdiff.GetNanoseconds() / (double)(iNumObjects - 1);

    xiiLog::Info("[test]32 Bit Integer Division: {0}ns", xiiArgF(t, 2), iResult);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "32 Bit Integer Multiplication")
  {
    xiiDynamicArray<xiiInt32> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (xiiInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = iNumObjects - i;

    xiiTime t0 = xiiTime::Now();

    xiiInt32 iResult = 0;

    for (xiiInt32 i = 0; i < iNumObjects; i += 1)
      iResult += Ints[i] * i;

    xiiTime t1 = xiiTime::Now();

    xiiTime tdiff = t1 - t0;
    double  t     = tdiff.GetNanoseconds() / (double)(iNumObjects);

    xiiLog::Info("[test]32 Bit Integer Multiplication: {0}ns", xiiArgF(t, 2), iResult);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "64 Bit Integer Division")
  {
    xiiDynamicArray<xiiInt64> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (xiiInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = (xiiInt64)i * (xiiInt64)100;

    xiiTime t0 = xiiTime::Now();

    xiiInt64 iResult = 0;

    for (xiiInt32 i = 1; i < iNumObjects; i += 1)
      iResult += Ints[i] / (xiiInt64)i;

    xiiTime t1 = xiiTime::Now();

    xiiTime tdiff = t1 - t0;
    double  t     = tdiff.GetNanoseconds() / (double)(iNumObjects - 1);

    xiiLog::Info("[test]64 Bit Integer Division: {0}ns", xiiArgF(t, 2), iResult);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "64 Bit Integer Multiplication")
  {
    xiiDynamicArray<xiiInt64> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (xiiInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = iNumObjects - i;

    xiiTime t0 = xiiTime::Now();

    xiiInt64 iResult = 0;

    for (xiiInt32 i = 0; i < iNumObjects; i += 1)
      iResult += Ints[i] * (xiiInt64)i;

    xiiTime t1 = xiiTime::Now();

    xiiTime tdiff = t1 - t0;
    double  t     = tdiff.GetNanoseconds() / (double)(iNumObjects);

    xiiLog::Info("[test]64 Bit Integer Multiplication: {0}ns", xiiArgF(t, 2), iResult);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "32 Bit Float Division")
  {
    xiiDynamicArray<float> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (xiiInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = i * 100.0f;

    xiiTime t0 = xiiTime::Now();

    float fResult = 0;

    float d = 1.0f;
    for (xiiInt32 i = 0; i < iNumObjects; i++, d += 1.0f)
      fResult += Ints[i] / d;

    xiiTime t1 = xiiTime::Now();

    xiiTime tdiff = t1 - t0;
    double  t     = tdiff.GetNanoseconds() / (double)(iNumObjects);

    xiiLog::Info("[test]32 Bit Float Division: {0}ns", xiiArgF(t, 2), fResult);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "32 Bit Float Multiplication")
  {
    xiiDynamicArray<float> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (xiiInt32 i = 0; i < iNumObjects; i++)
      Ints[i] = (float)(fNumObjects) - (float)(i);

    xiiTime t0 = xiiTime::Now();

    float iResult = 0;

    float d = 1.0f;
    for (xiiInt32 i = 0; i < iNumObjects; i++, d += 1.0f)
      iResult += Ints[i] * d;

    xiiTime t1 = xiiTime::Now();

    xiiTime tdiff = t1 - t0;
    double  t     = tdiff.GetNanoseconds() / (double)(iNumObjects);

    xiiLog::Info("[test]32 Bit Float Multiplication: {0}ns", xiiArgF(t, 2), iResult);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "64 Bit Double Division")
  {
    xiiDynamicArray<double> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (xiiInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = i * 100.0;

    xiiTime t0 = xiiTime::Now();

    double fResult = 0;

    double d = 1.0;
    for (xiiInt32 i = 0; i < iNumObjects; i++, d += 1.0f)
      fResult += Ints[i] / d;

    xiiTime t1 = xiiTime::Now();

    xiiTime tdiff = t1 - t0;
    double  t     = tdiff.GetNanoseconds() / (double)(iNumObjects);

    xiiLog::Info("[test]64 Bit Double Division: {0}ns", xiiArgF(t, 2), fResult);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "64 Bit Double Multiplication")
  {
    xiiDynamicArray<double> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (xiiInt32 i = 0; i < iNumObjects; i++)
      Ints[i] = (double)(fNumObjects) - (double)(i);

    xiiTime t0 = xiiTime::Now();

    double iResult = 0;

    double d = 1.0;
    for (xiiInt32 i = 0; i < iNumObjects; i++, d += 1.0)
      iResult += Ints[i] * d;

    xiiTime t1 = xiiTime::Now();

    xiiTime tdiff = t1 - t0;
    double  t     = tdiff.GetNanoseconds() / (double)(iNumObjects);

    xiiLog::Info("[test]64 Bit Double Multiplication: {0}ns", xiiArgF(t, 2), iResult);
  }
}
