/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Implementation/AllClasses_inl.h>
#include <Foundation/Math/Mat4.h>

XII_CREATE_SIMPLE_TEST(Math, Mat4)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Default Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (xiiMath::SupportsNaN<xiiMat3T::ComponentType>())
    {
      // In debug the default constructor initializes everything with NaN.
      xiiMat4T m;
      XII_TEST_BOOL(xiiMath::IsNaN(m.m_fElementsCM[0]) && xiiMath::IsNaN(m.m_fElementsCM[1]) && xiiMath::IsNaN(m.m_fElementsCM[2]) &&
                    xiiMath::IsNaN(m.m_fElementsCM[3]) && xiiMath::IsNaN(m.m_fElementsCM[4]) && xiiMath::IsNaN(m.m_fElementsCM[5]) &&
                    xiiMath::IsNaN(m.m_fElementsCM[6]) && xiiMath::IsNaN(m.m_fElementsCM[7]) && xiiMath::IsNaN(m.m_fElementsCM[8]) &&
                    xiiMath::IsNaN(m.m_fElementsCM[9]) && xiiMath::IsNaN(m.m_fElementsCM[10]) && xiiMath::IsNaN(m.m_fElementsCM[11]) &&
                    xiiMath::IsNaN(m.m_fElementsCM[12]) && xiiMath::IsNaN(m.m_fElementsCM[13]) && xiiMath::IsNaN(m.m_fElementsCM[14]) &&
                    xiiMath::IsNaN(m.m_fElementsCM[15]));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    xiiMat4T::ComponentType testBlock[16] = {(xiiMat4T::ComponentType)1, (xiiMat4T::ComponentType)2, (xiiMat4T::ComponentType)3,
                                             (xiiMat4T::ComponentType)4, (xiiMat4T::ComponentType)5, (xiiMat4T::ComponentType)6, (xiiMat4T::ComponentType)7, (xiiMat4T::ComponentType)8,
                                             (xiiMat4T::ComponentType)9, (xiiMat4T::ComponentType)10, (xiiMat4T::ComponentType)11, (xiiMat4T::ComponentType)12, (xiiMat4T::ComponentType)13,
                                             (xiiMat4T::ComponentType)14, (xiiMat4T::ComponentType)15, (xiiMat4T::ComponentType)16};
    xiiMat4T*               m             = ::new ((void*)&testBlock[0]) xiiMat4T;

    XII_TEST_BOOL(m->m_fElementsCM[0] == 1.0f && m->m_fElementsCM[1] == 2.0f && m->m_fElementsCM[2] == 3.0f && m->m_fElementsCM[3] == 4.0f &&
                  m->m_fElementsCM[4] == 5.0f && m->m_fElementsCM[5] == 6.0f && m->m_fElementsCM[6] == 7.0f && m->m_fElementsCM[7] == 8.0f &&
                  m->m_fElementsCM[8] == 9.0f && m->m_fElementsCM[9] == 10.0f && m->m_fElementsCM[10] == 11.0f && m->m_fElementsCM[11] == 12.0f &&
                  m->m_fElementsCM[12] == 13.0f && m->m_fElementsCM[13] == 14.0f && m->m_fElementsCM[14] == 15.0f && m->m_fElementsCM[15] == 16.0f);
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (Array Data)")
  {
    const xiiMathTestType data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      xiiMat4T m = xiiMat4T::MakeFromColumnMajorArray(data);

      XII_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 2.0f && m.m_fElementsCM[2] == 3.0f && m.m_fElementsCM[3] == 4.0f &&
                    m.m_fElementsCM[4] == 5.0f && m.m_fElementsCM[5] == 6.0f && m.m_fElementsCM[6] == 7.0f && m.m_fElementsCM[7] == 8.0f &&
                    m.m_fElementsCM[8] == 9.0f && m.m_fElementsCM[9] == 10.0f && m.m_fElementsCM[10] == 11.0f && m.m_fElementsCM[11] == 12.0f &&
                    m.m_fElementsCM[12] == 13.0f && m.m_fElementsCM[13] == 14.0f && m.m_fElementsCM[14] == 15.0f && m.m_fElementsCM[15] == 16.0f);
    }

    {
      xiiMat4T m = xiiMat4T::MakeFromRowMajorArray(data);

      XII_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 5.0f && m.m_fElementsCM[2] == 9.0f && m.m_fElementsCM[3] == 13.0f &&
                    m.m_fElementsCM[4] == 2.0f && m.m_fElementsCM[5] == 6.0f && m.m_fElementsCM[6] == 10.0f && m.m_fElementsCM[7] == 14.0f &&
                    m.m_fElementsCM[8] == 3.0f && m.m_fElementsCM[9] == 7.0f && m.m_fElementsCM[10] == 11.0f && m.m_fElementsCM[11] == 15.0f &&
                    m.m_fElementsCM[12] == 4.0f && m.m_fElementsCM[13] == 8.0f && m.m_fElementsCM[14] == 12.0f && m.m_fElementsCM[15] == 16.0f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (Elements)")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 0), 2, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 0), 3, 0.00001f);
    XII_TEST_FLOAT(m.Element(3, 0), 4, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 1), 5, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 1), 6, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 1), 7, 0.00001f);
    XII_TEST_FLOAT(m.Element(3, 1), 8, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 2), 9, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 2), 10, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 2), 11, 0.00001f);
    XII_TEST_FLOAT(m.Element(3, 2), 12, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 3), 13, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 3), 14, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 3), 15, 0.00001f);
    XII_TEST_FLOAT(m.Element(3, 3), 16, 0.00001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (composite)")
  {
    xiiMat3T mr = xiiMat3T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);
    xiiVec3T vt(10, 11, 12);

    xiiMat4T m(mr, vt);

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0);
    XII_TEST_FLOAT(m.Element(1, 0), 2, 0);
    XII_TEST_FLOAT(m.Element(2, 0), 3, 0);
    XII_TEST_FLOAT(m.Element(3, 0), 10, 0);
    XII_TEST_FLOAT(m.Element(0, 1), 4, 0);
    XII_TEST_FLOAT(m.Element(1, 1), 5, 0);
    XII_TEST_FLOAT(m.Element(2, 1), 6, 0);
    XII_TEST_FLOAT(m.Element(3, 1), 11, 0);
    XII_TEST_FLOAT(m.Element(0, 2), 7, 0);
    XII_TEST_FLOAT(m.Element(1, 2), 8, 0);
    XII_TEST_FLOAT(m.Element(2, 2), 9, 0);
    XII_TEST_FLOAT(m.Element(3, 2), 12, 0);
    XII_TEST_FLOAT(m.Element(0, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromArray")
  {
    const xiiMathTestType data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      xiiMat4T m = xiiMat4T::MakeFromColumnMajorArray(data);

      XII_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 2.0f && m.m_fElementsCM[2] == 3.0f && m.m_fElementsCM[3] == 4.0f &&
                    m.m_fElementsCM[4] == 5.0f && m.m_fElementsCM[5] == 6.0f && m.m_fElementsCM[6] == 7.0f && m.m_fElementsCM[7] == 8.0f &&
                    m.m_fElementsCM[8] == 9.0f && m.m_fElementsCM[9] == 10.0f && m.m_fElementsCM[10] == 11.0f && m.m_fElementsCM[11] == 12.0f &&
                    m.m_fElementsCM[12] == 13.0f && m.m_fElementsCM[13] == 14.0f && m.m_fElementsCM[14] == 15.0f && m.m_fElementsCM[15] == 16.0f);
    }

    {
      xiiMat4T m = xiiMat4T::MakeFromRowMajorArray(data);

      XII_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 5.0f && m.m_fElementsCM[2] == 9.0f && m.m_fElementsCM[3] == 13.0f &&
                    m.m_fElementsCM[4] == 2.0f && m.m_fElementsCM[5] == 6.0f && m.m_fElementsCM[6] == 10.0f && m.m_fElementsCM[7] == 14.0f &&
                    m.m_fElementsCM[8] == 3.0f && m.m_fElementsCM[9] == 7.0f && m.m_fElementsCM[10] == 11.0f && m.m_fElementsCM[11] == 15.0f &&
                    m.m_fElementsCM[12] == 4.0f && m.m_fElementsCM[13] == 8.0f && m.m_fElementsCM[14] == 12.0f && m.m_fElementsCM[15] == 16.0f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetElements")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 0), 2, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 0), 3, 0.00001f);
    XII_TEST_FLOAT(m.Element(3, 0), 4, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 1), 5, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 1), 6, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 1), 7, 0.00001f);
    XII_TEST_FLOAT(m.Element(3, 1), 8, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 2), 9, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 2), 10, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 2), 11, 0.00001f);
    XII_TEST_FLOAT(m.Element(3, 2), 12, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 3), 13, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 3), 14, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 3), 15, 0.00001f);
    XII_TEST_FLOAT(m.Element(3, 3), 16, 0.00001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeTransformation")
  {
    xiiMat3T mr = xiiMat3T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);
    xiiVec3T vt(10, 11, 12);

    xiiMat4T m = xiiMat4T::MakeTransformation(mr, vt);

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0);
    XII_TEST_FLOAT(m.Element(1, 0), 2, 0);
    XII_TEST_FLOAT(m.Element(2, 0), 3, 0);
    XII_TEST_FLOAT(m.Element(3, 0), 10, 0);
    XII_TEST_FLOAT(m.Element(0, 1), 4, 0);
    XII_TEST_FLOAT(m.Element(1, 1), 5, 0);
    XII_TEST_FLOAT(m.Element(2, 1), 6, 0);
    XII_TEST_FLOAT(m.Element(3, 1), 11, 0);
    XII_TEST_FLOAT(m.Element(0, 2), 7, 0);
    XII_TEST_FLOAT(m.Element(1, 2), 8, 0);
    XII_TEST_FLOAT(m.Element(2, 2), 9, 0);
    XII_TEST_FLOAT(m.Element(3, 2), 12, 0);
    XII_TEST_FLOAT(m.Element(0, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsArray")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiMathTestType data[16];

    m.GetAsArray(data, xiiMatrixLayout::ColumnMajor);
    XII_TEST_FLOAT(data[0], 1, 0.0001f);
    XII_TEST_FLOAT(data[1], 5, 0.0001f);
    XII_TEST_FLOAT(data[2], 9, 0.0001f);
    XII_TEST_FLOAT(data[3], 13, 0.0001f);
    XII_TEST_FLOAT(data[4], 2, 0.0001f);
    XII_TEST_FLOAT(data[5], 6, 0.0001f);
    XII_TEST_FLOAT(data[6], 10, 0.0001f);
    XII_TEST_FLOAT(data[7], 14, 0.0001f);
    XII_TEST_FLOAT(data[8], 3, 0.0001f);
    XII_TEST_FLOAT(data[9], 7, 0.0001f);
    XII_TEST_FLOAT(data[10], 11, 0.0001f);
    XII_TEST_FLOAT(data[11], 15, 0.0001f);
    XII_TEST_FLOAT(data[12], 4, 0.0001f);
    XII_TEST_FLOAT(data[13], 8, 0.0001f);
    XII_TEST_FLOAT(data[14], 12, 0.0001f);
    XII_TEST_FLOAT(data[15], 16, 0.0001f);

    m.GetAsArray(data, xiiMatrixLayout::RowMajor);
    XII_TEST_FLOAT(data[0], 1, 0.0001f);
    XII_TEST_FLOAT(data[1], 2, 0.0001f);
    XII_TEST_FLOAT(data[2], 3, 0.0001f);
    XII_TEST_FLOAT(data[3], 4, 0.0001f);
    XII_TEST_FLOAT(data[4], 5, 0.0001f);
    XII_TEST_FLOAT(data[5], 6, 0.0001f);
    XII_TEST_FLOAT(data[6], 7, 0.0001f);
    XII_TEST_FLOAT(data[7], 8, 0.0001f);
    XII_TEST_FLOAT(data[8], 9, 0.0001f);
    XII_TEST_FLOAT(data[9], 10, 0.0001f);
    XII_TEST_FLOAT(data[10], 11, 0.0001f);
    XII_TEST_FLOAT(data[11], 12, 0.0001f);
    XII_TEST_FLOAT(data[12], 13, 0.0001f);
    XII_TEST_FLOAT(data[13], 14, 0.0001f);
    XII_TEST_FLOAT(data[14], 15, 0.0001f);
    XII_TEST_FLOAT(data[15], 16, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetZero")
  {
    xiiMat4T m;
    m.SetZero();

    for (xiiUInt32 i = 0; i < 16; ++i)
      XII_TEST_FLOAT(m.m_fElementsCM[i], 0.0f, 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetIdentity")
  {
    xiiMat4T m;
    m.SetIdentity();

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0);
    XII_TEST_FLOAT(m.Element(1, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 1), 1, 0);
    XII_TEST_FLOAT(m.Element(2, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 2), 1, 0);
    XII_TEST_FLOAT(m.Element(3, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetTranslationMatrix")
  {
    xiiMat4T m = xiiMat4::MakeTranslation(xiiVec3T(2, 3, 4));

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0);
    XII_TEST_FLOAT(m.Element(1, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 0), 2, 0);
    XII_TEST_FLOAT(m.Element(0, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 1), 1, 0);
    XII_TEST_FLOAT(m.Element(2, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 1), 3, 0);
    XII_TEST_FLOAT(m.Element(0, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 2), 1, 0);
    XII_TEST_FLOAT(m.Element(3, 2), 4, 0);
    XII_TEST_FLOAT(m.Element(0, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetScalingMatrix")
  {
    xiiMat4T m = xiiMat4::MakeScaling(xiiVec3T(2, 3, 4));

    XII_TEST_FLOAT(m.Element(0, 0), 2, 0);
    XII_TEST_FLOAT(m.Element(1, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 1), 3, 0);
    XII_TEST_FLOAT(m.Element(2, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 2), 4, 0);
    XII_TEST_FLOAT(m.Element(3, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRotationMatrixX")
  {
    xiiMat4T m;

    m = xiiMat4::MakeRotationX(xiiAngle::MakeFromDegree(90));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, -3, 2), 0.0001f));

    m = xiiMat4::MakeRotationX(xiiAngle::MakeFromDegree(180));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, -2, -3), 0.0001f));

    m = xiiMat4::MakeRotationX(xiiAngle::MakeFromDegree(270));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, 3, -2), 0.0001f));

    m = xiiMat4::MakeRotationX(xiiAngle::MakeFromDegree(360));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, 2, 3), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRotationMatrixY")
  {
    xiiMat4T m;

    m = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(90));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(3, 2, -1), 0.0001f));

    m = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(180));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-1, 2, -3), 0.0001f));

    m = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(270));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-3, 2, 1), 0.0001f));

    m = xiiMat4::MakeRotationY(xiiAngle::MakeFromDegree(360));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, 2, 3), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRotationMatrixZ")
  {
    xiiMat4T m;

    m = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(90));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-2, 1, 3), 0.0001f));

    m = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(180));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-1, -2, 3), 0.0001f));

    m = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(270));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(2, -1, 3), 0.0001f));

    m = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree(360));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, 2, 3), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRotationMatrix")
  {
    xiiMat4T m;

    m = xiiMat4::MakeAxisRotation(xiiVec3T(1, 0, 0), xiiAngle::MakeFromDegree(90));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, -3, 2), xiiMath::DefaultEpsilon<xiiMat3T::ComponentType>()));

    m = xiiMat4::MakeAxisRotation(xiiVec3T(1, 0, 0), xiiAngle::MakeFromDegree(180));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, -2, -3), xiiMath::DefaultEpsilon<xiiMat3T::ComponentType>()));

    m = xiiMat4::MakeAxisRotation(xiiVec3T(1, 0, 0), xiiAngle::MakeFromDegree(270));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, 3, -2), xiiMath::DefaultEpsilon<xiiMat3T::ComponentType>()));

    m = xiiMat4::MakeAxisRotation(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(90));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(3, 2, -1), xiiMath::DefaultEpsilon<xiiMat3T::ComponentType>()));

    m = xiiMat4::MakeAxisRotation(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(180));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-1, 2, -3), xiiMath::DefaultEpsilon<xiiMat3T::ComponentType>()));

    m = xiiMat4::MakeAxisRotation(xiiVec3T(0, 1, 0), xiiAngle::MakeFromDegree(270));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-3, 2, 1), xiiMath::DefaultEpsilon<xiiMat3T::ComponentType>()));

    m = xiiMat4::MakeAxisRotation(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(90));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-2, 1, 3), xiiMath::DefaultEpsilon<xiiMat3T::ComponentType>()));

    m = xiiMat4::MakeAxisRotation(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(180));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-1, -2, 3), xiiMath::DefaultEpsilon<xiiMat3T::ComponentType>()));

    m = xiiMat4::MakeAxisRotation(xiiVec3T(0, 0, 1), xiiAngle::MakeFromDegree(270));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(2, -1, 3), xiiMath::DefaultEpsilon<xiiMat3T::ComponentType>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeIdentity")
  {
    xiiMat4T m = xiiMat4T::MakeIdentity();

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0);
    XII_TEST_FLOAT(m.Element(1, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 1), 1, 0);
    XII_TEST_FLOAT(m.Element(2, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 2), 1, 0);
    XII_TEST_FLOAT(m.Element(3, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 3), 1, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "MakeZero")
  {
    xiiMat4T m = xiiMat4T::MakeZero();

    XII_TEST_FLOAT(m.Element(0, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 3), 0, 0);
    XII_TEST_FLOAT(m.Element(3, 3), 0, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transpose")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m.Transpose();

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0);
    XII_TEST_FLOAT(m.Element(1, 0), 5, 0);
    XII_TEST_FLOAT(m.Element(2, 0), 9, 0);
    XII_TEST_FLOAT(m.Element(3, 0), 13, 0);
    XII_TEST_FLOAT(m.Element(0, 1), 2, 0);
    XII_TEST_FLOAT(m.Element(1, 1), 6, 0);
    XII_TEST_FLOAT(m.Element(2, 1), 10, 0);
    XII_TEST_FLOAT(m.Element(3, 1), 14, 0);
    XII_TEST_FLOAT(m.Element(0, 2), 3, 0);
    XII_TEST_FLOAT(m.Element(1, 2), 7, 0);
    XII_TEST_FLOAT(m.Element(2, 2), 11, 0);
    XII_TEST_FLOAT(m.Element(3, 2), 15, 0);
    XII_TEST_FLOAT(m.Element(0, 3), 4, 0);
    XII_TEST_FLOAT(m.Element(1, 3), 8, 0);
    XII_TEST_FLOAT(m.Element(2, 3), 12, 0);
    XII_TEST_FLOAT(m.Element(3, 3), 16, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetTranspose")
  {
    xiiMat4T m0 = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiMat4T m = m0.GetTranspose();

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0);
    XII_TEST_FLOAT(m.Element(1, 0), 5, 0);
    XII_TEST_FLOAT(m.Element(2, 0), 9, 0);
    XII_TEST_FLOAT(m.Element(3, 0), 13, 0);
    XII_TEST_FLOAT(m.Element(0, 1), 2, 0);
    XII_TEST_FLOAT(m.Element(1, 1), 6, 0);
    XII_TEST_FLOAT(m.Element(2, 1), 10, 0);
    XII_TEST_FLOAT(m.Element(3, 1), 14, 0);
    XII_TEST_FLOAT(m.Element(0, 2), 3, 0);
    XII_TEST_FLOAT(m.Element(1, 2), 7, 0);
    XII_TEST_FLOAT(m.Element(2, 2), 11, 0);
    XII_TEST_FLOAT(m.Element(3, 2), 15, 0);
    XII_TEST_FLOAT(m.Element(0, 3), 4, 0);
    XII_TEST_FLOAT(m.Element(1, 3), 8, 0);
    XII_TEST_FLOAT(m.Element(2, 3), 12, 0);
    XII_TEST_FLOAT(m.Element(3, 3), 16, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invert")
  {
    for (float x = 1.0f; x < 360.0f; x += 10.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 17.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 23.0f)
        {
          xiiMat4T m, inv;
          m   = xiiMat4::MakeAxisRotation(xiiVec3T(x, y, z).GetNormalized(), xiiAngle::MakeFromDegree(19.0f));
          inv = m;
          XII_TEST_BOOL(inv.Invert() == XII_SUCCESS);

          xiiVec3T v    = m * xiiVec3T(1, 1, 1);
          xiiVec3T vinv = inv * v;

          XII_TEST_VEC3(vinv, xiiVec3T(1, 1, 1), xiiMath::DefaultEpsilon<xiiMathTestType>());
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetInverse")
  {
    for (float x = 1.0f; x < 360.0f; x += 9.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 19.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 21.0f)
        {
          xiiMat4T m, inv;
          m   = xiiMat4::MakeAxisRotation(xiiVec3T(x, y, z).GetNormalized(), xiiAngle::MakeFromDegree(83.0f));
          inv = m.GetInverse();

          xiiVec3T v    = m * xiiVec3T(1, 1, 1);
          xiiVec3T vinv = inv * v;

          XII_TEST_VEC3(vinv, xiiVec3T(1, 1, 1), xiiMath::DefaultEpsilon<xiiMathTestType>());
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsZero")
  {
    xiiMat4T m;

    m.SetIdentity();
    XII_TEST_BOOL(!m.IsZero());

    m.SetZero();
    XII_TEST_BOOL(m.IsZero());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdentity")
  {
    xiiMat4T m;

    m.SetIdentity();
    XII_TEST_BOOL(m.IsIdentity());

    m.SetZero();
    XII_TEST_BOOL(!m.IsIdentity());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsValid")
  {
    if (xiiMath::SupportsNaN<xiiMat3T::ComponentType>())
    {
      xiiMat4T m;

      m.SetZero();
      XII_TEST_BOOL(m.IsValid());

      m.m_fElementsCM[0] = xiiMath::NaN<xiiMat4T::ComponentType>();
      XII_TEST_BOOL(!m.IsValid());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRow")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    XII_TEST_VEC4(m.GetRow(0), xiiVec4T(1, 2, 3, 4), 0.0f);
    XII_TEST_VEC4(m.GetRow(1), xiiVec4T(5, 6, 7, 8), 0.0f);
    XII_TEST_VEC4(m.GetRow(2), xiiVec4T(9, 10, 11, 12), 0.0f);
    XII_TEST_VEC4(m.GetRow(3), xiiVec4T(13, 14, 15, 16), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRow")
  {
    xiiMat4T m;
    m.SetZero();

    m.SetRow(0, xiiVec4T(1, 2, 3, 4));
    XII_TEST_VEC4(m.GetRow(0), xiiVec4T(1, 2, 3, 4), 0.0f);

    m.SetRow(1, xiiVec4T(5, 6, 7, 8));
    XII_TEST_VEC4(m.GetRow(1), xiiVec4T(5, 6, 7, 8), 0.0f);

    m.SetRow(2, xiiVec4T(9, 10, 11, 12));
    XII_TEST_VEC4(m.GetRow(2), xiiVec4T(9, 10, 11, 12), 0.0f);

    m.SetRow(3, xiiVec4T(13, 14, 15, 16));
    XII_TEST_VEC4(m.GetRow(3), xiiVec4T(13, 14, 15, 16), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetColumn")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    XII_TEST_VEC4(m.GetColumn(0), xiiVec4T(1, 5, 9, 13), 0.0f);
    XII_TEST_VEC4(m.GetColumn(1), xiiVec4T(2, 6, 10, 14), 0.0f);
    XII_TEST_VEC4(m.GetColumn(2), xiiVec4T(3, 7, 11, 15), 0.0f);
    XII_TEST_VEC4(m.GetColumn(3), xiiVec4T(4, 8, 12, 16), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetColumn")
  {
    xiiMat4T m;
    m.SetZero();

    m.SetColumn(0, xiiVec4T(1, 2, 3, 4));
    XII_TEST_VEC4(m.GetColumn(0), xiiVec4T(1, 2, 3, 4), 0.0f);

    m.SetColumn(1, xiiVec4T(5, 6, 7, 8));
    XII_TEST_VEC4(m.GetColumn(1), xiiVec4T(5, 6, 7, 8), 0.0f);

    m.SetColumn(2, xiiVec4T(9, 10, 11, 12));
    XII_TEST_VEC4(m.GetColumn(2), xiiVec4T(9, 10, 11, 12), 0.0f);

    m.SetColumn(3, xiiVec4T(13, 14, 15, 16));
    XII_TEST_VEC4(m.GetColumn(3), xiiVec4T(13, 14, 15, 16), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDiagonal")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    XII_TEST_VEC4(m.GetDiagonal(), xiiVec4T(1, 6, 11, 16), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetDiagonal")
  {
    xiiMat4T m;
    m.SetZero();

    m.SetDiagonal(xiiVec4T(1, 2, 3, 4));
    XII_TEST_VEC4(m.GetColumn(0), xiiVec4T(1, 0, 0, 0), 0.0f);
    XII_TEST_VEC4(m.GetColumn(1), xiiVec4T(0, 2, 0, 0), 0.0f);
    XII_TEST_VEC4(m.GetColumn(2), xiiVec4T(0, 0, 3, 0), 0.0f);
    XII_TEST_VEC4(m.GetColumn(3), xiiVec4T(0, 0, 0, 4), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetTranslationVector")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    XII_TEST_VEC3(m.GetTranslationVector(), xiiVec3T(4, 8, 12), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetTranslationVector")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m.SetTranslationVector(xiiVec3T(17, 18, 19));
    XII_TEST_VEC4(m.GetRow(0), xiiVec4T(1, 2, 3, 17), 0.0f);
    XII_TEST_VEC4(m.GetRow(1), xiiVec4T(5, 6, 7, 18), 0.0f);
    XII_TEST_VEC4(m.GetRow(2), xiiVec4T(9, 10, 11, 19), 0.0f);
    XII_TEST_VEC4(m.GetRow(3), xiiVec4T(13, 14, 15, 16), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRotationalPart")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiMat3T r = xiiMat3T::MakeFromValues(17, 18, 19, 20, 21, 22, 23, 24, 25);

    m.SetRotationalPart(r);
    XII_TEST_VEC4(m.GetRow(0), xiiVec4T(17, 18, 19, 4), 0.0f);
    XII_TEST_VEC4(m.GetRow(1), xiiVec4T(20, 21, 22, 8), 0.0f);
    XII_TEST_VEC4(m.GetRow(2), xiiVec4T(23, 24, 25, 12), 0.0f);
    XII_TEST_VEC4(m.GetRow(3), xiiVec4T(13, 14, 15, 16), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRotationalPart")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiMat3T r = m.GetRotationalPart();
    XII_TEST_VEC3(r.GetRow(0), xiiVec3T(1, 2, 3), 0.0f);
    XII_TEST_VEC3(r.GetRow(1), xiiVec3T(5, 6, 7), 0.0f);
    XII_TEST_VEC3(r.GetRow(2), xiiVec3T(9, 10, 11), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetScalingFactors")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiVec3T s = m.GetScalingFactors();
    XII_TEST_VEC3(s, xiiVec3T(xiiMath::Sqrt((xiiMathTestType)(1 * 1 + 5 * 5 + 9 * 9)), xiiMath::Sqrt((xiiMathTestType)(2 * 2 + 6 * 6 + 10 * 10)), xiiMath::Sqrt((xiiMathTestType)(3 * 3 + 7 * 7 + 11 * 11))), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetScalingFactors")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    XII_TEST_BOOL(m.SetScalingFactors(xiiVec3T(1, 2, 3)) == XII_SUCCESS);

    xiiVec3T s = m.GetScalingFactors();
    XII_TEST_VEC3(s, xiiVec3T(1, 2, 3), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformDirection")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const xiiVec3T r = m.TransformDirection(xiiVec3T(1, 2, 3));

    XII_TEST_VEC3(r, xiiVec3T(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformDirection(array)")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiVec3T data[3] = {xiiVec3T(1, 2, 3), xiiVec3T(4, 5, 6), xiiVec3T(7, 8, 9)};

    m.TransformDirection(data, 2);

    XII_TEST_VEC3(data[0], xiiVec3T(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), 0.0001f);
    XII_TEST_VEC3(data[1], xiiVec3T(4 * 1 + 5 * 2 + 6 * 3, 4 * 5 + 5 * 6 + 6 * 7, 4 * 9 + 5 * 10 + 6 * 11), 0.0001f);
    XII_TEST_VEC3(data[2], xiiVec3T(7, 8, 9), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformPosition")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const xiiVec3T r = m.TransformPosition(xiiVec3T(1, 2, 3));

    XII_TEST_VEC3(r, xiiVec3T(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformPosition(array)")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiVec3T data[3] = {xiiVec3T(1, 2, 3), xiiVec3T(4, 5, 6), xiiVec3T(7, 8, 9)};

    m.TransformPosition(data, 2);

    XII_TEST_VEC3(data[0], xiiVec3T(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f);
    XII_TEST_VEC3(data[1], xiiVec3T(4 * 1 + 5 * 2 + 6 * 3 + 4, 4 * 5 + 5 * 6 + 6 * 7 + 8, 4 * 9 + 5 * 10 + 6 * 11 + 12), 0.0001f);
    XII_TEST_VEC3(data[2], xiiVec3T(7, 8, 9), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transform")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const xiiVec4T r = m.Transform(xiiVec4T(1, 2, 3, 4));

    XII_TEST_VEC4(r,
                  xiiVec4T(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 8 * 4, 1 * 9 + 2 * 10 + 3 * 11 + 12 * 4, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16),
                  0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transform(array)")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiVec4T data[3] = {xiiVec4T(1, 2, 3, 4), xiiVec4T(5, 6, 7, 8), xiiVec4T(9, 10, 11, 12)};

    m.Transform(data, 2);

    XII_TEST_VEC4(data[0],
                  xiiVec4T(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 8 * 4, 1 * 9 + 2 * 10 + 3 * 11 + 12 * 4, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16),
                  0.0001f);
    XII_TEST_VEC4(data[1],
                  xiiVec4T(5 * 1 + 6 * 2 + 7 * 3 + 8 * 4, 5 * 5 + 6 * 6 + 7 * 7 + 8 * 8, 5 * 9 + 6 * 10 + 7 * 11 + 12 * 8, 5 * 13 + 6 * 14 + 7 * 15 + 8 * 16),
                  0.0001f);
    XII_TEST_VEC4(data[2], xiiVec4T(9, 10, 11, 12), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*=")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m *= 2.0f;

    XII_TEST_VEC4(m.GetRow(0), xiiVec4T(2, 4, 6, 8), 0.0001f);
    XII_TEST_VEC4(m.GetRow(1), xiiVec4T(10, 12, 14, 16), 0.0001f);
    XII_TEST_VEC4(m.GetRow(2), xiiVec4T(18, 20, 22, 24), 0.0001f);
    XII_TEST_VEC4(m.GetRow(3), xiiVec4T(26, 28, 30, 32), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator/=")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m *= 4.0f;
    m /= 2.0f;

    XII_TEST_VEC4(m.GetRow(0), xiiVec4T(2, 4, 6, 8), 0.0001f);
    XII_TEST_VEC4(m.GetRow(1), xiiVec4T(10, 12, 14, 16), 0.0001f);
    XII_TEST_VEC4(m.GetRow(2), xiiVec4T(18, 20, 22, 24), 0.0001f);
    XII_TEST_VEC4(m.GetRow(3), xiiVec4T(26, 28, 30, 32), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdentical")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiMat4T m2 = m;

    XII_TEST_BOOL(m.IsIdentical(m2));

    m2.m_fElementsCM[0] += 0.00001f;
    XII_TEST_BOOL(!m.IsIdentical(m2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiMat4T m2 = m;

    XII_TEST_BOOL(m.IsEqual(m2, 0.0001f));

    m2.m_fElementsCM[0] += 0.00001f;
    XII_TEST_BOOL(m.IsEqual(m2, 0.0001f));
    XII_TEST_BOOL(!m.IsEqual(m2, 0.000001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(mat, mat)")
  {
    xiiMat4T m1 = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiMat4T m2 = xiiMat4T::MakeFromValues(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

    xiiMat4T r = m1 * m2;

    XII_TEST_VEC4(r.GetColumn(0),
                  xiiVec4T(-1 * 1 + -5 * 2 + -9 * 3 + -13 * 4, -1 * 5 + -5 * 6 + -9 * 7 + -13 * 8, -1 * 9 + -5 * 10 + -9 * 11 + -13 * 12,
                           -1 * 13 + -5 * 14 + -9 * 15 + -13 * 16),
                  0.001f);
    XII_TEST_VEC4(r.GetColumn(1),
                  xiiVec4T(-2 * 1 + -6 * 2 + -10 * 3 + -14 * 4, -2 * 5 + -6 * 6 + -10 * 7 + -14 * 8, -2 * 9 + -6 * 10 + -10 * 11 + -14 * 12,
                           -2 * 13 + -6 * 14 + -10 * 15 + -14 * 16),
                  0.001f);
    XII_TEST_VEC4(r.GetColumn(2),
                  xiiVec4T(-3 * 1 + -7 * 2 + -11 * 3 + -15 * 4, -3 * 5 + -7 * 6 + -11 * 7 + -15 * 8, -3 * 9 + -7 * 10 + -11 * 11 + -15 * 12,
                           -3 * 13 + -7 * 14 + -11 * 15 + -15 * 16),
                  0.001f);
    XII_TEST_VEC4(r.GetColumn(3),
                  xiiVec4T(-4 * 1 + -8 * 2 + -12 * 3 + -16 * 4, -4 * 5 + -8 * 6 + -12 * 7 + -16 * 8, -4 * 9 + -8 * 10 + -12 * 11 + -16 * 12,
                           -4 * 13 + -8 * 14 + -12 * 15 + -16 * 16),
                  0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(mat, vec3)")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const xiiVec3T r = m * xiiVec3T(1, 2, 3);

    XII_TEST_VEC3(r, xiiVec3T(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(mat, vec4)")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const xiiVec4T r = m * xiiVec4T(1, 2, 3, 4);

    XII_TEST_VEC4(r,
                  xiiVec4T(1 * 1 + 2 * 2 + 3 * 3 + 4 * 4, 1 * 5 + 2 * 6 + 3 * 7 + 4 * 8, 1 * 9 + 2 * 10 + 3 * 11 + 4 * 12, 1 * 13 + 2 * 14 + 3 * 15 + 4 * 16),
                  0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(mat, float) | operator*(float, mat)")
  {
    xiiMat4T m0 = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiMat4T m  = m0 * (xiiMathTestType)2;
    xiiMat4T m2 = (xiiMathTestType)2 * m0;

    XII_TEST_VEC4(m.GetRow(0), xiiVec4T(2, 4, 6, 8), 0.0001f);
    XII_TEST_VEC4(m.GetRow(1), xiiVec4T(10, 12, 14, 16), 0.0001f);
    XII_TEST_VEC4(m.GetRow(2), xiiVec4T(18, 20, 22, 24), 0.0001f);
    XII_TEST_VEC4(m.GetRow(3), xiiVec4T(26, 28, 30, 32), 0.0001f);

    XII_TEST_VEC4(m2.GetRow(0), xiiVec4T(2, 4, 6, 8), 0.0001f);
    XII_TEST_VEC4(m2.GetRow(1), xiiVec4T(10, 12, 14, 16), 0.0001f);
    XII_TEST_VEC4(m2.GetRow(2), xiiVec4T(18, 20, 22, 24), 0.0001f);
    XII_TEST_VEC4(m2.GetRow(3), xiiVec4T(26, 28, 30, 32), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator/(mat, float)")
  {
    xiiMat4T m0 = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m0 *= (xiiMathTestType)4;

    xiiMat4T m = m0 / (xiiMathTestType)2;

    XII_TEST_VEC4(m.GetRow(0), xiiVec4T(2, 4, 6, 8), 0.0001f);
    XII_TEST_VEC4(m.GetRow(1), xiiVec4T(10, 12, 14, 16), 0.0001f);
    XII_TEST_VEC4(m.GetRow(2), xiiVec4T(18, 20, 22, 24), 0.0001f);
    XII_TEST_VEC4(m.GetRow(3), xiiVec4T(26, 28, 30, 32), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator+(mat, mat) | operator-(mat, mat)")
  {
    xiiMat4T m0 = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiMat4T m1 = xiiMat4T::MakeFromValues(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

    XII_TEST_BOOL((m0 + m1).IsZero());
    XII_TEST_BOOL((m0 - m1).IsEqual(m0 * (xiiMathTestType)2, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    xiiMat4T m = xiiMat4T::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiMat4T m2 = m;

    XII_TEST_BOOL(m == m2);

    m2.m_fElementsCM[0] += 0.00001f;

    XII_TEST_BOOL(m != m2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      xiiMat4T m;

      m.SetIdentity();
      XII_TEST_BOOL(!m.IsNaN());

      for (xiiUInt32 i = 0; i < 16; ++i)
      {
        m.SetIdentity();
        m.m_fElementsCM[i] = xiiMath::NaN<xiiMathTestType>();

        XII_TEST_BOOL(m.IsNaN());
      }
    }
  }
}
