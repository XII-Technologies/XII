#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Intersection.h>
#include <Foundation/Math/Mat4.h>

XII_CREATE_SIMPLE_TEST(Math, Intersection)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RayPolygonIntersection")
  {
    for (xiiUInt32 i = 0; i < 100; ++i)
    {
      xiiMat4 m;
      m = xiiMat4::MakeAxisRotation(xiiVec3(i + 1.0f, i * 3.0f, i * 7.0f).GetNormalized(), xiiAngle::MakeFromDegree((float)i));
      m.SetTranslationVector(xiiVec3((float)i, i * 2.0f, i * 3.0f));

      xiiVec3 Vertices[8] = {m.TransformPosition(xiiVec3(-10, -10, 0)), xiiVec3(-10, -10, 0), m.TransformPosition(xiiVec3(10, -10, 0)),
                             xiiVec3(10, -10, 0), m.TransformPosition(xiiVec3(10, 10, 0)), xiiVec3(10, 10, 0), m.TransformPosition(xiiVec3(-10, 10, 0)), xiiVec3(-10, 10, 0)};

      for (float y = -14.5; y <= 14.5f; y += 2.0f)
      {
        for (float x = -14.5; x <= 14.5f; x += 2.0f)
        {
          const xiiVec3 vRayDir   = m.TransformDirection(xiiVec3(x, y, -10.0f));
          const xiiVec3 vRayStart = m.TransformPosition(xiiVec3(x, y, 0.0f)) - vRayDir * 3.0f;

          const bool bIntersects = (x >= -10.0f && x <= 10.0f && y >= -10.0f && y <= 10.0f);

          float   fIntersection;
          xiiVec3 vIntersection;
          XII_TEST_BOOL(xiiIntersectionUtils::RayPolygonIntersection(vRayStart, vRayDir, Vertices, 4, &fIntersection, &vIntersection, sizeof(xiiVec3) * 2) == bIntersects);

          if (bIntersects)
          {
            XII_TEST_FLOAT(fIntersection, 3.0f, 0.0001f);
            XII_TEST_VEC3(vIntersection, m.TransformPosition(xiiVec3(x, y, 0.0f)), 0.0001f);
          }
        }
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ClosestPoint_PointLineSegment")
  {
    for (xiiUInt32 i = 0; i < 100; ++i)
    {
      xiiMat4 m;
      m = xiiMat4::MakeAxisRotation(xiiVec3(i + 1.0f, i * 3.0f, i * 7.0f).GetNormalized(), xiiAngle::MakeFromDegree((float)i));
      m.SetTranslationVector(xiiVec3((float)i, i * 2.0f, i * 3.0f));

      xiiVec3 vSegment0 = m.TransformPosition(xiiVec3(-10, 1, 2));
      xiiVec3 vSegment1 = m.TransformPosition(xiiVec3(10, 1, 2));

      for (float f = -20; f <= -10; f += 0.5f)
      {
        const xiiVec3 vPos = m.TransformPosition(xiiVec3(f, 10.0f, 20.0f));

        float         fFraction = -1.0f;
        const xiiVec3 vClosest  = xiiIntersectionUtils::ClosestPoint_PointLineSegment(vPos, vSegment0, vSegment1, &fFraction);

        XII_TEST_FLOAT(fFraction, 0.0f, 0.0001f);
        XII_TEST_VEC3(vClosest, vSegment0, 0.0001f);
      }

      for (float f = -10; f <= 10; f += 0.5f)
      {
        const xiiVec3 vPos = m.TransformPosition(xiiVec3(f, 10.0f, 20.0f));

        float         fFraction = -1.0f;
        const xiiVec3 vClosest  = xiiIntersectionUtils::ClosestPoint_PointLineSegment(vPos, vSegment0, vSegment1, &fFraction);

        XII_TEST_FLOAT(fFraction, (f + 10.0f) / 20.0f, 0.0001f);
        XII_TEST_VEC3(vClosest, m.TransformPosition(xiiVec3(f, 1, 2)), 0.0001f);
      }

      for (float f = 10; f <= 20; f += 0.5f)
      {
        const xiiVec3 vPos = m.TransformPosition(xiiVec3(f, 10.0f, 20.0f));

        float         fFraction = -1.0f;
        const xiiVec3 vClosest  = xiiIntersectionUtils::ClosestPoint_PointLineSegment(vPos, vSegment0, vSegment1, &fFraction);

        XII_TEST_FLOAT(fFraction, 1.0f, 0.0001f);
        XII_TEST_VEC3(vClosest, vSegment1, 0.0001f);
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Ray2DLine2D")
  {
    for (xiiUInt32 i = 0; i < 100; ++i)
    {
      xiiMat4 m;
      m = xiiMat4::MakeRotationZ(xiiAngle::MakeFromDegree((float)i));
      m.SetTranslationVector(xiiVec3((float)i, i * 2.0f, i * 3.0f));

      const xiiVec2 vSegment0   = m.TransformPosition(xiiVec3(23, 42, 0)).GetAsVec2();
      const xiiVec2 vSegmentDir = m.TransformDirection(xiiVec3(13, 15, 0)).GetAsVec2();

      const xiiVec2 vSegment1 = vSegment0 + vSegmentDir;

      for (float f = -1.1f; f < 2.0f; f += 0.2f)
      {
        const bool    bIntersection = (f >= 0.0f && f <= 1.0f);
        const xiiVec2 vSegmentPos   = vSegment0 + f * vSegmentDir;

        const xiiVec2 vRayDir   = xiiVec2(2.0f, f);
        const xiiVec2 vRayStart = vSegmentPos - vRayDir * 5.0f;

        float   fIntersection;
        xiiVec2 vIntersection;
        XII_TEST_BOOL(xiiIntersectionUtils::Ray2DLine2D(vRayStart, vRayDir, vSegment0, vSegment1, &fIntersection, &vIntersection) == bIntersection);

        if (bIntersection)
        {
          XII_TEST_FLOAT(fIntersection, 5.0f, 0.0001f);
          XII_TEST_VEC2(vIntersection, vSegmentPos, 0.0001f);
        }
      };
    }
  }
}
