#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

namespace
{
  struct TestType
  {
    TestType(){}; // NOLINT: Allow default construction.

    xiiInt32 MethodWithManyParams(xiiInt32 a, xiiInt32 b, xiiInt32 c, xiiInt32 d, xiiInt32 e, xiiInt32 f) { return m_iA + a + b + c + d + e + f; }

    xiiInt32 Method(xiiInt32 b) { return b + m_iA; }

    xiiInt32 ConstMethod(xiiInt32 b) const { return b + m_iA + 4; }

    virtual xiiInt32 VirtualMethod(xiiInt32 b) { return b; }

    mutable xiiInt32 m_iA;
  };

  struct TestTypeDerived : public TestType
  {
    xiiInt32 Method(xiiInt32 b) { return b + 4; }

    virtual xiiInt32 VirtualMethod(xiiInt32 b) override { return b + 43; }
  };

  struct BaseA
  {
    virtual ~BaseA() = default;
    virtual void bar() {}

    int m_i1;
  };

  struct BaseB
  {
    virtual ~BaseB() = default;
    virtual void foo() {}
    int          m_i2;
  };

  struct ComplexClass : public BaseA, public BaseB
  {
    ComplexClass() { m_ctorDel = xiiMakeDelegate(&ComplexClass::nonVirtualFunc, this); }

    virtual ~ComplexClass()
    {
      m_dtorDel = xiiMakeDelegate(&ComplexClass::nonVirtualFunc, this);
      XII_TEST_BOOL(m_ctorDel.IsEqualIfComparable(m_dtorDel));
    }
    virtual void bar() override {}
    virtual void foo() override {}



    void nonVirtualFunc()
    {
      m_i1 = 1;
      m_i2 = 2;
      m_i3 = 3;
    }

    int m_i3;

    xiiDelegate<void()> m_ctorDel;
    xiiDelegate<void()> m_dtorDel;
  };

  static xiiInt32 Function(xiiInt32 b) { return b + 2; }
} // namespace

