#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdTransform.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdMat4f)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (Array Data)")
  {
    const float data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      xiiSimdMat4f m(data, xiiMatrixLayout::ColumnMajor);

      XII_TEST_BOOL((m.m_col0 == xiiSimdVec4f(1, 2, 3, 4)).AllSet());
      XII_TEST_BOOL((m.m_col1 == xiiSimdVec4f(5, 6, 7, 8)).AllSet());
      XII_TEST_BOOL((m.m_col2 == xiiSimdVec4f(9, 10, 11, 12)).AllSet());
      XII_TEST_BOOL((m.m_col3 == xiiSimdVec4f(13, 14, 15, 16)).AllSet());
    }

    {
      xiiSimdMat4f m(data, xiiMatrixLayout::RowMajor);

      XII_TEST_BOOL((m.m_col0 == xiiSimdVec4f(1, 5, 9, 13)).AllSet());
      XII_TEST_BOOL((m.m_col1 == xiiSimdVec4f(2, 6, 10, 14)).AllSet());
      XII_TEST_BOOL((m.m_col2 == xiiSimdVec4f(3, 7, 11, 15)).AllSet());
      XII_TEST_BOOL((m.m_col3 == xiiSimdVec4f(4, 8, 12, 16)).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (Columns)")
  {
    xiiSimdVec4f c0(1, 2, 3, 4);
    xiiSimdVec4f c1(5, 6, 7, 8);
    xiiSimdVec4f c2(9, 10, 11, 12);
    xiiSimdVec4f c3(13, 14, 15, 16);

    xiiSimdMat4f m(c0, c1, c2, c3);

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4f(1, 2, 3, 4)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4f(5, 6, 7, 8)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4f(9, 10, 11, 12)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4f(13, 14, 15, 16)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromArray")
  {
    const float data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      xiiSimdMat4f m;
      m.SetFromArray(data, xiiMatrixLayout::ColumnMajor);

      XII_TEST_BOOL((m.m_col0 == xiiSimdVec4f(1, 2, 3, 4)).AllSet());
      XII_TEST_BOOL((m.m_col1 == xiiSimdVec4f(5, 6, 7, 8)).AllSet());
      XII_TEST_BOOL((m.m_col2 == xiiSimdVec4f(9, 10, 11, 12)).AllSet());
      XII_TEST_BOOL((m.m_col3 == xiiSimdVec4f(13, 14, 15, 16)).AllSet());
    }

    {
      xiiSimdMat4f m;
      m.SetFromArray(data, xiiMatrixLayout::RowMajor);

      XII_TEST_BOOL((m.m_col0 == xiiSimdVec4f(1, 5, 9, 13)).AllSet());
      XII_TEST_BOOL((m.m_col1 == xiiSimdVec4f(2, 6, 10, 14)).AllSet());
      XII_TEST_BOOL((m.m_col2 == xiiSimdVec4f(3, 7, 11, 15)).AllSet());
      XII_TEST_BOOL((m.m_col3 == xiiSimdVec4f(4, 8, 12, 16)).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsArray")
  {
    xiiSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    float data[16];

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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetIdentity")
  {
    xiiSimdMat4f m;
    m.SetIdentity();

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4f(1, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4f(0, 1, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4f(0, 0, 1, 0)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4f(0, 0, 0, 1)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetZero")
  {
    xiiSimdMat4f m;
    m.SetZero();

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4f(0, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4f(0, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4f(0, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4f(0, 0, 0, 0)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IdentityMatrix")
  {
    xiiSimdMat4f m = xiiSimdMat4f::IdentityMatrix();

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4f(1, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4f(0, 1, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4f(0, 0, 1, 0)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4f(0, 0, 0, 1)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ZeroMatrix")
  {
    xiiSimdMat4f m = xiiSimdMat4f::ZeroMatrix();

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4f(0, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4f(0, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4f(0, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4f(0, 0, 0, 0)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transpose")
  {
    xiiSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m.Transpose();

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4f(1, 2, 3, 4)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4f(5, 6, 7, 8)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4f(9, 10, 11, 12)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4f(13, 14, 15, 16)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetTranspose")
  {
    xiiSimdMat4f m0(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiSimdMat4f m = m0.GetTranspose();

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4f(1, 2, 3, 4)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4f(5, 6, 7, 8)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4f(9, 10, 11, 12)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4f(13, 14, 15, 16)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invert")
  {
    for (float x = 1.0f; x < 360.0f; x += 20.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 27.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 33.0f)
        {
          xiiSimdQuat q;
          q.SetFromAxisAndAngle(xiiSimdVec4f(x, y, z).GetNormalized<3>(), xiiAngle::Degree(19.0f));

          xiiSimdTransform t(q);

          xiiSimdMat4f m, inv;
          m   = t.GetAsMat4();
          inv = m;
          XII_TEST_BOOL(inv.Invert() == XII_SUCCESS);

          xiiSimdVec4f v    = m.TransformDirection(xiiSimdVec4f(1, 3, -10));
          xiiSimdVec4f vinv = inv.TransformDirection(v);

          XII_TEST_BOOL(vinv.IsEqual(xiiSimdVec4f(1, 3, -10), xiiMath::DefaultEpsilon<float>()).AllSet<3>());
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetInverse")
  {
    for (float x = 1.0f; x < 360.0f; x += 19.0f)
    {
      for (float y = 2.0f; y < 360.0f; y += 29.0f)
      {
        for (float z = 3.0f; z < 360.0f; z += 31.0f)
        {
          xiiSimdQuat q;
          q.SetFromAxisAndAngle(xiiSimdVec4f(x, y, z).GetNormalized<3>(), xiiAngle::Degree(83.0f));

          xiiSimdTransform t(q);

          xiiSimdMat4f m, inv;
          m   = t.GetAsMat4();
          inv = m.GetInverse();

          xiiSimdVec4f v    = m.TransformDirection(xiiSimdVec4f(1, 3, -10));
          xiiSimdVec4f vinv = inv.TransformDirection(v);

          XII_TEST_BOOL(vinv.IsEqual(xiiSimdVec4f(1, 3, -10), xiiMath::DefaultEpsilon<float>()).AllSet<3>());
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    xiiSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiSimdMat4f m2 = m;

    XII_TEST_BOOL(m.IsEqual(m2, 0.0001f));

    m2.m_col0 += xiiSimdVec4f(0.00001f);
    XII_TEST_BOOL(m.IsEqual(m2, 0.0001f));
    XII_TEST_BOOL(!m.IsEqual(m2, 0.000001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdentity")
  {
    xiiSimdMat4f m;

    m.SetIdentity();
    XII_TEST_BOOL(m.IsIdentity());

    m.m_col0.SetZero();
    XII_TEST_BOOL(!m.IsIdentity());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsValid")
  {
    xiiSimdMat4f m;

    m.SetIdentity();
    XII_TEST_BOOL(m.IsValid());

    m.m_col0.SetX(xiiMath::NaN<float>());
    XII_TEST_BOOL(!m.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    xiiSimdMat4f m;

    m.SetIdentity();
    XII_TEST_BOOL(!m.IsNaN());

    float data[16];

    for (xiiUInt32 i = 0; i < 16; ++i)
    {
      m.SetIdentity();
      m.GetAsArray(data, xiiMatrixLayout::ColumnMajor);
      data[i] = xiiMath::NaN<float>();
      m.SetFromArray(data, xiiMatrixLayout::ColumnMajor);

      XII_TEST_BOOL(m.IsNaN());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRows")
  {
    xiiSimdVec4f r0(1, 2, 3, 4);
    xiiSimdVec4f r1(5, 6, 7, 8);
    xiiSimdVec4f r2(9, 10, 11, 12);
    xiiSimdVec4f r3(13, 14, 15, 16);

    xiiSimdMat4f m;
    m.SetRows(r0, r1, r2, r3);

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4f(1, 5, 9, 13)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4f(2, 6, 10, 14)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4f(3, 7, 11, 15)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4f(4, 8, 12, 16)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRows")
  {
    xiiSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiSimdVec4f r0, r1, r2, r3;
    m.GetRows(r0, r1, r2, r3);

    XII_TEST_BOOL((r0 == xiiSimdVec4f(1, 2, 3, 4)).AllSet());
    XII_TEST_BOOL((r1 == xiiSimdVec4f(5, 6, 7, 8)).AllSet());
    XII_TEST_BOOL((r2 == xiiSimdVec4f(9, 10, 11, 12)).AllSet());
    XII_TEST_BOOL((r3 == xiiSimdVec4f(13, 14, 15, 16)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformPosition")
  {
    xiiSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const xiiSimdVec4f r = m.TransformPosition(xiiSimdVec4f(1, 2, 3));

    XII_TEST_BOOL(r.IsEqual(xiiSimdVec4f(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001f).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformDirection")
  {
    xiiSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const xiiSimdVec4f r = m.TransformDirection(xiiSimdVec4f(1, 2, 3));

    XII_TEST_BOOL(r.IsEqual(xiiSimdVec4f(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), 0.0001f).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(mat, mat)")
  {
    xiiSimdMat4f m1(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiSimdMat4f m2(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

    xiiSimdMat4f r = m1 * m2;

    XII_TEST_BOOL((r.m_col0 == xiiSimdVec4f(-1 * 1 + -5 * 2 + -9 * 3 + -13 * 4, -1 * 5 + -5 * 6 + -9 * 7 + -13 * 8, -1 * 9 + -5 * 10 + -9 * 11 + -13 * 12, -1 * 13 + -5 * 14 + -9 * 15 + -13 * 16))
                    .AllSet());
    XII_TEST_BOOL((r.m_col1 == xiiSimdVec4f(-2 * 1 + -6 * 2 + -10 * 3 + -14 * 4, -2 * 5 + -6 * 6 + -10 * 7 + -14 * 8, -2 * 9 + -6 * 10 + -10 * 11 + -14 * 12, -2 * 13 + -6 * 14 + -10 * 15 + -14 * 16))
                    .AllSet());
    XII_TEST_BOOL((r.m_col2 == xiiSimdVec4f(-3 * 1 + -7 * 2 + -11 * 3 + -15 * 4, -3 * 5 + -7 * 6 + -11 * 7 + -15 * 8, -3 * 9 + -7 * 10 + -11 * 11 + -15 * 12, -3 * 13 + -7 * 14 + -11 * 15 + -15 * 16))
                    .AllSet());
    XII_TEST_BOOL((r.m_col3 == xiiSimdVec4f(-4 * 1 + -8 * 2 + -12 * 3 + -16 * 4, -4 * 5 + -8 * 6 + -12 * 7 + -16 * 8, -4 * 9 + -8 * 10 + -12 * 11 + -16 * 12, -4 * 13 + -8 * 14 + -12 * 15 + -16 * 16))
                    .AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    xiiSimdMat4f m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiSimdMat4f m2 = m;

    XII_TEST_BOOL(m == m2);

    m2.m_col0 += xiiSimdVec4f(0.00001f);

    XII_TEST_BOOL(m != m2);
  }
}
