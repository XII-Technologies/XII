#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Strings/StringUtils.h>
#include <FoundationTest/IO/JSONTestHelpers.h>

static xiiVariant CreateVariant(xiiVariant::Type::Enum t, const void* pData);

XII_CREATE_SIMPLE_TEST(IO, DdlUtils)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToColor")
  {
    const char* szTestData = "\
Color $c1 { float { 1, 0, 0.5 } }\
Color $c2 { float { 2, 1, 1.5, 0.1 } }\
Color $c3 { unsigned_int8 { 128, 2, 32 } }\
Color $c4 { unsigned_int8 { 128, 0, 32, 64 } }\
float $c5 { 1, 0, 0.5 }\
float $c6 { 2, 1, 1.5, 0.1 }\
unsigned_int8 $c7 { 128, 2, 32 }\
unsigned_int8 $c8 { 128, 0, 32, 64 }\
Color $c9 { float { 1, 0 } }\
Color $c10 { float { 1, 0, 3, 4, 5 } }\
Color $c11 { float { } }\
Color $c12 { }\
Color $c13 { double { 1, 1, 1, 2 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiColor c1, c2, c3, c4, c5, c6, c7, c8, c0;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("t0"), c0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("c1"), c1).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("c2"), c2).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("c3"), c3).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("c4"), c4).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("c5"), c5).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("c6"), c6).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("c7"), c7).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("c8"), c8).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("c9"), c0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("c10"), c0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("c11"), c0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("c12"), c0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColor(doc.FindElement("c13"), c0).Failed());

    XII_TEST_BOOL(c1 == xiiColor(1, 0, 0.5f, 1.0f));
    XII_TEST_BOOL(c2 == xiiColor(2, 1, 1.5f, 0.1f));
    XII_TEST_BOOL(c3 == xiiColorGammaUB(128, 2, 32));
    XII_TEST_BOOL(c4 == xiiColorGammaUB(128, 0, 32, 64));
    XII_TEST_BOOL(c5 == xiiColor(1, 0, 0.5f, 1.0f));
    XII_TEST_BOOL(c6 == xiiColor(2, 1, 1.5f, 0.1f));
    XII_TEST_BOOL(c7 == xiiColorGammaUB(128, 2, 32));
    XII_TEST_BOOL(c8 == xiiColorGammaUB(128, 0, 32, 64));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToColorGamma")
  {
    const char* szTestData = "\
Color $c1 { float { 1, 0, 0.5 } }\
Color $c2 { float { 2, 1, 1.5, 0.1 } }\
Color $c3 { unsigned_int8 { 128, 2, 32 } }\
Color $c4 { unsigned_int8 { 128, 0, 32, 64 } }\
float $c5 { 1, 0, 0.5 }\
float $c6 { 2, 1, 1.5, 0.1 }\
unsigned_int8 $c7 { 128, 2, 32 }\
unsigned_int8 $c8 { 128, 0, 32, 64 }\
Color $c9 { float { 1, 0 } }\
Color $c10 { float { 1, 0, 3, 4, 5 } }\
Color $c11 { float { } }\
Color $c12 { }\
Color $c13 { double { 1, 1, 1, 2 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiColorGammaUB c1, c2, c3, c4, c5, c6, c7, c8, c0;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("t0"), c0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c1"), c1).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c2"), c2).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c3"), c3).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c4"), c4).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c5"), c5).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c6"), c6).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c7"), c7).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c8"), c8).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c9"), c0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c10"), c0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c11"), c0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c12"), c0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToColorGamma(doc.FindElement("c13"), c0).Failed());

    XII_TEST_BOOL(c1 == xiiColorGammaUB(xiiColor(1, 0, 0.5f, 1.0f)));
    XII_TEST_BOOL(c2 == xiiColorGammaUB(xiiColor(2, 1, 1.5f, 0.1f)));
    XII_TEST_BOOL(c3 == xiiColorGammaUB(128, 2, 32));
    XII_TEST_BOOL(c4 == xiiColorGammaUB(128, 0, 32, 64));
    XII_TEST_BOOL(c5 == xiiColorGammaUB(xiiColor(1, 0, 0.5f, 1.0f)));
    XII_TEST_BOOL(c6 == xiiColorGammaUB(xiiColor(2, 1, 1.5f, 0.1f)));
    XII_TEST_BOOL(c7 == xiiColorGammaUB(128, 2, 32));
    XII_TEST_BOOL(c8 == xiiColorGammaUB(128, 0, 32, 64));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToTime")
  {
    const char* szTestData = "\
Time $t1 { float { 0.1 } }\
Time $t2 { double { 0.2 } }\
float $t3 { 0.3 }\
double $t4 { 0.4 }\
Time $t5 { double { 0.2, 2 } }\
Time $t6 { int8 { 0, 2 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiTime t1, t2, t3, t4, t0;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToTime(doc.FindElement("t0"), t0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToTime(doc.FindElement("t1"), t1).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToTime(doc.FindElement("t2"), t2).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToTime(doc.FindElement("t3"), t3).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToTime(doc.FindElement("t4"), t4).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToTime(doc.FindElement("t5"), t0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToTime(doc.FindElement("t6"), t0).Failed());

    XII_TEST_FLOAT(t1.GetSeconds(), 0.1, 0.0001f);
    XII_TEST_FLOAT(t2.GetSeconds(), 0.2, 0.0001f);
    XII_TEST_FLOAT(t3.GetSeconds(), 0.3, 0.0001f);
    XII_TEST_FLOAT(t4.GetSeconds(), 0.4, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToVec2")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2 } }\
float $v2 { 0.3, 3 }\
Vector $v3 { float { 0.1 } }\
Vector $v4 { float { 0.1, 2.2, 3.33 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiVec2 v0, v1, v2;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec2(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec2(doc.FindElement("v1"), v1).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec2(doc.FindElement("v2"), v2).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec2(doc.FindElement("v3"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec2(doc.FindElement("v4"), v0).Failed());

    XII_TEST_VEC2(v1, xiiVec2(0.1f, 2.0f), 0.0001f);
    XII_TEST_VEC2(v2, xiiVec2(0.3f, 3.0f), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToVec2d")
  {
    const char* szTestData = "\
Vector $v1 { double { 0.1, 2 } }\
double $v2 { 0.3, 3 }\
Vector $v3 { double { 0.1 } }\
Vector $v4 { double { 0.1, 2.2, 3.33 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiVec2d v0, v1, v2;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec2d(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec2d(doc.FindElement("v1"), v1).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec2d(doc.FindElement("v2"), v2).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec2d(doc.FindElement("v3"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec2d(doc.FindElement("v4"), v0).Failed());

    XII_TEST_VEC2(v1, xiiVec2d(0.1, 2.0), 0.0001);
    XII_TEST_VEC2(v2, xiiVec2d(0.3, 3.0), 0.0001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToVec3")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2, 3.2 } }\
float $v2 { 0.3, 3,0}\
Vector $v3 { float { 0.1,2 } }\
Vector $v4 { float { 0.1, 2.2, 3.33,44 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiVec3 v0, v1, v2;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec3(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec3(doc.FindElement("v1"), v1).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec3(doc.FindElement("v2"), v2).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec3(doc.FindElement("v3"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec3(doc.FindElement("v4"), v0).Failed());

    XII_TEST_VEC3(v1, xiiVec3(0.1f, 2.0f, 3.2f), 0.0001f);
    XII_TEST_VEC3(v2, xiiVec3(0.3f, 3.0f, 0.0f), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToVec3d")
  {
    const char* szTestData = "\
Vector $v1 { double { 0.1, 2, 3.2 } }\
double $v2 { 0.3, 3,0}\
Vector $v3 { double { 0.1,2 } }\
Vector $v4 { double { 0.1, 2.2, 3.33,44 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiVec3d v0, v1, v2;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec3d(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec3d(doc.FindElement("v1"), v1).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec3d(doc.FindElement("v2"), v2).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec3d(doc.FindElement("v3"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec3d(doc.FindElement("v4"), v0).Failed());

    XII_TEST_VEC3(v1, xiiVec3d(0.1, 2.0, 3.2), 0.0001);
    XII_TEST_VEC3(v2, xiiVec3d(0.3, 3.0, 0.0), 0.0001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToVec4")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2, 3.2, 44.5 } }\
float $v2 { 0.3, 3,0, 12.}\
Vector $v3 { float { 0.1,2 } }\
Vector $v4 { float { 0.1, 2.2, 3.33, 44, 67 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiVec4 v0, v1, v2;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec4(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec4(doc.FindElement("v1"), v1).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec4(doc.FindElement("v2"), v2).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec4(doc.FindElement("v3"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec4(doc.FindElement("v4"), v0).Failed());

    XII_TEST_VEC4(v1, xiiVec4(0.1f, 2.0f, 3.2f, 44.5f), 0.0001f);
    XII_TEST_VEC4(v2, xiiVec4(0.3f, 3.0f, 0.0f, 12.0f), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToVec4d")
  {
    const char* szTestData = "\
Vector $v1 { double { 0.1, 2, 3.2, 44.5 } }\
double $v2 { 0.3, 3,0, 12.}\
Vector $v3 { double { 0.1,2 } }\
Vector $v4 { double { 0.1, 2.2, 3.33, 44, 67 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiVec4d v0, v1, v2;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec4d(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec4d(doc.FindElement("v1"), v1).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec4d(doc.FindElement("v2"), v2).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec4d(doc.FindElement("v3"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVec4d(doc.FindElement("v4"), v0).Failed());

    XII_TEST_VEC4(v1, xiiVec4d(0.1, 2.0, 3.2, 44.5), 0.0001);
    XII_TEST_VEC4(v2, xiiVec4d(0.3, 3.0, 0.0, 12.0), 0.0001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToMat3")
  {
    const char* szTestData = "\
Group $v1 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiMat3 v0, v1;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToMat3(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToMat3(doc.FindElement("v1"), v1).Succeeded());

    XII_TEST_BOOL(v1.IsEqual(xiiMat3(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToMat3d")
  {
    const char* szTestData = "\
Group $v1 { double { 1, 2, 3, 4, 5, 6, 7, 8, 9 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiMat3d v0, v1;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToMat3d(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToMat3d(doc.FindElement("v1"), v1).Succeeded());

    XII_TEST_BOOL(v1.IsEqual(xiiMat3d(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToMat4")
  {
    const char* szTestData = "\
Group $v1 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiMat4 v0, v1;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToMat4(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToMat4(doc.FindElement("v1"), v1).Succeeded());

    XII_TEST_BOOL(v1.IsEqual(xiiMat4(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToMat4d")
  {
    const char* szTestData = "\
Group $v1 { double { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiMat4d v0, v1;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToMat4d(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToMat4d(doc.FindElement("v1"), v1).Succeeded());

    XII_TEST_BOOL(v1.IsEqual(xiiMat4d(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16), 0.0001f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToTransform")
  {
    const char* szTestData = "\
Group $v1 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiTransform v0, v1;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToTransform(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToTransform(doc.FindElement("v1"), v1).Succeeded());

    XII_TEST_VEC3(v1.m_vPosition, xiiVec3(1, 2, 3), 0.0001f);
    XII_TEST_BOOL(v1.m_qRotation == xiiQuat(4, 5, 6, 7));
    XII_TEST_VEC3(v1.m_vScale, xiiVec3(8, 9, 10), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToTransformd")
  {
    const char* szTestData = "\
Group $v1 { double { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiTransformd v0, v1;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToTransformd(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToTransformd(doc.FindElement("v1"), v1).Succeeded());

    XII_TEST_VEC3(v1.m_vPosition, xiiVec3d(1, 2, 3), 0.0001f);
    XII_TEST_BOOL(v1.m_qRotation == xiiQuatd(4, 5, 6, 7));
    XII_TEST_VEC3(v1.m_vScale, xiiVec3d(8, 9, 10), 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToQuat")
  {
    const char* szTestData = "\
Vector $v1 { float { 0.1, 2, 3.2, 44.5 } }\
float $v2 { 0.3, 3,0, 12.}\
Vector $v3 { float { 0.1,2 } }\
Vector $v4 { float { 0.1, 2.2, 3.33, 44, 67 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiQuat v0, v1, v2;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToQuat(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToQuat(doc.FindElement("v1"), v1).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToQuat(doc.FindElement("v2"), v2).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToQuat(doc.FindElement("v3"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToQuat(doc.FindElement("v4"), v0).Failed());

    XII_TEST_BOOL(v1 == xiiQuat(0.1f, 2.0f, 3.2f, 44.5f));
    XII_TEST_BOOL(v2 == xiiQuat(0.3f, 3.0f, 0.0f, 12.0f));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToQuatd")
  {
    const char* szTestData = "\
Vector $v1 { double { 0.1, 2, 3.2, 44.5 } }\
double $v2 { 0.3, 3,0, 12.}\
Vector $v3 { double { 0.1,2 } }\
Vector $v4 { double { 0.1, 2.2, 3.33, 44, 67 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiQuatd v0, v1, v2;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToQuatd(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToQuatd(doc.FindElement("v1"), v1).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToQuatd(doc.FindElement("v2"), v2).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToQuatd(doc.FindElement("v3"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToQuatd(doc.FindElement("v4"), v0).Failed());

    XII_TEST_BOOL(v1 == xiiQuatd(0.1, 2.0, 3.2, 44.5));
    XII_TEST_BOOL(v2 == xiiQuatd(0.3, 3.0, 0.0, 12.0));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToUuid")
  {
    const char* szTestData = "\
Data $v1 { unsigned_int64 { 12345678910, 10987654321 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiUuid v0, v1;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToUuid(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToUuid(doc.FindElement("v1"), v1).Succeeded());

    XII_TEST_BOOL(v1 == xiiUuid(12345678910, 10987654321));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToAngle")
  {
    const char* szTestData = "\
Data $v1 { float { 45.23 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiAngle v0, v1;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToAngle(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToAngle(doc.FindElement("v1"), v1).Succeeded());

    XII_TEST_FLOAT(v1.GetRadian(), 45.23f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToAngled")
  {
    const char* szTestData = "\
Data $v1 { double { 45.22 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiAngled v0, v1;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToAngle(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToAngle(doc.FindElement("v1"), v1).Succeeded());

    XII_TEST_DOUBLE(v1.GetRadian(), 45.22, 0.0001);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToHashedString")
  {
    const char* szTestData = "\
Data $v1 { string { \"Hello World\" } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiHashedString v0, v1;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToHashedString(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToHashedString(doc.FindElement("v1"), v1).Succeeded());

    XII_TEST_STRING(v1.GetView(), "Hello World");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToTempHashedString")
  {
    const char* szTestData = "\
Data $v1 { uint64 { 2720389094277464445 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiTempHashedString v0, v1;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToTempHashedString(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToTempHashedString(doc.FindElement("v1"), v1).Succeeded());

    XII_TEST_BOOL(v1 == xiiTempHashedString("GHIJK"));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiOpenDdlUtils::ConvertToVariant")
  {
    const char* szTestData = "\
Color $v1 { float { 1, 0, 0.5 } }\
ColorGamma $v2 { unsigned_int8 { 128, 0, 32, 64 } }\
Time $v3 { float { 0.1 } }\
Vec2 $v4 { float { 0.1, 2 } }\
Vec2d $v4d { double { 0.1, 2 } }\
Vec3 $v5 { float { 0.1, 2, 3.2 } }\
Vec3d $v5d { double { 0.1, 2, 3.2 } }\
Vec4 $v6 { float { 0.1, 2, 3.2, 44.5 } }\
Vec4d $v6d { double { 0.1, 2, 3.2, 44.5 } }\
Mat3 $v7 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9 } }\
Mat3d $v7d { double { 1, 2, 3, 4, 5, 6, 7, 8, 9 } }\
Mat4 $v8 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } }\
Mat4d $v8d { double { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } }\
Transform $v9 { float { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } }\
Transformd $v9d { double { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 } }\
Quat $v10 { float { 0.1, 2, 3.2, 44.5 } }\
Quatd $v10d { double { 0.1, 2, 3.2, 44.5 } }\
Uuid $v11 { unsigned_int64 { 12345678910, 10987654321 } }\
Angle $v12 { float { 45.23 } }\
Angled $v12d { double { 22.22 } }\
HashedString $v13 { string { \"Soo much string\" } }\
TempHashedString $v14 { uint64 { 2720389094277464445 } }\
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiVariant v[23];

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v0"), v[0]).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v1"), v[1]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v2"), v[2]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v3"), v[3]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v4"), v[4]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v4d"), v[5]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v5"), v[6]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v5d"), v[7]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v6"), v[8]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v6d"), v[9]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v7"), v[10]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v7d"), v[11]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v8"), v[12]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v8d"), v[13]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v9"), v[14]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v9d"), v[15]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v10"), v[16]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v10d"), v[17]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v11"), v[18]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v12"), v[19]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v12d"), v[20]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v13"), v[21]).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v14"), v[22]).Succeeded());

    XII_TEST_BOOL(v[1].IsA<xiiColor>());
    XII_TEST_BOOL(v[2].IsA<xiiColorGammaUB>());
    XII_TEST_BOOL(v[3].IsA<xiiTime>());
    XII_TEST_BOOL(v[4].IsA<xiiVec2>());
    XII_TEST_BOOL(v[5].IsA<xiiVec2d>());
    XII_TEST_BOOL(v[6].IsA<xiiVec3>());
    XII_TEST_BOOL(v[7].IsA<xiiVec3d>());
    XII_TEST_BOOL(v[8].IsA<xiiVec4>());
    XII_TEST_BOOL(v[9].IsA<xiiVec4d>());
    XII_TEST_BOOL(v[10].IsA<xiiMat3>());
    XII_TEST_BOOL(v[11].IsA<xiiMat3d>());
    XII_TEST_BOOL(v[12].IsA<xiiMat4>());
    XII_TEST_BOOL(v[13].IsA<xiiMat4d>());
    XII_TEST_BOOL(v[14].IsA<xiiTransform>());
    XII_TEST_BOOL(v[15].IsA<xiiTransformd>());
    XII_TEST_BOOL(v[16].IsA<xiiQuat>());
    XII_TEST_BOOL(v[17].IsA<xiiQuatd>());
    XII_TEST_BOOL(v[18].IsA<xiiUuid>());
    XII_TEST_BOOL(v[19].IsA<xiiAngle>());
    XII_TEST_BOOL(v[20].IsA<xiiAngled>());
    XII_TEST_BOOL(v[21].IsA<xiiHashedString>());
    XII_TEST_BOOL(v[22].IsA<xiiTempHashedString>());

    XII_TEST_BOOL(v[1].Get<xiiColor>() == xiiColor(1, 0, 0.5));
    XII_TEST_BOOL(v[2].Get<xiiColorGammaUB>() == xiiColorGammaUB(128, 0, 32, 64));
    XII_TEST_DOUBLE(v[3].Get<xiiTime>().GetSeconds(), 0.1, 0.0001f);
    XII_TEST_VEC2(v[4].Get<xiiVec2>(), xiiVec2(0.1f, 2.0f), 0.0001f);
    XII_TEST_VEC2(v[5].Get<xiiVec2d>(), xiiVec2d(0.1, 2.0), 0.0001f);
    XII_TEST_VEC3(v[6].Get<xiiVec3>(), xiiVec3(0.1f, 2.0f, 3.2f), 0.0001f);
    XII_TEST_VEC3(v[7].Get<xiiVec3d>(), xiiVec3d(0.1, 2.0, 3.2), 0.0001f);
    XII_TEST_VEC4(v[8].Get<xiiVec4>(), xiiVec4(0.1f, 2.0f, 3.2f, 44.5f), 0.0001f);
    XII_TEST_VEC4(v[9].Get<xiiVec4d>(), xiiVec4d(0.1, 2.0, 3.2, 44.5), 0.0001f);
    XII_TEST_BOOL(v[10].Get<xiiMat3>().IsEqual(xiiMat3(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
    XII_TEST_BOOL(v[11].Get<xiiMat3d>().IsEqual(xiiMat3d(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
    XII_TEST_BOOL(v[12].Get<xiiMat4>().IsEqual(xiiMat4(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16), 0.0001f));
    XII_TEST_BOOL(v[13].Get<xiiMat4d>().IsEqual(xiiMat4d(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16), 0.0001f));
    XII_TEST_BOOL(v[14].Get<xiiTransform>().m_qRotation == xiiQuat(4, 5, 6, 7));
    XII_TEST_VEC3(v[14].Get<xiiTransform>().m_vPosition, xiiVec3(1, 2, 3), 0.0001f);
    XII_TEST_VEC3(v[14].Get<xiiTransform>().m_vScale, xiiVec3(8, 9, 10), 0.0001f);
    XII_TEST_BOOL(v[15].Get<xiiTransformd>().m_qRotation == xiiQuatd(4, 5, 6, 7));
    XII_TEST_VEC3(v[15].Get<xiiTransformd>().m_vPosition, xiiVec3d(1, 2, 3), 0.0001f);
    XII_TEST_VEC3(v[15].Get<xiiTransformd>().m_vScale, xiiVec3d(8, 9, 10), 0.0001f);
    XII_TEST_BOOL(v[16].Get<xiiQuat>() == xiiQuat(0.1f, 2.0f, 3.2f, 44.5f));
    XII_TEST_BOOL(v[17].Get<xiiQuatd>() == xiiQuatd(0.1, 2.0, 3.2, 44.5));
    XII_TEST_BOOL(v[18].Get<xiiUuid>() == xiiUuid(12345678910, 10987654321));
    XII_TEST_FLOAT(v[19].Get<xiiAngle>().GetRadian(), 45.23f, 0.0001f);
    XII_TEST_DOUBLE(v[20].Get<xiiAngled>().GetRadian(), 22.22, 0.0001);
    XII_TEST_STRING(v[21].Get<xiiHashedString>().GetView(), "Soo much string");
    XII_TEST_BOOL(v[22].Get<xiiTempHashedString>() == xiiTempHashedString("GHIJK"));

    /// \test Test primitive types in xiiVariant
  }

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreColor")
  {
    StreamComparer sc("Color $v1{float{1,2,3,4}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreColor(js, xiiColor(1, 2, 3, 4), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreColorGamma")
  {
    StreamComparer sc("ColorGamma $v1{uint8{1,2,3,4}}\n");

    xiiOpenDdlWriter js;
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreColorGamma(js, xiiColorGammaUB(1, 2, 3, 4), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreTime")
  {
    StreamComparer sc("Time $v1{double{2.3}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreTime(js, xiiTime::MakeFromSeconds(2.3), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreVec2")
  {
    StreamComparer sc("Vec2 $v1{float{1,2}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreVec2(js, xiiVec2(1, 2), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreVec2d")
  {
    StreamComparer sc("Vec2d $v1{double{1,2}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreVec2d(js, xiiVec2d(1, 2), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreVec3")
  {
    StreamComparer sc("Vec3 $v1{float{1,2,3}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreVec3(js, xiiVec3(1, 2, 3), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreVec3d")
  {
    StreamComparer sc("Vec3d $v1{double{1,2,3}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreVec3d(js, xiiVec3d(1, 2, 3), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreVec4")
  {
    StreamComparer sc("Vec4 $v1{float{1,2,3,4}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreVec4(js, xiiVec4(1, 2, 3, 4), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreVec4d")
  {
    StreamComparer sc("Vec4d $v1{double{1,2,3,4}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreVec4d(js, xiiVec4d(1, 2, 3, 4), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreMat3")
  {
    StreamComparer sc("Mat3 $v1{float{1,4,7,2,5,8,3,6,9}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreMat3(js, xiiMat3(1, 2, 3, 4, 5, 6, 7, 8, 9), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreMat3d")
  {
    StreamComparer sc("Mat3d $v1{double{1,4,7,2,5,8,3,6,9}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreMat3d(js, xiiMat3d(1, 2, 3, 4, 5, 6, 7, 8, 9), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreMat4")
  {
    StreamComparer sc("Mat4 $v1{float{1,5,9,13,2,6,10,14,3,7,11,15,4,8,12,16}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreMat4(js, xiiMat4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreMat4d")
  {
    StreamComparer sc("Mat4d $v1{double{1,5,9,13,2,6,10,14,3,7,11,15,4,8,12,16}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreMat4d(js, xiiMat4d(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreTransform")
  {
    StreamComparer sc("Transform $v1{float{1,4,7,2,5,8,3,6,9,10}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreTransform(js, xiiTransform(xiiVec3(1, 4, 7), xiiQuat(2, 5, 8, 3), xiiVec3(6, 9, 10)), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreTransformd")
  {
    StreamComparer sc("Transformd $v1{double{1,4,7,2,5,8,3,6,9,10}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreTransformd(js, xiiTransformd(xiiVec3d(1, 4, 7), xiiQuatd(2, 5, 8, 3), xiiVec3d(6, 9, 10)), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreQuat")
  {
    StreamComparer sc("Quat $v1{float{1,2,3,4}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreQuat(js, xiiQuat(1, 2, 3, 4), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreQuatd")
  {
    StreamComparer sc("Quatd $v1{double{1,2,3,4}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreQuatd(js, xiiQuatd(1, 2, 3, 4), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreUuid")
  {
    StreamComparer sc("Uuid $v1{u4{12345678910,10987654321}}\n");

    xiiOpenDdlWriter js;
    js.SetPrimitiveTypeStringMode(xiiOpenDdlWriter::TypeStringMode::Shortest);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreUuid(js, xiiUuid(12345678910, 10987654321), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreAngle")
  {
    StreamComparer sc("Angle $v1{float{2.3}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreAngle(js, xiiAngle::MakeFromRadian(2.3f), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreAngled")
  {
    StreamComparer sc("Angled $v1{double{2.2}}\n");

    xiiOpenDdlWriter js;
    js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Readable);
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreAngle(js, xiiAngled::MakeFromRadian(2.2), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreHashedString")
  {
    StreamComparer sc("HashedString $v1{string{\"ABCDE\"}}\n");

    xiiOpenDdlWriter js;
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreHashedString(js, xiiMakeHashedString("ABCDE"), "v1", true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreTempHashedString")
  {
    StreamComparer sc("TempHashedString $v1{uint64{2720389094277464445}}\n");

    xiiOpenDdlWriter js;
    js.SetOutputStream(&sc);

    xiiOpenDdlUtils::StoreTempHashedString(js, xiiTempHashedString("GHIJK"), "v1", true);
  }

  // This test also covers all the types that Variant supports.
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreVariant")
  {
    alignas(alignof(float)) xiiUInt8 rawData[sizeof(float) * 16]; // Enough for mat4

    for (xiiUInt8 i = 0; i < XII_ARRAY_SIZE(rawData); ++i)
    {
      rawData[i] = i + 33;
    }

    rawData[XII_ARRAY_SIZE(rawData) - 1] = 0; // string terminator

    for (xiiUInt32 t = xiiVariant::Type::FirstStandardType + 1; t < xiiVariant::Type::LastStandardType; ++t)
    {
      const xiiVariant var = CreateVariant((xiiVariant::Type::Enum)t, rawData);

      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         writer(&storage);
      xiiMemoryStreamReader         reader(&storage);

      xiiOpenDdlWriter js;
      js.SetFloatPrecisionMode(xiiOpenDdlWriter::FloatPrecisionMode::Exact);
      js.SetOutputStream(&writer);

      xiiOpenDdlUtils::StoreVariant(js, var, "bla");

      xiiOpenDdlReader doc;
      XII_TEST_BOOL(doc.ParseDocument(reader).Succeeded());

      const auto pVarElem = doc.GetRootElement()->FindChild("bla");

      xiiVariant result;
      xiiOpenDdlUtils::ConvertToVariant(pVarElem, result).IgnoreResult();

      XII_TEST_BOOL(var == result);
    }
  }
}

static xiiVariant CreateVariant(xiiVariant::Type::Enum t, const void* pData)
{
  switch (t)
  {
    case xiiVariant::Type::Bool:
      return xiiVariant(*(xiiInt8*)pData != 0);
    case xiiVariant::Type::Int8:
      return xiiVariant(*((xiiInt8*)pData));
    case xiiVariant::Type::UInt8:
      return xiiVariant(*((xiiUInt8*)pData));
    case xiiVariant::Type::Int16:
      return xiiVariant(*((xiiInt16*)pData));
    case xiiVariant::Type::UInt16:
      return xiiVariant(*((xiiUInt16*)pData));
    case xiiVariant::Type::Int32:
      return xiiVariant(*((xiiInt32*)pData));
    case xiiVariant::Type::UInt32:
      return xiiVariant(*((xiiUInt32*)pData));
    case xiiVariant::Type::Int64:
      return xiiVariant(*((xiiInt64*)pData));
    case xiiVariant::Type::UInt64:
      return xiiVariant(*((xiiUInt64*)pData));
    case xiiVariant::Type::Float:
      return xiiVariant(*((float*)pData));
    case xiiVariant::Type::Double:
      return xiiVariant(*((double*)pData));
    case xiiVariant::Type::Color:
      return xiiVariant(*((xiiColor*)pData));
    case xiiVariant::Type::Vector2:
      return xiiVariant(*((xiiVec2*)pData));
    case xiiVariant::Type::Vector2d:
      return xiiVariant(*((xiiVec2d*)pData));
    case xiiVariant::Type::Vector3:
      return xiiVariant(*((xiiVec3*)pData));
    case xiiVariant::Type::Vector3d:
      return xiiVariant(*((xiiVec3d*)pData));
    case xiiVariant::Type::Vector4:
      return xiiVariant(*((xiiVec4*)pData));
    case xiiVariant::Type::Vector4d:
      return xiiVariant(*((xiiVec4d*)pData));
    case xiiVariant::Type::Vector2I:
      return xiiVariant(*((xiiVec2I32*)pData));
    case xiiVariant::Type::Vector2I64:
      return xiiVariant(*((xiiVec2I64*)pData));
    case xiiVariant::Type::Vector3I:
      return xiiVariant(*((xiiVec3I32*)pData));
    case xiiVariant::Type::Vector3I64:
      return xiiVariant(*((xiiVec3I64*)pData));
    case xiiVariant::Type::Vector4I:
      return xiiVariant(*((xiiVec4I32*)pData));
    case xiiVariant::Type::Vector4I64:
      return xiiVariant(*((xiiVec4I64*)pData));
    case xiiVariant::Type::Vector2U:
      return xiiVariant(*((xiiVec2U32*)pData));
    case xiiVariant::Type::Vector2U64:
      return xiiVariant(*((xiiVec2U64*)pData));
    case xiiVariant::Type::Vector3U:
      return xiiVariant(*((xiiVec3U32*)pData));
    case xiiVariant::Type::Vector3U64:
      return xiiVariant(*((xiiVec3U64*)pData));
    case xiiVariant::Type::Vector4U:
      return xiiVariant(*((xiiVec4U32*)pData));
    case xiiVariant::Type::Vector4U64:
      return xiiVariant(*((xiiVec4U64*)pData));
    case xiiVariant::Type::Quaternion:
      return xiiVariant(*((xiiQuat*)pData));
    case xiiVariant::Type::Quaterniond:
      return xiiVariant(*((xiiQuatd*)pData));
    case xiiVariant::Type::Matrix3:
      return xiiVariant(*((xiiMat3*)pData));
    case xiiVariant::Type::Matrix3d:
      return xiiVariant(*((xiiMat3d*)pData));
    case xiiVariant::Type::Matrix4:
      return xiiVariant(*((xiiMat4*)pData));
    case xiiVariant::Type::Matrix4d:
      return xiiVariant(*((xiiMat4d*)pData));
    case xiiVariant::Type::Transform:
      return xiiVariant(*((xiiTransform*)pData));
    case xiiVariant::Type::Transformd:
      return xiiVariant(*((xiiTransformd*)pData));
    case xiiVariant::Type::String:
    case xiiVariant::Type::StringView: // String Views are stored as full strings as well
      return xiiVariant((const char*)pData);
    case xiiVariant::Type::HashedString:
    {
      xiiHashedString s;
      s.Assign((const char*)pData);
      return xiiVariant(s);
    }
    case xiiVariant::Type::TempHashedString:
      return xiiVariant(xiiTempHashedString((const char*)pData));
    case xiiVariant::Type::DataBuffer:
    {
      xiiDataBuffer db;
      db.SetCountUninitialized(sizeof(float) * 16);
      for (xiiUInt32 i = 0; i < db.GetCount(); ++i)
        db[i] = ((xiiUInt8*)pData)[i];

      return xiiVariant(db);
    }
    case xiiVariant::Type::Time:
      return xiiVariant(*((xiiTime*)pData));
    case xiiVariant::Type::Uuid:
      return xiiVariant(*((xiiUuid*)pData));
    case xiiVariant::Type::Angle:
      return xiiVariant(*((xiiAngle*)pData));
    case xiiVariant::Type::Angled:
      return xiiVariant(*((xiiAngled*)pData));
    case xiiVariant::Type::ColorGamma:
      return xiiVariant(*((xiiColorGammaUB*)pData));

    default:
      XII_REPORT_FAILURE("Unknown type");
  }

  return xiiVariant();
}
