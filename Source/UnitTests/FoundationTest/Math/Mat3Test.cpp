#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Implementation/AllClasses_inl.h>
#include <Foundation/Math/Mat3.h>

XII_CREATE_SIMPLE_TEST(Math, Mat3)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Default Constructor")
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      // In debug the default constructor initializes everything with NaN.
      xiiMat3T m;
      XII_TEST_BOOL(xiiMath::IsNaN(m.m_fElementsCM[0]) && xiiMath::IsNaN(m.m_fElementsCM[1]) && xiiMath::IsNaN(m.m_fElementsCM[2]) &&
                    xiiMath::IsNaN(m.m_fElementsCM[3]) && xiiMath::IsNaN(m.m_fElementsCM[4]) && xiiMath::IsNaN(m.m_fElementsCM[5]) &&
                    xiiMath::IsNaN(m.m_fElementsCM[6]) && xiiMath::IsNaN(m.m_fElementsCM[7]) && xiiMath::IsNaN(m.m_fElementsCM[8]));
    }
#else
    // Placement new of the default constructor should not have any effect on the previous data.
    xiiMat3T::ComponentType testBlock[9] = {(xiiMat3T::ComponentType)1, (xiiMat3T::ComponentType)2, (xiiMat3T::ComponentType)3, (xiiMat3T::ComponentType)4,
                                            (xiiMat3T::ComponentType)5, (xiiMat3T::ComponentType)6, (xiiMat3T::ComponentType)7, (xiiMat3T::ComponentType)8, (xiiMat3T::ComponentType)9};

    xiiMat3T* m = ::new ((void*)&testBlock[0]) xiiMat3T;

    XII_TEST_BOOL(m->m_fElementsCM[0] == (xiiMat3T::ComponentType)1 && m->m_fElementsCM[1] == (xiiMat3T::ComponentType)2 &&
                  m->m_fElementsCM[2] == (xiiMat3T::ComponentType)3 && m->m_fElementsCM[3] == (xiiMat3T::ComponentType)4 &&
                  m->m_fElementsCM[4] == (xiiMat3T::ComponentType)5 && m->m_fElementsCM[5] == (xiiMat3T::ComponentType)6 &&
                  m->m_fElementsCM[6] == (xiiMat3T::ComponentType)7 && m->m_fElementsCM[7] == (xiiMat3T::ComponentType)8 &&
                  m->m_fElementsCM[8] == (xiiMat3T::ComponentType)9);
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (Array Data)")
  {
    const xiiMathTestType data[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    {
      xiiMat3T m(data, xiiMatrixLayout::ColumnMajor);

      XII_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 2.0f && m.m_fElementsCM[2] == 3.0f && m.m_fElementsCM[3] == 4.0f &&
                    m.m_fElementsCM[4] == 5.0f && m.m_fElementsCM[5] == 6.0f && m.m_fElementsCM[6] == 7.0f && m.m_fElementsCM[7] == 8.0f &&
                    m.m_fElementsCM[8] == 9.0f);
    }

    {
      xiiMat3T m(data, xiiMatrixLayout::RowMajor);

      XII_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 4.0f && m.m_fElementsCM[2] == 7.0f && m.m_fElementsCM[3] == 2.0f &&
                    m.m_fElementsCM[4] == 5.0f && m.m_fElementsCM[5] == 8.0f && m.m_fElementsCM[6] == 3.0f && m.m_fElementsCM[7] == 6.0f &&
                    m.m_fElementsCM[8] == 9.0f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (Elements)")
  {
    xiiMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 0), 2, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 0), 3, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 1), 4, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 1), 5, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 1), 6, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 2), 7, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 2), 8, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 2), 9, 0.00001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromArray")
  {
    const xiiMathTestType data[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    {
      xiiMat3T m;
      m.SetFromArray(data, xiiMatrixLayout::ColumnMajor);

      XII_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 2.0f && m.m_fElementsCM[2] == 3.0f && m.m_fElementsCM[3] == 4.0f &&
                    m.m_fElementsCM[4] == 5.0f && m.m_fElementsCM[5] == 6.0f && m.m_fElementsCM[6] == 7.0f && m.m_fElementsCM[7] == 8.0f &&
                    m.m_fElementsCM[8] == 9.0f);
    }

    {
      xiiMat3T m;
      m.SetFromArray(data, xiiMatrixLayout::RowMajor);

      XII_TEST_BOOL(m.m_fElementsCM[0] == 1.0f && m.m_fElementsCM[1] == 4.0f && m.m_fElementsCM[2] == 7.0f && m.m_fElementsCM[3] == 2.0f &&
                    m.m_fElementsCM[4] == 5.0f && m.m_fElementsCM[5] == 8.0f && m.m_fElementsCM[6] == 3.0f && m.m_fElementsCM[7] == 6.0f &&
                    m.m_fElementsCM[8] == 9.0f);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetElements")
  {
    xiiMat3T m;
    m.SetElements(1, 2, 3, 4, 5, 6, 7, 8, 9);

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 0), 2, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 0), 3, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 1), 4, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 1), 5, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 1), 6, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 2), 7, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 2), 8, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 2), 9, 0.00001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsArray")
  {
    xiiMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    xiiMathTestType data[9];

    m.GetAsArray(data, xiiMatrixLayout::ColumnMajor);
    XII_TEST_FLOAT(data[0], 1, 0.0001f);
    XII_TEST_FLOAT(data[1], 4, 0.0001f);
    XII_TEST_FLOAT(data[2], 7, 0.0001f);
    XII_TEST_FLOAT(data[3], 2, 0.0001f);
    XII_TEST_FLOAT(data[4], 5, 0.0001f);
    XII_TEST_FLOAT(data[5], 8, 0.0001f);
    XII_TEST_FLOAT(data[6], 3, 0.0001f);
    XII_TEST_FLOAT(data[7], 6, 0.0001f);
    XII_TEST_FLOAT(data[8], 9, 0.0001f);

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
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetZero")
  {
    xiiMat3T m;
    m.SetZero();

    for (xiiUInt32 i = 0; i < 9; ++i)
      XII_TEST_FLOAT(m.m_fElementsCM[i], 0.0f, 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetIdentity")
  {
    xiiMat3T m;
    m.SetIdentity();

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0);
    XII_TEST_FLOAT(m.Element(1, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 1), 1, 0);
    XII_TEST_FLOAT(m.Element(2, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 2), 1, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetScalingMatrix")
  {
    xiiMat3T m;
    m.SetScalingMatrix(xiiVec3T(2, 3, 4));

    XII_TEST_FLOAT(m.Element(0, 0), 2, 0);
    XII_TEST_FLOAT(m.Element(1, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 1), 3, 0);
    XII_TEST_FLOAT(m.Element(2, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 2), 4, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRotationMatrixX")
  {
    xiiMat3T m;

    m.SetRotationMatrixX(xiiAngle::Degree(90));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, -3, 2), 0.0001f));

    m.SetRotationMatrixX(xiiAngle::Degree(180));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, -2, -3), 0.0001f));

    m.SetRotationMatrixX(xiiAngle::Degree(270));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, 3, -2), 0.0001f));

    m.SetRotationMatrixX(xiiAngle::Degree(360));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, 2, 3), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRotationMatrixY")
  {
    xiiMat3T m;

    m.SetRotationMatrixY(xiiAngle::Degree(90));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(3, 2, -1), 0.0001f));

    m.SetRotationMatrixY(xiiAngle::Degree(180));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-1, 2, -3), 0.0001f));

    m.SetRotationMatrixY(xiiAngle::Degree(270));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-3, 2, 1), 0.0001f));

    m.SetRotationMatrixY(xiiAngle::Degree(360));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, 2, 3), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRotationMatrixZ")
  {
    xiiMat3T m;

    m.SetRotationMatrixZ(xiiAngle::Degree(90));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-2, 1, 3), 0.0001f));

    m.SetRotationMatrixZ(xiiAngle::Degree(180));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-1, -2, 3), 0.0001f));

    m.SetRotationMatrixZ(xiiAngle::Degree(270));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(2, -1, 3), 0.0001f));

    m.SetRotationMatrixZ(xiiAngle::Degree(360));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, 2, 3), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRotationMatrix")
  {
    xiiMat3T m;

    m.SetRotationMatrix(xiiVec3T(1, 0, 0), xiiAngle::Degree(90));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, -3, 2), xiiMath::LargeEpsilon<xiiMathTestType>()));

    m.SetRotationMatrix(xiiVec3T(1, 0, 0), xiiAngle::Degree(180));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, -2, -3), xiiMath::LargeEpsilon<xiiMathTestType>()));

    m.SetRotationMatrix(xiiVec3T(1, 0, 0), xiiAngle::Degree(270));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(1, 3, -2), xiiMath::LargeEpsilon<xiiMathTestType>()));

    m.SetRotationMatrix(xiiVec3T(0, 1, 0), xiiAngle::Degree(90));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(3, 2, -1), xiiMath::LargeEpsilon<xiiMathTestType>()));

    m.SetRotationMatrix(xiiVec3T(0, 1, 0), xiiAngle::Degree(180));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-1, 2, -3), xiiMath::LargeEpsilon<xiiMathTestType>()));

    m.SetRotationMatrix(xiiVec3T(0, 1, 0), xiiAngle::Degree(270));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-3, 2, 1), xiiMath::LargeEpsilon<xiiMathTestType>()));

    m.SetRotationMatrix(xiiVec3T(0, 0, 1), xiiAngle::Degree(90));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-2, 1, 3), xiiMath::LargeEpsilon<xiiMathTestType>()));

    m.SetRotationMatrix(xiiVec3T(0, 0, 1), xiiAngle::Degree(180));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(-1, -2, 3), xiiMath::LargeEpsilon<xiiMathTestType>()));

    m.SetRotationMatrix(xiiVec3T(0, 0, 1), xiiAngle::Degree(270));
    XII_TEST_BOOL((m * xiiVec3T(1, 2, 3)).IsEqual(xiiVec3T(2, -1, 3), xiiMath::LargeEpsilon<xiiMathTestType>()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IdentityMatrix")
  {
    xiiMat3T m = xiiMat3T::IdentityMatrix();

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0);
    XII_TEST_FLOAT(m.Element(1, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 1), 1, 0);
    XII_TEST_FLOAT(m.Element(2, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 2), 1, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ZeroMatrix")
  {
    xiiMat3T m = xiiMat3T::ZeroMatrix();

    XII_TEST_FLOAT(m.Element(0, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 0), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 1), 0, 0);
    XII_TEST_FLOAT(m.Element(0, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(1, 2), 0, 0);
    XII_TEST_FLOAT(m.Element(2, 2), 0, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transpose")
  {
    xiiMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m.Transpose();

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 0), 4, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 0), 7, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 1), 2, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 1), 5, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 1), 8, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 2), 3, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 2), 6, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 2), 9, 0.00001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetTranspose")
  {
    xiiMat3T m0(1, 2, 3, 4, 5, 6, 7, 8, 9);

    xiiMat3T m = m0.GetTranspose();

    XII_TEST_FLOAT(m.Element(0, 0), 1, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 0), 4, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 0), 7, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 1), 2, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 1), 5, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 1), 8, 0.00001f);
    XII_TEST_FLOAT(m.Element(0, 2), 3, 0.00001f);
    XII_TEST_FLOAT(m.Element(1, 2), 6, 0.00001f);
    XII_TEST_FLOAT(m.Element(2, 2), 9, 0.00001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invert")
  {
    for (float x = 1.0f; x < 360.0f; x += 10.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 17.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 23.0f)
        {
          xiiMat3T m, inv;
          m.SetRotationMatrix(xiiVec3T(x, y, z).GetNormalized(), xiiAngle::Degree(19.0f));
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
          xiiMat3T m, inv;
          m.SetRotationMatrix(xiiVec3T(x, y, z).GetNormalized(), xiiAngle::Degree(83.0f));
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
    xiiMat3T m;

    m.SetIdentity();
    XII_TEST_BOOL(!m.IsZero());

    m.SetZero();
    XII_TEST_BOOL(m.IsZero());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdentity")
  {
    xiiMat3T m;

    m.SetIdentity();
    XII_TEST_BOOL(m.IsIdentity());

    m.SetZero();
    XII_TEST_BOOL(!m.IsIdentity());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsValid")
  {
    if (xiiMath::SupportsNaN<xiiMat3T::ComponentType>())
    {
      xiiMat3T m;

      m.SetZero();
      XII_TEST_BOOL(m.IsValid());

      m.m_fElementsCM[0] = xiiMath::NaN<xiiMat3T::ComponentType>();
      XII_TEST_BOOL(!m.IsValid());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRow")
  {
    xiiMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    XII_TEST_VEC3(m.GetRow(0), xiiVec3T(1, 2, 3), 0.0f);
    XII_TEST_VEC3(m.GetRow(1), xiiVec3T(4, 5, 6), 0.0f);
    XII_TEST_VEC3(m.GetRow(2), xiiVec3T(7, 8, 9), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRow")
  {
    xiiMat3T m;
    m.SetZero();

    m.SetRow(0, xiiVec3T(1, 2, 3));
    XII_TEST_VEC3(m.GetRow(0), xiiVec3T(1, 2, 3), 0.0f);

    m.SetRow(1, xiiVec3T(4, 5, 6));
    XII_TEST_VEC3(m.GetRow(1), xiiVec3T(4, 5, 6), 0.0f);

    m.SetRow(2, xiiVec3T(7, 8, 9));
    XII_TEST_VEC3(m.GetRow(2), xiiVec3T(7, 8, 9), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetColumn")
  {
    xiiMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    XII_TEST_VEC3(m.GetColumn(0), xiiVec3T(1, 4, 7), 0.0f);
    XII_TEST_VEC3(m.GetColumn(1), xiiVec3T(2, 5, 8), 0.0f);
    XII_TEST_VEC3(m.GetColumn(2), xiiVec3T(3, 6, 9), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetColumn")
  {
    xiiMat3T m;
    m.SetZero();

    m.SetColumn(0, xiiVec3T(1, 2, 3));
    XII_TEST_VEC3(m.GetColumn(0), xiiVec3T(1, 2, 3), 0.0f);

    m.SetColumn(1, xiiVec3T(4, 5, 6));
    XII_TEST_VEC3(m.GetColumn(1), xiiVec3T(4, 5, 6), 0.0f);

    m.SetColumn(2, xiiVec3T(7, 8, 9));
    XII_TEST_VEC3(m.GetColumn(2), xiiVec3T(7, 8, 9), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDiagonal")
  {
    xiiMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    XII_TEST_VEC3(m.GetDiagonal(), xiiVec3T(1, 5, 9), 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetDiagonal")
  {
    xiiMat3T m;
    m.SetZero();

    m.SetDiagonal(xiiVec3T(1, 2, 3));
    XII_TEST_VEC3(m.GetColumn(0), xiiVec3T(1, 0, 0), 0.0f);
    XII_TEST_VEC3(m.GetColumn(1), xiiVec3T(0, 2, 0), 0.0f);
    XII_TEST_VEC3(m.GetColumn(2), xiiVec3T(0, 0, 3), 0.0f);
  }


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetScalingFactors")
  {
    xiiMat3T m(1, 2, 3, 5, 6, 7, 9, 10, 11);

    xiiVec3T s = m.GetScalingFactors();
    XII_TEST_VEC3(s, xiiVec3T(xiiMath::Sqrt((xiiMathTestType)(1 * 1 + 5 * 5 + 9 * 9)), xiiMath::Sqrt((xiiMathTestType)(2 * 2 + 6 * 6 + 10 * 10)), xiiMath::Sqrt((xiiMathTestType)(3 * 3 + 7 * 7 + 11 * 11))), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetScalingFactors")
  {
    xiiMat3T m(1, 2, 3, 5, 6, 7, 9, 10, 11);

    XII_TEST_BOOL(m.SetScalingFactors(xiiVec3T(1, 2, 3)) == XII_SUCCESS);

    xiiVec3T s = m.GetScalingFactors();
    XII_TEST_VEC3(s, xiiVec3T(1, 2, 3), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformDirection")
  {
    xiiMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    const xiiVec3T r = m.TransformDirection(xiiVec3T(1, 2, 3));

    XII_TEST_VEC3(r, xiiVec3T(1 * 1 + 2 * 2 + 3 * 3, 1 * 4 + 2 * 5 + 3 * 6, 1 * 7 + 2 * 8 + 3 * 9), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*=")
  {
    xiiMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m *= 2.0f;

    XII_TEST_VEC3(m.GetRow(0), xiiVec3T(2, 4, 6), 0.0001f);
    XII_TEST_VEC3(m.GetRow(1), xiiVec3T(8, 10, 12), 0.0001f);
    XII_TEST_VEC3(m.GetRow(2), xiiVec3T(14, 16, 18), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator/=")
  {
    xiiMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m *= 4.0f;
    m /= 2.0f;

    XII_TEST_VEC3(m.GetRow(0), xiiVec3T(2, 4, 6), 0.0001f);
    XII_TEST_VEC3(m.GetRow(1), xiiVec3T(8, 10, 12), 0.0001f);
    XII_TEST_VEC3(m.GetRow(2), xiiVec3T(14, 16, 18), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdentical")
  {
    xiiMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    xiiMat3T m2 = m;

    XII_TEST_BOOL(m.IsIdentical(m2));

    m2.m_fElementsCM[0] += 0.00001f;
    XII_TEST_BOOL(!m.IsIdentical(m2));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    xiiMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    xiiMat3T m2 = m;

    XII_TEST_BOOL(m.IsEqual(m2, 0.0001f));

    m2.m_fElementsCM[0] += 0.00001f;
    XII_TEST_BOOL(m.IsEqual(m2, 0.0001f));
    XII_TEST_BOOL(!m.IsEqual(m2, 0.000001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(mat, mat)")
  {
    xiiMat3T m1(1, 2, 3, 4, 5, 6, 7, 8, 9);

    xiiMat3T m2(-1, -2, -3, -4, -5, -6, -7, -8, -9);

    xiiMat3T r = m1 * m2;

    XII_TEST_VEC3(r.GetColumn(0), xiiVec3T(-1 * 1 + -4 * 2 + -7 * 3, -1 * 4 + -4 * 5 + -7 * 6, -1 * 7 + -4 * 8 + -7 * 9), 0.001f);
    XII_TEST_VEC3(r.GetColumn(1), xiiVec3T(-2 * 1 + -5 * 2 + -8 * 3, -2 * 4 + -5 * 5 + -8 * 6, -2 * 7 + -5 * 8 + -8 * 9), 0.001f);
    XII_TEST_VEC3(r.GetColumn(2), xiiVec3T(-3 * 1 + -6 * 2 + -9 * 3, -3 * 4 + -6 * 5 + -9 * 6, -3 * 7 + -6 * 8 + -9 * 9), 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(mat, vec)")
  {
    xiiMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    const xiiVec3T r = m * (xiiVec3T(1, 2, 3));

    XII_TEST_VEC3(r, xiiVec3T(1 * 1 + 2 * 2 + 3 * 3, 1 * 4 + 2 * 5 + 3 * 6, 1 * 7 + 2 * 8 + 3 * 9), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(mat, float) | operator*(float, mat)")
  {
    xiiMat3T m0(1, 2, 3, 4, 5, 6, 7, 8, 9);

    xiiMat3T m  = m0 * (xiiMathTestType)2;
    xiiMat3T m2 = (xiiMathTestType)2 * m0;

    XII_TEST_VEC3(m.GetRow(0), xiiVec3T(2, 4, 6), 0.0001f);
    XII_TEST_VEC3(m.GetRow(1), xiiVec3T(8, 10, 12), 0.0001f);
    XII_TEST_VEC3(m.GetRow(2), xiiVec3T(14, 16, 18), 0.0001f);

    XII_TEST_VEC3(m2.GetRow(0), xiiVec3T(2, 4, 6), 0.0001f);
    XII_TEST_VEC3(m2.GetRow(1), xiiVec3T(8, 10, 12), 0.0001f);
    XII_TEST_VEC3(m2.GetRow(2), xiiVec3T(14, 16, 18), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator/(mat, float)")
  {
    xiiMat3T m0(1, 2, 3, 4, 5, 6, 7, 8, 9);

    m0 *= 4.0f;

    xiiMat3T m = m0 / (xiiMathTestType)2;

    XII_TEST_VEC3(m.GetRow(0), xiiVec3T(2, 4, 6), 0.0001f);
    XII_TEST_VEC3(m.GetRow(1), xiiVec3T(8, 10, 12), 0.0001f);
    XII_TEST_VEC3(m.GetRow(2), xiiVec3T(14, 16, 18), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator+(mat, mat) | operator-(mat, mat)")
  {
    xiiMat3T m0(1, 2, 3, 4, 5, 6, 7, 8, 9);

    xiiMat3T m1(-1, -2, -3, -4, -5, -6, -7, -8, -9);

    XII_TEST_BOOL((m0 + m1).IsZero());
    XII_TEST_BOOL((m0 - m1).IsEqual(m0 * (xiiMathTestType)2, 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    xiiMat3T m(1, 2, 3, 4, 5, 6, 7, 8, 9);

    xiiMat3T m2 = m;

    XII_TEST_BOOL(m == m2);

    m2.m_fElementsCM[0] += 0.00001f;

    XII_TEST_BOOL(m != m2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    if (xiiMath::SupportsNaN<xiiMathTestType>())
    {
      xiiMat3T m;

      m.SetIdentity();
      XII_TEST_BOOL(!m.IsNaN());

      for (xiiUInt32 i = 0; i < 9; ++i)
      {
        m.SetIdentity();
        m.m_fElementsCM[i] = xiiMath::NaN<xiiMathTestType>();

        XII_TEST_BOOL(m.IsNaN());
      }
    }
  }
}
