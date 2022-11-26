#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Types/RefCounted.h>

class RefCountedTestClass : public xiiRefCounted
{
public:
  xiiUInt32 m_uiDummyMember = 0x42u;
};

XII_CREATE_SIMPLE_TEST(Basics, RefCounted)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Ref Counting")
  {
    RefCountedTestClass Instance;

    XII_TEST_BOOL(Instance.GetRefCount() == 0);
    XII_TEST_BOOL(!Instance.IsReferenced());

    Instance.AddRef();

    XII_TEST_BOOL(Instance.GetRefCount() == 1);
    XII_TEST_BOOL(Instance.IsReferenced());

    /// Test scoped ref pointer
    {
      xiiScopedRefPointer<RefCountedTestClass> ScopeTester(&Instance);

      XII_TEST_BOOL(Instance.GetRefCount() == 2);
      XII_TEST_BOOL(Instance.IsReferenced());
    }

    /// Test assignment of scoped ref pointer
    {
      xiiScopedRefPointer<RefCountedTestClass> ScopeTester;

      ScopeTester = &Instance;

      XII_TEST_BOOL(Instance.GetRefCount() == 2);
      XII_TEST_BOOL(Instance.IsReferenced());

      xiiScopedRefPointer<RefCountedTestClass> ScopeTester2;

      ScopeTester2 = ScopeTester;

      XII_TEST_BOOL(Instance.GetRefCount() == 3);
      XII_TEST_BOOL(Instance.IsReferenced());

      xiiScopedRefPointer<RefCountedTestClass> ScopeTester3(ScopeTester);

      XII_TEST_BOOL(Instance.GetRefCount() == 4);
      XII_TEST_BOOL(Instance.IsReferenced());
    }

    /// Test copy constructor for xiiRefCounted
    {
      RefCountedTestClass inst2(Instance);
      RefCountedTestClass inst3;
      inst3 = Instance;

      XII_TEST_BOOL(Instance.GetRefCount() == 1);
      XII_TEST_BOOL(Instance.IsReferenced());

      XII_TEST_BOOL(inst2.GetRefCount() == 0);
      XII_TEST_BOOL(!inst2.IsReferenced());

      XII_TEST_BOOL(inst3.GetRefCount() == 0);
      XII_TEST_BOOL(!inst3.IsReferenced());
    }

    XII_TEST_BOOL(Instance.GetRefCount() == 1);
    XII_TEST_BOOL(Instance.IsReferenced());

    Instance.ReleaseRef();

    XII_TEST_BOOL(Instance.GetRefCount() == 0);
    XII_TEST_BOOL(!Instance.IsReferenced());
  }
}