XII_CREATE_SIMPLE_TEST(Basics, Delegate)
{
  typedef xiiDelegate<xiiInt32(xiiInt32)> TestDelegate;
  TestDelegate                            d;

#if XII_ENABLED(XII_PLATFORM_64BIT)
  XII_TEST_BOOL(sizeof(d) == 32);
#endif

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Method")
  {
    TestTypeDerived test;
    test.m_iA = 42;

    d = TestDelegate(&TestType::Method, &test);
    XII_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestType::Method, &test)));
    XII_TEST_BOOL(d.IsComparable());
    XII_TEST_INT(d(4), 46);

    d = TestDelegate(&TestTypeDerived::Method, &test);
    XII_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestTypeDerived::Method, &test)));
    XII_TEST_BOOL(d.IsComparable());
    XII_TEST_INT(d(4), 8);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Method With Many Params")
  {
    typedef xiiDelegate<xiiInt32(xiiInt32, xiiInt32, xiiInt32, xiiInt32, xiiInt32, xiiInt32)> TestDelegateMany;
    TestDelegateMany                                                                          many;

    TestType test;
    test.m_iA = 1000000;

    many = TestDelegateMany(&TestType::MethodWithManyParams, &test);
    XII_TEST_BOOL(many.IsEqualIfComparable(TestDelegateMany(&TestType::MethodWithManyParams, &test)));
    XII_TEST_BOOL(d.IsComparable());
    XII_TEST_INT(many(1, 10, 100, 1000, 10000, 100000), 1111111);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Complex Class")
  {
    XII_WARNING_PUSH()
    XII_WARNING_DISABLE_GCC("-Wfree-nonheap-object")

    ComplexClass* c = new ComplexClass();
    delete c;

    XII_WARNING_POP()
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Const Method")
  {
    const TestType constTest;
    constTest.m_iA = 35;

    d = TestDelegate(&TestType::ConstMethod, &constTest);
    XII_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestType::ConstMethod, &constTest)));
    XII_TEST_BOOL(d.IsComparable());
    XII_TEST_INT(d(4), 43);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Virtual Method")
  {
    TestTypeDerived test;

    d = TestDelegate(&TestType::VirtualMethod, &test);
    XII_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestType::VirtualMethod, &test)));
    XII_TEST_BOOL(d.IsComparable());
    XII_TEST_INT(d(4), 47);

    d = TestDelegate(&TestTypeDerived::VirtualMethod, &test);
    XII_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestTypeDerived::VirtualMethod, &test)));
    XII_TEST_BOOL(d.IsComparable());
    XII_TEST_INT(d(4), 47);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Function")
  {
    d = &Function;
    XII_TEST_BOOL(d.IsEqualIfComparable(&Function));
    XII_TEST_BOOL(d.IsComparable());
    XII_TEST_INT(d(4), 6);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Lambda - no capture")
  {
    d = [](xiiInt32 i) {
      return i * 4;
    };
    XII_TEST_BOOL(d.IsComparable());
    XII_TEST_INT(d(2), 8);

    TestDelegate d2 = d;
    XII_TEST_BOOL(d2.IsEqualIfComparable(d));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Lambda - capture by value")
  {
    xiiInt32 c = 20;
    d          = [c](xiiInt32) {
      return c;
    };
    XII_TEST_BOOL(!d.IsComparable());
    XII_TEST_INT(d(3), 20);
    c = 10;
    XII_TEST_INT(d(3), 20);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Lambda - capture by value, mutable")
  {
    xiiInt32 c = 20;
    d          = [c](xiiInt32) mutable {
      return c;
    };
    XII_TEST_BOOL(!d.IsComparable());
    XII_TEST_INT(d(3), 20);
    c = 10;
    XII_TEST_INT(d(3), 20);

    d = [c](xiiInt32 b) mutable -> decltype(b + c) {
      auto result = b + c;
      c           = 1;
      return result;
    };
    XII_TEST_BOOL(!d.IsComparable());
    XII_TEST_INT(d(3), 13);
    XII_TEST_INT(d(3), 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Lambda - capture by reference")
  {
    xiiInt32 c = 20;
    d          = [&c](xiiInt32 i) -> decltype(i) {
      c = 5;
      return i;
    };
    XII_TEST_BOOL(!d.IsComparable());
    XII_TEST_INT(d(3), 3);
    XII_TEST_INT(c, 5);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Lambda - capture by value of non-pod")
  {
    struct RefCountedInt : public xiiRefCounted
    {
      RefCountedInt() = default;
      RefCountedInt(int i) :
        m_value(i)
      {
      }
      int m_value;
    };

    xiiSharedPtr<RefCountedInt> shared = XII_DEFAULT_NEW(RefCountedInt, 1);
    XII_TEST_INT(shared->GetRefCount(), 1);
    {
      TestDelegate deleteMe = [shared](xiiInt32 i) -> decltype(i) {
        return 0;
      };
      XII_TEST_BOOL(!deleteMe.IsComparable());
      XII_TEST_INT(shared->GetRefCount(), 2);
    }
    XII_TEST_INT(shared->GetRefCount(), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Lambda - capture lots of things")
  {
    xiiInt64 a = 10;
    xiiInt64 b = 20;
    xiiInt64 c = 30;
    d          = [a, b, c](xiiInt32 i) -> xiiInt32 {
      return static_cast<xiiInt32>(a + b + c + i);
    };
    XII_TEST_INT(d(6), 66);
    XII_TEST_BOOL(!d.IsComparable());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Lambda - capture lots of things - custom allocator")
  {
    xiiInt64 a = 10;
    xiiInt64 b = 20;
    xiiInt64 c = 30;
    d          = TestDelegate([a, b, c](xiiInt32 i) -> xiiInt32 { return static_cast<xiiInt32>(a + b + c + i); }, xiiFoundation::GetAlignedAllocator());
    XII_TEST_INT(d(6), 66);
    XII_TEST_BOOL(!d.IsComparable());

    d.Invalidate();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move semantics")
  {
    // Move pure function
    {
      d.Invalidate();
      TestDelegate d2 = &Function;
      d               = std::move(d2);
      XII_TEST_BOOL(d.IsValid());
      XII_TEST_BOOL(!d2.IsValid());
      XII_TEST_BOOL(d.IsComparable());
      XII_TEST_INT(d(4), 6);
    }

    // Move delegate
    xiiConstructionCounter::Reset();
    d.Invalidate();
    {
      xiiConstructionCounter value;
      value.m_iData = 666;
      XII_TEST_INT(xiiConstructionCounter::s_iConstructions, 1);
      XII_TEST_INT(xiiConstructionCounter::s_iDestructions, 0);
      TestDelegate d2 = [value](xiiInt32 i) -> xiiInt32 {
        return value.m_iData;
      };
      XII_TEST_INT(xiiConstructionCounter::s_iConstructions, 3); // Capture plus moving the lambda.
      XII_TEST_INT(xiiConstructionCounter::s_iDestructions, 1);  // Move of lambda
      d = std::move(d2);
      // Moving a construction counter also counts as construction
      XII_TEST_INT(xiiConstructionCounter::s_iConstructions, 4);
      XII_TEST_INT(xiiConstructionCounter::s_iDestructions, 1);
      XII_TEST_BOOL(d.IsValid());
      XII_TEST_BOOL(!d2.IsValid());
      XII_TEST_BOOL(!d.IsComparable());
      XII_TEST_INT(d(0), 666);
    }
    XII_TEST_INT(xiiConstructionCounter::s_iDestructions, 2); // value out of scope
    XII_TEST_INT(xiiConstructionCounter::s_iConstructions, 4);
    d.Invalidate();
    XII_TEST_INT(xiiConstructionCounter::s_iDestructions, 3); // lambda destroyed.
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Lambda - Copy")
  {
    d.Invalidate();
    xiiConstructionCounter::Reset();
    {
      xiiConstructionCounter value;
      value.m_iData = 666;
      XII_TEST_INT(xiiConstructionCounter::s_iConstructions, 1);
      XII_TEST_INT(xiiConstructionCounter::s_iDestructions, 0);
      TestDelegate d2 = TestDelegate([value](xiiInt32 i) -> xiiInt32 { return value.m_iData; }, xiiFoundation::GetAlignedAllocator());
      XII_TEST_INT(xiiConstructionCounter::s_iConstructions, 3); // Capture plus moving the lambda.
      XII_TEST_INT(xiiConstructionCounter::s_iDestructions, 1);  // Move of lambda
      d = d2;
      XII_TEST_INT(xiiConstructionCounter::s_iConstructions, 4); // Lambda Copy
      XII_TEST_INT(xiiConstructionCounter::s_iDestructions, 1);
      XII_TEST_BOOL(d.IsValid());
      XII_TEST_BOOL(d2.IsValid());
      XII_TEST_BOOL(!d.IsComparable());
      XII_TEST_BOOL(!d2.IsComparable());
      XII_TEST_INT(d(0), 666);
      XII_TEST_INT(d2(0), 666);
    }
    XII_TEST_INT(xiiConstructionCounter::s_iDestructions, 3); // value and lambda out of scope
    XII_TEST_INT(xiiConstructionCounter::s_iConstructions, 4);
    d.Invalidate();
    XII_TEST_INT(xiiConstructionCounter::s_iDestructions, 4); // lambda destroyed.
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Lambda - capture non-copyable type")
  {
    xiiUniquePtr<xiiConstructionCounter> data(XII_DEFAULT_NEW(xiiConstructionCounter));
    data->m_iData   = 666;
    TestDelegate d2 = [data = std::move(data)](xiiInt32 i) -> xiiInt32 {
      return data->m_iData;
    };
    XII_TEST_INT(d2(0), 666);
    d = std::move(d2);
    XII_TEST_BOOL(d.IsValid());
    XII_TEST_BOOL(!d2.IsValid());
    XII_TEST_INT(d(0), 666);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiMakeDelegate")
  {
    auto d1 = xiiMakeDelegate(&Function);
    XII_TEST_BOOL(d1.IsEqualIfComparable(xiiMakeDelegate(&Function)));

    TestType instance;
    auto     d2 = xiiMakeDelegate(&TestType::Method, &instance);
    XII_TEST_BOOL(d2.IsEqualIfComparable(xiiMakeDelegate(&TestType::Method, &instance)));
    auto d3 = xiiMakeDelegate(&TestType::ConstMethod, &instance);
    XII_TEST_BOOL(d3.IsEqualIfComparable(xiiMakeDelegate(&TestType::ConstMethod, &instance)));
    auto d4 = xiiMakeDelegate(&TestType::VirtualMethod, &instance);
    XII_TEST_BOOL(d4.IsEqualIfComparable(xiiMakeDelegate(&TestType::VirtualMethod, &instance)));

    TestType instance2;
    auto     d2_2 = xiiMakeDelegate(&TestType::Method, &instance2);
    XII_TEST_BOOL(!d2_2.IsEqualIfComparable(d2));

    XII_IGNORE_UNUSED(d1);
    XII_IGNORE_UNUSED(d2);
    XII_IGNORE_UNUSED(d2_2);
    XII_IGNORE_UNUSED(d3);
    XII_IGNORE_UNUSED(d4);
  }
}
