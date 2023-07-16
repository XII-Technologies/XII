#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Types/PointerWithFlags.h>

XII_CREATE_SIMPLE_TEST(Basics, PointerWithFlags)
{
  struct Dummy
  {
    float a = 3.0f;
    int   b = 7;
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "General")
  {
    xiiPointerWithFlags<Dummy, 2> ptr;

    XII_TEST_INT(ptr.GetFlags(), 0);
    ptr.SetFlags(3);
    XII_TEST_INT(ptr.GetFlags(), 3);

    XII_TEST_BOOL(ptr == nullptr);
    XII_TEST_BOOL(!ptr);

    XII_TEST_INT(ptr.GetFlags(), 3);
    ptr.SetFlags(2);
    XII_TEST_INT(ptr.GetFlags(), 2);

    Dummy d1, d2;
    ptr  = &d1;
    d2.a = 4;
    d2.b = 8;

    XII_TEST_BOOL(ptr.GetPtr() == &d1);
    XII_TEST_BOOL(ptr.GetPtr() != &d2);

    XII_TEST_INT(ptr.GetFlags(), 2);
    ptr.SetFlags(1);
    XII_TEST_INT(ptr.GetFlags(), 1);

    XII_TEST_BOOL(ptr == &d1);
    XII_TEST_BOOL(ptr != &d2);
    XII_TEST_BOOL(ptr);


    XII_TEST_FLOAT(ptr->a, 3.0f, 0.0f);
    XII_TEST_INT(ptr->b, 7);

    ptr = &d2;

    XII_TEST_INT(ptr.GetFlags(), 1);
    ptr.SetFlags(3);
    XII_TEST_INT(ptr.GetFlags(), 3);

    XII_TEST_BOOL(ptr != &d1);
    XII_TEST_BOOL(ptr == &d2);
    XII_TEST_BOOL(ptr);

    ptr = nullptr;
    XII_TEST_BOOL(!ptr);
    XII_TEST_BOOL(ptr == nullptr);

    XII_TEST_INT(ptr.GetFlags(), 3);
    ptr.SetFlags(0);
    XII_TEST_INT(ptr.GetFlags(), 0);

    xiiPointerWithFlags<Dummy, 2> ptr2 = ptr;
    XII_TEST_BOOL(ptr == ptr2);

    XII_TEST_BOOL(ptr2.GetPtr() == ptr.GetPtr());
    XII_TEST_BOOL(ptr2.GetFlags() == ptr.GetFlags());

    ptr2.SetFlags(3);
    XII_TEST_BOOL(ptr2.GetPtr() == ptr.GetPtr());
    XII_TEST_BOOL(ptr2.GetFlags() != ptr.GetFlags());

    // the two Ptrs still compare equal (pointer part is equal, even if flags are different)
    XII_TEST_BOOL(ptr == ptr2);
  }
}
