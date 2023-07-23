#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/SimdMath/SimdTransformd.h>

XII_CREATE_SIMPLE_TEST(SimdMath, SimdMat4d)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (Array Data)")
  {
    const double data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      xiiSimdMat4d m(data, xiiMatrixLayout::ColumnMajor);

      XII_TEST_BOOL((m.m_col0 == xiiSimdVec4d(1, 2, 3, 4)).AllSet());
      XII_TEST_BOOL((m.m_col1 == xiiSimdVec4d(5, 6, 7, 8)).AllSet());
      XII_TEST_BOOL((m.m_col2 == xiiSimdVec4d(9, 10, 11, 12)).AllSet());
      XII_TEST_BOOL((m.m_col3 == xiiSimdVec4d(13, 14, 15, 16)).AllSet());
    }

    {
      xiiSimdMat4d m(data, xiiMatrixLayout::RowMajor);

      XII_TEST_BOOL((m.m_col0 == xiiSimdVec4d(1, 5, 9, 13)).AllSet());
      XII_TEST_BOOL((m.m_col1 == xiiSimdVec4d(2, 6, 10, 14)).AllSet());
      XII_TEST_BOOL((m.m_col2 == xiiSimdVec4d(3, 7, 11, 15)).AllSet());
      XII_TEST_BOOL((m.m_col3 == xiiSimdVec4d(4, 8, 12, 16)).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Constructor (Columns)")
  {
    xiiSimdVec4d c0(1, 2, 3, 4);
    xiiSimdVec4d c1(5, 6, 7, 8);
    xiiSimdVec4d c2(9, 10, 11, 12);
    xiiSimdVec4d c3(13, 14, 15, 16);

    xiiSimdMat4d m(c0, c1, c2, c3);

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4d(1, 2, 3, 4)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4d(5, 6, 7, 8)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4d(9, 10, 11, 12)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4d(13, 14, 15, 16)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetFromArray")
  {
    const double data[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    {
      xiiSimdMat4d m;
      m.SetFromArray(data, xiiMatrixLayout::ColumnMajor);

      XII_TEST_BOOL((m.m_col0 == xiiSimdVec4d(1, 2, 3, 4)).AllSet());
      XII_TEST_BOOL((m.m_col1 == xiiSimdVec4d(5, 6, 7, 8)).AllSet());
      XII_TEST_BOOL((m.m_col2 == xiiSimdVec4d(9, 10, 11, 12)).AllSet());
      XII_TEST_BOOL((m.m_col3 == xiiSimdVec4d(13, 14, 15, 16)).AllSet());
    }

    {
      xiiSimdMat4d m;
      m.SetFromArray(data, xiiMatrixLayout::RowMajor);

      XII_TEST_BOOL((m.m_col0 == xiiSimdVec4d(1, 5, 9, 13)).AllSet());
      XII_TEST_BOOL((m.m_col1 == xiiSimdVec4d(2, 6, 10, 14)).AllSet());
      XII_TEST_BOOL((m.m_col2 == xiiSimdVec4d(3, 7, 11, 15)).AllSet());
      XII_TEST_BOOL((m.m_col3 == xiiSimdVec4d(4, 8, 12, 16)).AllSet());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAsArray")
  {
    xiiSimdMat4d m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    double data[16];

    m.GetAsArray(data, xiiMatrixLayout::ColumnMajor);
    XII_TEST_DOUBLE(data[0], 1, 0.0001);
    XII_TEST_DOUBLE(data[1], 5, 0.0001);
    XII_TEST_DOUBLE(data[2], 9, 0.0001);
    XII_TEST_DOUBLE(data[3], 13, 0.0001);
    XII_TEST_DOUBLE(data[4], 2, 0.0001);
    XII_TEST_DOUBLE(data[5], 6, 0.0001);
    XII_TEST_DOUBLE(data[6], 10, 0.0001);
    XII_TEST_DOUBLE(data[7], 14, 0.0001);
    XII_TEST_DOUBLE(data[8], 3, 0.0001);
    XII_TEST_DOUBLE(data[9], 7, 0.0001);
    XII_TEST_DOUBLE(data[10], 11, 0.0001);
    XII_TEST_DOUBLE(data[11], 15, 0.0001);
    XII_TEST_DOUBLE(data[12], 4, 0.0001);
    XII_TEST_DOUBLE(data[13], 8, 0.0001);
    XII_TEST_DOUBLE(data[14], 12, 0.0001);
    XII_TEST_DOUBLE(data[15], 16, 0.0001);

    m.GetAsArray(data, xiiMatrixLayout::RowMajor);
    XII_TEST_DOUBLE(data[0], 1, 0.0001);
    XII_TEST_DOUBLE(data[1], 2, 0.0001);
    XII_TEST_DOUBLE(data[2], 3, 0.0001);
    XII_TEST_DOUBLE(data[3], 4, 0.0001);
    XII_TEST_DOUBLE(data[4], 5, 0.0001);
    XII_TEST_DOUBLE(data[5], 6, 0.0001);
    XII_TEST_DOUBLE(data[6], 7, 0.0001);
    XII_TEST_DOUBLE(data[7], 8, 0.0001);
    XII_TEST_DOUBLE(data[8], 9, 0.0001);
    XII_TEST_DOUBLE(data[9], 10, 0.0001);
    XII_TEST_DOUBLE(data[10], 11, 0.0001);
    XII_TEST_DOUBLE(data[11], 12, 0.0001);
    XII_TEST_DOUBLE(data[12], 13, 0.0001);
    XII_TEST_DOUBLE(data[13], 14, 0.0001);
    XII_TEST_DOUBLE(data[14], 15, 0.0001);
    XII_TEST_DOUBLE(data[15], 16, 0.0001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetIdentity")
  {
    xiiSimdMat4d m;
    m.SetIdentity();

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4d(1, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4d(0, 1, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4d(0, 0, 1, 0)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4d(0, 0, 0, 1)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetZero")
  {
    xiiSimdMat4d m;
    m.SetZero();

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4d(0, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4d(0, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4d(0, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4d(0, 0, 0, 0)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IdentityMatrix")
  {
    xiiSimdMat4d m = xiiSimdMat4d::IdentityMatrix();

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4d(1, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4d(0, 1, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4d(0, 0, 1, 0)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4d(0, 0, 0, 1)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ZeroMatrix")
  {
    xiiSimdMat4d m = xiiSimdMat4d::ZeroMatrix();

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4d(0, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4d(0, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4d(0, 0, 0, 0)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4d(0, 0, 0, 0)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transpose")
  {
    xiiSimdMat4d m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    m.Transpose();

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4d(1, 2, 3, 4)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4d(5, 6, 7, 8)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4d(9, 10, 11, 12)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4d(13, 14, 15, 16)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetTranspose")
  {
    xiiSimdMat4d m0(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiSimdMat4d m = m0.GetTranspose();

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4d(1, 2, 3, 4)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4d(5, 6, 7, 8)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4d(9, 10, 11, 12)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4d(13, 14, 15, 16)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invert")
  {
    for (double x = 1.0; x < 360.0; x += 20.0)
    {
      for (double y = 2.0; y < 360.0; y += 27.0)
      {
        for (double z = 3.0; z < 360.0; z += 33.0)
        {
          xiiSimdQuatd q;
          q.SetFromAxisAndAngle(xiiSimdVec4d(x, y, z).GetNormalized<3>(), xiiAngled::Degree(19.0));

          xiiSimdTransformd t(q);

          xiiSimdMat4d m, inv;
          m   = t.GetAsMat4();
          inv = m;
          XII_TEST_BOOL(inv.Invert() == XII_SUCCESS);

          xiiSimdVec4d v    = m.TransformDirection(xiiSimdVec4d(1, 3, -10));
          xiiSimdVec4d vinv = inv.TransformDirection(v);

          XII_TEST_BOOL(vinv.IsEqual(xiiSimdVec4d(1, 3, -10), xiiMath::DefaultEpsilon<double>()).AllSet<3>());
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetInverse")
  {
    for (double x = 1.0; x < 360.0; x += 19.0)
    {
      for (double y = 2.0; y < 360.0; y += 29.0)
      {
        for (double z = 3.0; z < 360.0; z += 31.0)
        {
          xiiSimdQuatd q;
          q.SetFromAxisAndAngle(xiiSimdVec4d(x, y, z).GetNormalized<3>(), xiiAngled::Degree(83.0));

          xiiSimdTransformd t(q);

          xiiSimdMat4d m, inv;
          m   = t.GetAsMat4();
          inv = m.GetInverse();

          xiiSimdVec4d v    = m.TransformDirection(xiiSimdVec4d(1, 3, -10));
          xiiSimdVec4d vinv = inv.TransformDirection(v);

          XII_TEST_BOOL(vinv.IsEqual(xiiSimdVec4d(1, 3, -10), xiiMath::DefaultEpsilon<double>()).AllSet<3>());
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsEqual")
  {
    xiiSimdMat4d m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiSimdMat4d m2 = m;

    XII_TEST_BOOL(m.IsEqual(m2, 0.0001));

    m2.m_col0 += xiiSimdVec4d(0.00001);
    XII_TEST_BOOL(m.IsEqual(m2, 0.0001));
    XII_TEST_BOOL(!m.IsEqual(m2, 0.000001));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsIdentity")
  {
    xiiSimdMat4d m;

    m.SetIdentity();
    XII_TEST_BOOL(m.IsIdentity());

    m.m_col0.SetZero();
    XII_TEST_BOOL(!m.IsIdentity());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsValid")
  {
    xiiSimdMat4d m;

    m.SetIdentity();
    XII_TEST_BOOL(m.IsValid());

    m.m_col0.SetX(xiiMath::NaN<double>());
    XII_TEST_BOOL(!m.IsValid());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsNaN")
  {
    xiiSimdMat4d m;

    m.SetIdentity();
    XII_TEST_BOOL(!m.IsNaN());

    double data[16];

    for (xiiUInt32 i = 0; i < 16; ++i)
    {
      m.SetIdentity();
      m.GetAsArray(data, xiiMatrixLayout::ColumnMajor);
      data[i] = xiiMath::NaN<double>();
      m.SetFromArray(data, xiiMatrixLayout::ColumnMajor);

      XII_TEST_BOOL(m.IsNaN());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetRows")
  {
    xiiSimdVec4d r0(1, 2, 3, 4);
    xiiSimdVec4d r1(5, 6, 7, 8);
    xiiSimdVec4d r2(9, 10, 11, 12);
    xiiSimdVec4d r3(13, 14, 15, 16);

    xiiSimdMat4d m;
    m.SetRows(r0, r1, r2, r3);

    XII_TEST_BOOL((m.m_col0 == xiiSimdVec4d(1, 5, 9, 13)).AllSet());
    XII_TEST_BOOL((m.m_col1 == xiiSimdVec4d(2, 6, 10, 14)).AllSet());
    XII_TEST_BOOL((m.m_col2 == xiiSimdVec4d(3, 7, 11, 15)).AllSet());
    XII_TEST_BOOL((m.m_col3 == xiiSimdVec4d(4, 8, 12, 16)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetRows")
  {
    xiiSimdMat4d m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiSimdVec4d r0, r1, r2, r3;
    m.GetRows(r0, r1, r2, r3);

    XII_TEST_BOOL((r0 == xiiSimdVec4d(1, 2, 3, 4)).AllSet());
    XII_TEST_BOOL((r1 == xiiSimdVec4d(5, 6, 7, 8)).AllSet());
    XII_TEST_BOOL((r2 == xiiSimdVec4d(9, 10, 11, 12)).AllSet());
    XII_TEST_BOOL((r3 == xiiSimdVec4d(13, 14, 15, 16)).AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformPosition")
  {
    xiiSimdMat4d m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const xiiSimdVec4d r = m.TransformPosition(xiiSimdVec4d(1, 2, 3));

    XII_TEST_BOOL(r.IsEqual(xiiSimdVec4d(1 * 1 + 2 * 2 + 3 * 3 + 4, 1 * 5 + 2 * 6 + 3 * 7 + 8, 1 * 9 + 2 * 10 + 3 * 11 + 12), 0.0001).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "TransformDirection")
  {
    xiiSimdMat4d m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    const xiiSimdVec4d r = m.TransformDirection(xiiSimdVec4d(1, 2, 3));

    XII_TEST_BOOL(r.IsEqual(xiiSimdVec4d(1 * 1 + 2 * 2 + 3 * 3, 1 * 5 + 2 * 6 + 3 * 7, 1 * 9 + 2 * 10 + 3 * 11), 0.0001).AllSet<3>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator*(mat, mat)")
  {
    xiiSimdMat4d m1(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiSimdMat4d m2(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16);

    xiiSimdMat4d r = m1 * m2;

    XII_TEST_BOOL((r.m_col0 == xiiSimdVec4d(-1 * 1 + -5 * 2 + -9 * 3 + -13 * 4, -1 * 5 + -5 * 6 + -9 * 7 + -13 * 8, -1 * 9 + -5 * 10 + -9 * 11 + -13 * 12, -1 * 13 + -5 * 14 + -9 * 15 + -13 * 16))
                    .AllSet());
    XII_TEST_BOOL((r.m_col1 == xiiSimdVec4d(-2 * 1 + -6 * 2 + -10 * 3 + -14 * 4, -2 * 5 + -6 * 6 + -10 * 7 + -14 * 8, -2 * 9 + -6 * 10 + -10 * 11 + -14 * 12, -2 * 13 + -6 * 14 + -10 * 15 + -14 * 16))
                    .AllSet());
    XII_TEST_BOOL((r.m_col2 == xiiSimdVec4d(-3 * 1 + -7 * 2 + -11 * 3 + -15 * 4, -3 * 5 + -7 * 6 + -11 * 7 + -15 * 8, -3 * 9 + -7 * 10 + -11 * 11 + -15 * 12, -3 * 13 + -7 * 14 + -11 * 15 + -15 * 16))
                    .AllSet());
    XII_TEST_BOOL((r.m_col3 == xiiSimdVec4d(-4 * 1 + -8 * 2 + -12 * 3 + -16 * 4, -4 * 5 + -8 * 6 + -12 * 7 + -16 * 8, -4 * 9 + -8 * 10 + -12 * 11 + -16 * 12, -4 * 13 + -8 * 14 + -12 * 15 + -16 * 16))
                    .AllSet());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator== (mat, mat) | operator!= (mat, mat)")
  {
    xiiSimdMat4d m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    xiiSimdMat4d m2 = m;

    XII_TEST_BOOL(m == m2);

    m2.m_col0 += xiiSimdVec4d(0.00001);

    XII_TEST_BOOL(m != m2);
  }
}
