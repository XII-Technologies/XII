#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>

template <typename T>
static void testArrayPtr(xiiArrayPtr<T> arrayPtr, typename xiiArrayPtr<T>::PointerType extectedPtr, xiiUInt32 uiExpectedCount)
{
  XII_TEST_BOOL(arrayPtr.GetPtr() == extectedPtr);
  XII_TEST_INT(arrayPtr.GetCount(), uiExpectedCount);
}

// static void TakeConstArrayPtr(xiiArrayPtr<const int> cint)
//{
//}
//
// static void TakeConstArrayPtr2(xiiArrayPtr<const int*> cint, xiiArrayPtr<const int* const> cintc)
//{
//}

XII_CREATE_SIMPLE_TEST(Basics, ArrayPtr)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Empty Constructor")
  {
    xiiArrayPtr<xiiInt32> Empty;

    XII_TEST_BOOL(Empty.GetPtr() == nullptr);
    XII_TEST_BOOL(Empty.GetCount() == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor")
  {
    xiiInt32 pIntData[] = {1, 2, 3, 4, 5};

    xiiArrayPtr<xiiInt32> ap(pIntData, 3);
    XII_TEST_BOOL(ap.GetPtr() == pIntData);
    XII_TEST_BOOL(ap.GetCount() == 3);

    xiiArrayPtr<xiiInt32> ap2(pIntData, 0u);
    XII_TEST_BOOL(ap2.GetPtr() == nullptr);
    XII_TEST_BOOL(ap2.GetCount() == 0);

    xiiArrayPtr<xiiInt32> ap3(pIntData);
    XII_TEST_BOOL(ap3.GetPtr() == pIntData);
    XII_TEST_BOOL(ap3.GetCount() == 5);

    xiiArrayPtr<xiiInt32> ap4(ap);
    XII_TEST_BOOL(ap4.GetPtr() == pIntData);
    XII_TEST_BOOL(ap4.GetCount() == 3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiMakeArrayPtr")
  {
    xiiInt32 pIntData[] = {1, 2, 3, 4, 5};

    testArrayPtr(xiiMakeArrayPtr(pIntData, 3), pIntData, 3);
    testArrayPtr(xiiMakeArrayPtr(pIntData, 0), nullptr, 0);
    testArrayPtr(xiiMakeArrayPtr(pIntData), pIntData, 5);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator=")
  {
    xiiInt32 pIntData[] = {1, 2, 3, 4, 5};

    xiiArrayPtr<xiiInt32> ap(pIntData, 3);
    XII_TEST_BOOL(ap.GetPtr() == pIntData);
    XII_TEST_BOOL(ap.GetCount() == 3);

    xiiArrayPtr<xiiInt32> ap2;
    ap2 = ap;

    XII_TEST_BOOL(ap2.GetPtr() == pIntData);
    XII_TEST_BOOL(ap2.GetCount() == 3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear")
  {
    xiiInt32 pIntData[] = {1, 2, 3, 4, 5};

    xiiArrayPtr<xiiInt32> ap(pIntData, 3);
    XII_TEST_BOOL(ap.GetPtr() == pIntData);
    XII_TEST_BOOL(ap.GetCount() == 3);

    ap.Clear();

    XII_TEST_BOOL(ap.GetPtr() == nullptr);
    XII_TEST_BOOL(ap.GetCount() == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator== / operator!= / operator<")
  {
    xiiInt32 pIntData[] = {1, 2, 3, 4, 5};

    xiiArrayPtr<xiiInt32> ap1(pIntData, 3);
    xiiArrayPtr<xiiInt32> ap2(pIntData, 3);
    xiiArrayPtr<xiiInt32> ap3(pIntData, 4);
    xiiArrayPtr<xiiInt32> ap4(pIntData + 1, 3);

    XII_TEST_BOOL(ap1 == ap2);
    XII_TEST_BOOL(ap1 != ap3);
    XII_TEST_BOOL(ap1 != ap4);

    XII_TEST_BOOL(ap1 < ap3);
    xiiInt32              pIntData2[] = {1, 2, 4};
    xiiArrayPtr<xiiInt32> ap5(pIntData2, 3);
    XII_TEST_BOOL(ap1 < ap5);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator[]")
  {
    xiiInt32 pIntData[] = {1, 2, 3, 4, 5};

    xiiArrayPtr<xiiInt32> ap(pIntData + 1, 3);
    XII_TEST_INT(ap[0], 2);
    XII_TEST_INT(ap[1], 3);
    XII_TEST_INT(ap[2], 4);
    ap[2] = 10;
    XII_TEST_INT(ap[2], 10);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "const operator[]")
  {
    xiiInt32 pIntData[] = {1, 2, 3, 4, 5};

    const xiiArrayPtr<xiiInt32> ap(pIntData + 1, 3);
    XII_TEST_INT(ap[0], 2);
    XII_TEST_INT(ap[1], 3);
    XII_TEST_INT(ap[2], 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CopyFrom")
  {
    xiiInt32 pIntData1[] = {1, 2, 3, 4, 5};
    xiiInt32 pIntData2[] = {6, 7, 8, 9, 0};

    xiiArrayPtr<xiiInt32> ap1(pIntData1 + 1, 3);
    xiiArrayPtr<xiiInt32> ap2(pIntData2 + 2, 3);

    ap1.CopyFrom(ap2);

    XII_TEST_INT(pIntData1[0], 1);
    XII_TEST_INT(pIntData1[1], 8);
    XII_TEST_INT(pIntData1[2], 9);
    XII_TEST_INT(pIntData1[3], 0);
    XII_TEST_INT(pIntData1[4], 5);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetSubArray")
  {
    xiiInt32 pIntData1[] = {1, 2, 3, 4, 5};

    xiiArrayPtr<xiiInt32> ap1(pIntData1, 5);
    xiiArrayPtr<xiiInt32> ap2 = ap1.GetSubArray(2, 3);

    XII_TEST_BOOL(ap2.GetPtr() == &pIntData1[2]);
    XII_TEST_BOOL(ap2.GetCount() == 3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Const Conversions")
  {
    xiiInt32                    pIntData1[] = {1, 2, 3, 4, 5};
    xiiArrayPtr<xiiInt32>       ap1(pIntData1);
    xiiArrayPtr<const xiiInt32> ap2(ap1);
    xiiArrayPtr<const xiiInt32> ap3(pIntData1);
    ap2 = ap1; // non const to const assign
    ap3 = ap2; // const to const assign
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Empty Constructor (const)")
  {
    xiiArrayPtr<const xiiInt32> Empty;

    XII_TEST_BOOL(Empty.GetPtr() == nullptr);
    XII_TEST_BOOL(Empty.GetCount() == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (const)")
  {
    const xiiInt32 pIntData[] = {1, 2, 3, 4, 5};

    xiiArrayPtr<const xiiInt32> ap(pIntData, 3);
    XII_TEST_BOOL(ap.GetPtr() == pIntData);
    XII_TEST_BOOL(ap.GetCount() == 3);

    xiiArrayPtr<const xiiInt32> ap2(pIntData, 0u);
    XII_TEST_BOOL(ap2.GetPtr() == nullptr);
    XII_TEST_BOOL(ap2.GetCount() == 0);

    xiiArrayPtr<const xiiInt32> ap3(pIntData);
    XII_TEST_BOOL(ap3.GetPtr() == pIntData);
    XII_TEST_BOOL(ap3.GetCount() == 5);

    xiiArrayPtr<const xiiInt32> ap4(ap);
    XII_TEST_BOOL(ap4.GetPtr() == pIntData);
    XII_TEST_BOOL(ap4.GetCount() == 3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator=  (const)")
  {
    const xiiInt32 pIntData[] = {1, 2, 3, 4, 5};

    xiiArrayPtr<const xiiInt32> ap(pIntData, 3);
    XII_TEST_BOOL(ap.GetPtr() == pIntData);
    XII_TEST_BOOL(ap.GetCount() == 3);

    xiiArrayPtr<const xiiInt32> ap2;
    ap2 = ap;

    XII_TEST_BOOL(ap2.GetPtr() == pIntData);
    XII_TEST_BOOL(ap2.GetCount() == 3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear (const)")
  {
    const xiiInt32 pIntData[] = {1, 2, 3, 4, 5};

    xiiArrayPtr<const xiiInt32> ap(pIntData, 3);
    XII_TEST_BOOL(ap.GetPtr() == pIntData);
    XII_TEST_BOOL(ap.GetCount() == 3);

    ap.Clear();

    XII_TEST_BOOL(ap.GetPtr() == nullptr);
    XII_TEST_BOOL(ap.GetCount() == 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator== / operator!=  (const)")
  {
    xiiInt32 pIntData[] = {1, 2, 3, 4, 5};

    xiiArrayPtr<xiiInt32>       ap1(pIntData, 3);
    xiiArrayPtr<const xiiInt32> ap2(pIntData, 3);
    xiiArrayPtr<const xiiInt32> ap3(pIntData, 4);
    xiiArrayPtr<const xiiInt32> ap4(pIntData + 1, 3);

    XII_TEST_BOOL(ap1 == ap2);
    XII_TEST_BOOL(ap3 != ap1);
    XII_TEST_BOOL(ap1 != ap4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator[]  (const)")
  {
    const xiiInt32 pIntData[] = {1, 2, 3, 4, 5};

    xiiArrayPtr<const xiiInt32> ap(pIntData + 1, 3);
    XII_TEST_INT(ap[0], 2);
    XII_TEST_INT(ap[1], 3);
    XII_TEST_INT(ap[2], 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "const operator[] (const)")
  {
    const xiiInt32 pIntData[] = {1, 2, 3, 4, 5};

    const xiiArrayPtr<const xiiInt32> ap(pIntData + 1, 3);
    XII_TEST_INT(ap[0], 2);
    XII_TEST_INT(ap[1], 3);
    XII_TEST_INT(ap[2], 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetSubArray (const)")
  {
    const xiiInt32 pIntData1[] = {1, 2, 3, 4, 5};

    xiiArrayPtr<const xiiInt32> ap1(pIntData1, 5);
    xiiArrayPtr<const xiiInt32> ap2 = ap1.GetSubArray(2, 3);

    XII_TEST_BOOL(ap2.GetPtr() == &pIntData1[2]);
    XII_TEST_BOOL(ap2.GetCount() == 3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "STL Iterator")
  {
    xiiDynamicArray<xiiInt32> a1;

    for (xiiInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    xiiArrayPtr<xiiInt32> ptr1 = a1;

    // STL sort
    std::sort(begin(ptr1), end(ptr1));

    for (xiiInt32 i = 1; i < 1000; ++i)
    {
      XII_TEST_BOOL(ptr1[i - 1] <= ptr1[i]);
    }

    // foreach
    xiiUInt32 prev = 0;
    for (xiiUInt32 val : ptr1)
    {
      XII_TEST_BOOL(prev <= val);
      prev = val;
    }

    // const array
    const xiiDynamicArray<xiiInt32>& a2 = a1;

    const xiiArrayPtr<const xiiInt32> ptr2 = a2;

    // STL lower bound
    auto lb = std::lower_bound(begin(ptr2), end(ptr2), 400);
    XII_TEST_BOOL(*lb == ptr2[400]);
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "STL Reverse Iterator")
  {
    xiiDynamicArray<xiiInt32> a1;

    for (xiiInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    xiiArrayPtr<xiiInt32> ptr1 = a1;

    // STL sort
    std::sort(rbegin(ptr1), rend(ptr1));

    for (xiiInt32 i = 1; i < 1000; ++i)
    {
      XII_TEST_BOOL(ptr1[i - 1] >= ptr1[i]);
    }

    // foreach
    xiiUInt32 prev = 1000;
    for (xiiUInt32 val : ptr1)
    {
      XII_TEST_BOOL(prev >= val);
      prev = val;
    }

    // const array
    const xiiDynamicArray<xiiInt32>& a2 = a1;

    const xiiArrayPtr<const xiiInt32> ptr2 = a2;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(ptr2), rend(ptr2), 400);
    XII_TEST_BOOL(*lb == ptr2[1000 - 400 - 1]);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    xiiDynamicArray<xiiInt32> a0;
    xiiArrayPtr<xiiInt32>     a1 = a0;

    for (xiiInt32 i = -100; i < 100; ++i)
      XII_TEST_BOOL(!a1.Contains(i));

    for (xiiInt32 i = 0; i < 100; ++i)
      a0.PushBack(i);
    for (xiiInt32 i = 0; i < 100; ++i)
      a0.PushBack(i);

    a1 = a0;

    for (xiiInt32 i = 0; i < 100; ++i)
    {
      XII_TEST_BOOL(a1.Contains(i));
      XII_TEST_INT(a1.IndexOf(i), i);
      XII_TEST_INT(a1.IndexOf(i, 100), i + 100);
      XII_TEST_INT(a1.LastIndexOf(i), i + 100);
      XII_TEST_INT(a1.LastIndexOf(i, 100), i);
    }
  }

  // "Implicit Conversions"
  //{
  //  {
  //    xiiHybridArray<int, 4> data;
  //    TakeConstArrayPtr(data);
  //    TakeConstArrayPtr(data.GetArrayPtr());
  //  }
  //  {
  //    xiiHybridArray<int*, 4> data;
  //    //TakeConstArrayPtr2(data, data); // does not compile
  //    TakeConstArrayPtr2(data.GetArrayPtr(), data.GetArrayPtr());
  //  }
  //}
}
