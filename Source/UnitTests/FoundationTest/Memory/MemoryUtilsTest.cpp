#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HybridArray.h>

struct xiiConstructTest
{
public:
  static xiiHybridArray<void*, 10> s_dtorList;

  xiiConstructTest() { m_iData = 42; }

  ~xiiConstructTest() { s_dtorList.PushBack(this); }

  xiiInt32 m_iData;
};
xiiHybridArray<void*, 10> xiiConstructTest::s_dtorList;

XII_CHECK_AT_COMPILETIME(sizeof(xiiConstructTest) == 4);


struct PODTest
{
  XII_DECLARE_POD_TYPE();

  PODTest() { m_iData = -1; }

  xiiInt32 m_iData;
};

static const xiiUInt32 s_uiSize = sizeof(xiiConstructTest);

XII_CREATE_SIMPLE_TEST(Memory, MemoryUtils)
{
  xiiConstructTest::s_dtorList.Clear();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Construct")
  {
    xiiUInt8          uiRawData[s_uiSize * 5] = {0};
    xiiConstructTest* pTest                   = (xiiConstructTest*)(uiRawData);

    xiiMemoryUtils::Construct<xiiConstructTest>(pTest + 1, 2);

    XII_TEST_INT(pTest[0].m_iData, 0);
    XII_TEST_INT(pTest[1].m_iData, 42);
    XII_TEST_INT(pTest[2].m_iData, 42);
    XII_TEST_INT(pTest[3].m_iData, 0);
    XII_TEST_INT(pTest[4].m_iData, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeConstructorFunction")
  {
    xiiMemoryUtils::ConstructorFunction func = xiiMemoryUtils::MakeConstructorFunction<xiiConstructTest>();
    XII_TEST_BOOL(func != nullptr);

    xiiUInt8          uiRawData[s_uiSize] = {0};
    xiiConstructTest* pTest               = (xiiConstructTest*)(uiRawData);

    (*func)(pTest);

    XII_TEST_INT(pTest->m_iData, 42);

    func = xiiMemoryUtils::MakeConstructorFunction<PODTest>();
    XII_TEST_BOOL(func != nullptr);

    func = xiiMemoryUtils::MakeConstructorFunction<xiiInt32>();
    XII_TEST_BOOL(func == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "DefaultConstruct")
  {
    xiiUInt32 uiRawData[5]; // not initialized here

    xiiMemoryUtils::DefaultConstruct(uiRawData + 1, 2);

    XII_TEST_INT(uiRawData[1], 0);
    XII_TEST_INT(uiRawData[2], 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeDefaultConstructorFunction")
  {
    xiiMemoryUtils::ConstructorFunction func = xiiMemoryUtils::MakeDefaultConstructorFunction<xiiInt32>();
    XII_TEST_BOOL(func != nullptr);

    xiiInt32 iTest = 2;

    (*func)(&iTest);

    XII_TEST_INT(iTest, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Construct Copy(Array)")
  {
    xiiUInt8          uiRawData[s_uiSize * 5] = {0};
    xiiConstructTest* pTest                   = (xiiConstructTest*)(uiRawData);

    xiiConstructTest copy[2];
    copy[0].m_iData = 43;
    copy[1].m_iData = 44;

    xiiMemoryUtils::CopyConstructArray<xiiConstructTest>(pTest + 1, copy, 2);

    XII_TEST_INT(pTest[0].m_iData, 0);
    XII_TEST_INT(pTest[1].m_iData, 43);
    XII_TEST_INT(pTest[2].m_iData, 44);
    XII_TEST_INT(pTest[3].m_iData, 0);
    XII_TEST_INT(pTest[4].m_iData, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Construct Copy(Element)")
  {
    xiiUInt8          uiRawData[s_uiSize * 5] = {0};
    xiiConstructTest* pTest                   = (xiiConstructTest*)(uiRawData);

    xiiConstructTest copy;
    copy.m_iData = 43;

    xiiMemoryUtils::CopyConstruct<xiiConstructTest>(pTest + 1, copy, 2);

    XII_TEST_INT(pTest[0].m_iData, 0);
    XII_TEST_INT(pTest[1].m_iData, 43);
    XII_TEST_INT(pTest[2].m_iData, 43);
    XII_TEST_INT(pTest[3].m_iData, 0);
    XII_TEST_INT(pTest[4].m_iData, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeCopyConstructorFunction")
  {
    xiiMemoryUtils::CopyConstructorFunction func = xiiMemoryUtils::MakeCopyConstructorFunction<xiiConstructTest>();
    XII_TEST_BOOL(func != nullptr);

    xiiUInt8          uiRawData[s_uiSize] = {0};
    xiiConstructTest* pTest               = (xiiConstructTest*)(uiRawData);

    xiiConstructTest copy;
    copy.m_iData = 43;

    (*func)(pTest, &copy);

    XII_TEST_INT(pTest->m_iData, 43);

    func = xiiMemoryUtils::MakeCopyConstructorFunction<PODTest>();
    XII_TEST_BOOL(func != nullptr);

    func = xiiMemoryUtils::MakeCopyConstructorFunction<xiiInt32>();
    XII_TEST_BOOL(func != nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Destruct")
  {
    xiiUInt8          uiRawData[s_uiSize * 5] = {0};
    xiiConstructTest* pTest                   = (xiiConstructTest*)(uiRawData);

    xiiMemoryUtils::Construct<xiiConstructTest>(pTest + 1, 2);

    XII_TEST_INT(pTest[0].m_iData, 0);
    XII_TEST_INT(pTest[1].m_iData, 42);
    XII_TEST_INT(pTest[2].m_iData, 42);
    XII_TEST_INT(pTest[3].m_iData, 0);
    XII_TEST_INT(pTest[4].m_iData, 0);

    xiiConstructTest::s_dtorList.Clear();
    xiiMemoryUtils::Destruct<xiiConstructTest>(pTest, 4);
    XII_TEST_INT(4, xiiConstructTest::s_dtorList.GetCount());

    if (xiiConstructTest::s_dtorList.GetCount() == 4)
    {
      XII_TEST_BOOL(xiiConstructTest::s_dtorList[0] == &pTest[3]);
      XII_TEST_BOOL(xiiConstructTest::s_dtorList[1] == &pTest[2]);
      XII_TEST_BOOL(xiiConstructTest::s_dtorList[2] == &pTest[1]);
      XII_TEST_BOOL(xiiConstructTest::s_dtorList[3] == &pTest[0]);
      XII_TEST_INT(pTest[4].m_iData, 0);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeDestructorFunction")
  {
    xiiMemoryUtils::DestructorFunction func = xiiMemoryUtils::MakeDestructorFunction<xiiConstructTest>();
    XII_TEST_BOOL(func != nullptr);

    xiiUInt8          uiRawData[s_uiSize] = {0};
    xiiConstructTest* pTest               = (xiiConstructTest*)(uiRawData);

    xiiMemoryUtils::Construct(pTest, 1);
    XII_TEST_INT(pTest->m_iData, 42);

    xiiConstructTest::s_dtorList.Clear();
    (*func)(pTest);
    XII_TEST_INT(1, xiiConstructTest::s_dtorList.GetCount());

    if (xiiConstructTest::s_dtorList.GetCount() == 1)
    {
      XII_TEST_BOOL(xiiConstructTest::s_dtorList[0] == pTest);
    }

    func = xiiMemoryUtils::MakeDestructorFunction<PODTest>();
    XII_TEST_BOOL(func == nullptr);

    func = xiiMemoryUtils::MakeDestructorFunction<xiiInt32>();
    XII_TEST_BOOL(func == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Copy")
  {
    xiiUInt8 uiRawData[5]  = {1, 2, 3, 4, 5};
    xiiUInt8 uiRawData2[5] = {6, 7, 8, 9, 0};

    XII_TEST_INT(uiRawData[0], 1);
    XII_TEST_INT(uiRawData[1], 2);
    XII_TEST_INT(uiRawData[2], 3);
    XII_TEST_INT(uiRawData[3], 4);
    XII_TEST_INT(uiRawData[4], 5);

    xiiMemoryUtils::Copy(uiRawData + 1, uiRawData2 + 2, 3);

    XII_TEST_INT(uiRawData[0], 1);
    XII_TEST_INT(uiRawData[1], 8);
    XII_TEST_INT(uiRawData[2], 9);
    XII_TEST_INT(uiRawData[3], 0);
    XII_TEST_INT(uiRawData[4], 5);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move")
  {
    xiiUInt8 uiRawData[5] = {1, 2, 3, 4, 5};

    XII_TEST_INT(uiRawData[0], 1);
    XII_TEST_INT(uiRawData[1], 2);
    XII_TEST_INT(uiRawData[2], 3);
    XII_TEST_INT(uiRawData[3], 4);
    XII_TEST_INT(uiRawData[4], 5);

    xiiMemoryUtils::CopyOverlapped(uiRawData + 1, uiRawData + 3, 2);

    XII_TEST_INT(uiRawData[0], 1);
    XII_TEST_INT(uiRawData[1], 4);
    XII_TEST_INT(uiRawData[2], 5);
    XII_TEST_INT(uiRawData[3], 4);
    XII_TEST_INT(uiRawData[4], 5);

    xiiMemoryUtils::CopyOverlapped(uiRawData + 1, uiRawData, 4);

    XII_TEST_INT(uiRawData[0], 1);
    XII_TEST_INT(uiRawData[1], 1);
    XII_TEST_INT(uiRawData[2], 4);
    XII_TEST_INT(uiRawData[3], 5);
    XII_TEST_INT(uiRawData[4], 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    xiiUInt8 uiRawData1[5] = {1, 2, 3, 4, 5};
    xiiUInt8 uiRawData2[5] = {1, 2, 3, 4, 5};
    xiiUInt8 uiRawData3[5] = {1, 2, 3, 4, 6};

    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(uiRawData1, uiRawData2, 5));
    XII_TEST_BOOL(!xiiMemoryUtils::IsEqual(uiRawData1, uiRawData3, 5));
    XII_TEST_BOOL(xiiMemoryUtils::IsEqual(uiRawData1, uiRawData3, 4));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ZeroFill")
  {
    xiiUInt8 uiRawData[5] = {1, 2, 3, 4, 5};

    XII_TEST_INT(uiRawData[0], 1);
    XII_TEST_INT(uiRawData[1], 2);
    XII_TEST_INT(uiRawData[2], 3);
    XII_TEST_INT(uiRawData[3], 4);
    XII_TEST_INT(uiRawData[4], 5);

    // T*, size_t N overload
    xiiMemoryUtils::ZeroFill(uiRawData + 1, 3);

    XII_TEST_INT(uiRawData[0], 1);
    XII_TEST_INT(uiRawData[1], 0);
    XII_TEST_INT(uiRawData[2], 0);
    XII_TEST_INT(uiRawData[3], 0);
    XII_TEST_INT(uiRawData[4], 5);

    // T[N] overload
    xiiMemoryUtils::ZeroFill(uiRawData);

    XII_TEST_INT(uiRawData[0], 0);
    XII_TEST_INT(uiRawData[1], 0);
    XII_TEST_INT(uiRawData[2], 0);
    XII_TEST_INT(uiRawData[3], 0);
    XII_TEST_INT(uiRawData[4], 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PatternFill")
  {
    xiiUInt8 uiRawData[5] = {1, 2, 3, 4, 5};

    XII_TEST_INT(uiRawData[0], 1);
    XII_TEST_INT(uiRawData[1], 2);
    XII_TEST_INT(uiRawData[2], 3);
    XII_TEST_INT(uiRawData[3], 4);
    XII_TEST_INT(uiRawData[4], 5);

    // T*, size_t N overload
    xiiMemoryUtils::PatternFill(uiRawData + 1, 0xAB, 3);

    XII_TEST_INT(uiRawData[0], 1);
    XII_TEST_INT(uiRawData[1], 0xAB);
    XII_TEST_INT(uiRawData[2], 0xAB);
    XII_TEST_INT(uiRawData[3], 0xAB);
    XII_TEST_INT(uiRawData[4], 5);

    // T[N] overload
    xiiMemoryUtils::PatternFill(uiRawData, 0xCD);

    XII_TEST_INT(uiRawData[0], 0xCD);
    XII_TEST_INT(uiRawData[1], 0xCD);
    XII_TEST_INT(uiRawData[2], 0xCD);
    XII_TEST_INT(uiRawData[3], 0xCD);
    XII_TEST_INT(uiRawData[4], 0xCD);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compare")
  {
    xiiUInt32 uiRawDataA[3] = {1, 2, 3};
    xiiUInt32 uiRawDataB[3] = {3, 4, 5};

    XII_TEST_INT(uiRawDataA[0], 1);
    XII_TEST_INT(uiRawDataA[1], 2);
    XII_TEST_INT(uiRawDataA[2], 3);
    XII_TEST_INT(uiRawDataB[0], 3);
    XII_TEST_INT(uiRawDataB[1], 4);
    XII_TEST_INT(uiRawDataB[2], 5);

    XII_TEST_BOOL(xiiMemoryUtils::Compare(uiRawDataA, uiRawDataB, 3) < 0);
    XII_TEST_BOOL(xiiMemoryUtils::Compare(uiRawDataA + 2, uiRawDataB, 1) == 0);
    XII_TEST_BOOL(xiiMemoryUtils::Compare(uiRawDataB, uiRawDataA, 3) > 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "AddByteOffset")
  {
    xiiInt32* pData1 = nullptr;
    pData1           = xiiMemoryUtils::AddByteOffset(pData1, 13);
    XII_TEST_BOOL(pData1 == reinterpret_cast<xiiInt32*>(13));

    const xiiInt32* pData2 = nullptr;
    const xiiInt32* pData3 = xiiMemoryUtils::AddByteOffset(pData2, 17);
    XII_TEST_BOOL(pData3 == reinterpret_cast<xiiInt32*>(17));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Align / IsAligned")
  {
    {
      xiiInt32* pData = (xiiInt32*)1;
      XII_TEST_BOOL(!xiiMemoryUtils::IsAligned(pData, 4));
      pData = xiiMemoryUtils::AlignBackwards(pData, 4);
      XII_TEST_BOOL(pData == reinterpret_cast<xiiInt32*>(0));
      XII_TEST_BOOL(xiiMemoryUtils::IsAligned(pData, 4));
    }
    {
      xiiInt32* pData = (xiiInt32*)2;
      XII_TEST_BOOL(!xiiMemoryUtils::IsAligned(pData, 4));
      pData = xiiMemoryUtils::AlignBackwards(pData, 4);
      XII_TEST_BOOL(pData == reinterpret_cast<xiiInt32*>(0));
      XII_TEST_BOOL(xiiMemoryUtils::IsAligned(pData, 4));
    }
    {
      xiiInt32* pData = (xiiInt32*)3;
      XII_TEST_BOOL(!xiiMemoryUtils::IsAligned(pData, 4));
      pData = xiiMemoryUtils::AlignBackwards(pData, 4);
      XII_TEST_BOOL(pData == reinterpret_cast<xiiInt32*>(0));
      XII_TEST_BOOL(xiiMemoryUtils::IsAligned(pData, 4));
    }
    {
      xiiInt32* pData = (xiiInt32*)4;
      XII_TEST_BOOL(xiiMemoryUtils::IsAligned(pData, 4));
      pData = xiiMemoryUtils::AlignBackwards(pData, 4);
      XII_TEST_BOOL(pData == reinterpret_cast<xiiInt32*>(4));
      XII_TEST_BOOL(xiiMemoryUtils::IsAligned(pData, 4));
    }

    {
      xiiInt32* pData = (xiiInt32*)1;
      XII_TEST_BOOL(!xiiMemoryUtils::IsAligned(pData, 4));
      pData = xiiMemoryUtils::AlignForwards(pData, 4);
      XII_TEST_BOOL(pData == reinterpret_cast<xiiInt32*>(4));
      XII_TEST_BOOL(xiiMemoryUtils::IsAligned(pData, 4));
    }
    {
      xiiInt32* pData = (xiiInt32*)2;
      XII_TEST_BOOL(!xiiMemoryUtils::IsAligned(pData, 4));
      pData = xiiMemoryUtils::AlignForwards(pData, 4);
      XII_TEST_BOOL(pData == reinterpret_cast<xiiInt32*>(4));
      XII_TEST_BOOL(xiiMemoryUtils::IsAligned(pData, 4));
    }
    {
      xiiInt32* pData = (xiiInt32*)3;
      XII_TEST_BOOL(!xiiMemoryUtils::IsAligned(pData, 4));
      pData = xiiMemoryUtils::AlignForwards(pData, 4);
      XII_TEST_BOOL(pData == reinterpret_cast<xiiInt32*>(4));
      XII_TEST_BOOL(xiiMemoryUtils::IsAligned(pData, 4));
    }
    {
      xiiInt32* pData = (xiiInt32*)4;
      XII_TEST_BOOL(xiiMemoryUtils::IsAligned(pData, 4));
      pData = xiiMemoryUtils::AlignForwards(pData, 4);
      XII_TEST_BOOL(pData == reinterpret_cast<xiiInt32*>(4));
      XII_TEST_BOOL(xiiMemoryUtils::IsAligned(pData, 4));
    }
  }
}
