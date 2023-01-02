#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Strings/StringUtils.h>
#include <FoundationTest/IO/JSONTestHelpers.h>

static xiiVariant CreateVariant(xiiVariant::Type::Enum t, const void* data);

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
";

    StringStream     stream(szTestData);
    xiiOpenDdlReader doc;
    XII_TEST_BOOL(doc.ParseDocument(stream).Succeeded());

    xiiVariant v0, v1, v2, v3, v4, v4d, v5, v5d, v6, v6d, v7, v7d, v8, v8d, v9, v9d, v10, v10d, v11, v12;

    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v0"), v0).Failed());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v1"), v1).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v2"), v2).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v3"), v3).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v4"), v4).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v4d"), v4d).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v5"), v5).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v5d"), v5d).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v6"), v6).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v6d"), v6d).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v7"), v7).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v7d"), v7d).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v8"), v8).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v8d"), v8d).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v9"), v9).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v9d"), v9d).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v10"), v10).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v10d"), v10d).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v11"), v11).Succeeded());
    XII_TEST_BOOL(xiiOpenDdlUtils::ConvertToVariant(doc.FindElement("v12"), v12).Succeeded());

    XII_TEST_BOOL(v1.IsA<xiiColor>());
    XII_TEST_BOOL(v2.IsA<xiiColorGammaUB>());
    XII_TEST_BOOL(v3.IsA<xiiTime>());
    XII_TEST_BOOL(v4.IsA<xiiVec2>());
    XII_TEST_BOOL(v4d.IsA<xiiVec2d>());
    XII_TEST_BOOL(v5.IsA<xiiVec3>());
    XII_TEST_BOOL(v5d.IsA<xiiVec3d>());
    XII_TEST_BOOL(v6.IsA<xiiVec4>());
    XII_TEST_BOOL(v6d.IsA<xiiVec4d>());
    XII_TEST_BOOL(v7.IsA<xiiMat3>());
    XII_TEST_BOOL(v7d.IsA<xiiMat3d>());
    XII_TEST_BOOL(v8.IsA<xiiMat4>());
    XII_TEST_BOOL(v8d.IsA<xiiMat4d>());
    XII_TEST_BOOL(v9.IsA<xiiTransform>());
    XII_TEST_BOOL(v9d.IsA<xiiTransformd>());
    XII_TEST_BOOL(v10.IsA<xiiQuat>());
    XII_TEST_BOOL(v10d.IsA<xiiQuatd>());
    XII_TEST_BOOL(v11.IsA<xiiUuid>());
    XII_TEST_BOOL(v12.IsA<xiiAngle>());

    XII_TEST_BOOL(v1.Get<xiiColor>() == xiiColor(1, 0, 0.5));
    XII_TEST_BOOL(v2.Get<xiiColorGammaUB>() == xiiColorGammaUB(128, 0, 32, 64));
    XII_TEST_FLOAT(v3.Get<xiiTime>().GetSeconds(), 0.1, 0.0001f);
    XII_TEST_VEC2(v4.Get<xiiVec2>(), xiiVec2(0.1f, 2.0f), 0.0001f);
    XII_TEST_VEC2(v4d.Get<xiiVec2d>(), xiiVec2d(0.1, 2.0), 0.0001f);
    XII_TEST_VEC3(v5.Get<xiiVec3>(), xiiVec3(0.1f, 2.0f, 3.2f), 0.0001f);
    XII_TEST_VEC3(v5d.Get<xiiVec3d>(), xiiVec3d(0.1, 2.0, 3.2), 0.0001f);
    XII_TEST_VEC4(v6.Get<xiiVec4>(), xiiVec4(0.1f, 2.0f, 3.2f, 44.5f), 0.0001f);
    XII_TEST_VEC4(v6d.Get<xiiVec4d>(), xiiVec4d(0.1, 2.0, 3.2, 44.5), 0.0001f);
    XII_TEST_BOOL(v7.Get<xiiMat3>().IsEqual(xiiMat3(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
    XII_TEST_BOOL(v7d.Get<xiiMat3d>().IsEqual(xiiMat3d(1, 4, 7, 2, 5, 8, 3, 6, 9), 0.0001f));
    XII_TEST_BOOL(v8.Get<xiiMat4>().IsEqual(xiiMat4(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16), 0.0001f));
    XII_TEST_BOOL(v8d.Get<xiiMat4d>().IsEqual(xiiMat4d(1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16), 0.0001f));
    XII_TEST_BOOL(v9.Get<xiiTransform>().m_qRotation == xiiQuat(4, 5, 6, 7));
    XII_TEST_VEC3(v9.Get<xiiTransform>().m_vPosition, xiiVec3(1, 2, 3), 0.0001f);
    XII_TEST_VEC3(v9.Get<xiiTransform>().m_vScale, xiiVec3(8, 9, 10), 0.0001f);
    XII_TEST_BOOL(v9d.Get<xiiTransformd>().m_qRotation == xiiQuatd(4, 5, 6, 7));
    XII_TEST_VEC3(v9d.Get<xiiTransformd>().m_vPosition, xiiVec3d(1, 2, 3), 0.0001f);
    XII_TEST_VEC3(v9d.Get<xiiTransformd>().m_vScale, xiiVec3d(8, 9, 10), 0.0001f);
    XII_TEST_BOOL(v10.Get<xiiQuat>() == xiiQuat(0.1f, 2.0f, 3.2f, 44.5f));
    XII_TEST_BOOL(v10d.Get<xiiQuatd>() == xiiQuatd(0.1, 2.0, 3.2, 44.5));
    XII_TEST_BOOL(v11.Get<xiiUuid>() == xiiUuid(12345678910, 10987654321));
    XII_TEST_FLOAT(v12.Get<xiiAngle>().GetRadian(), 45.23f, 0.0001f);


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

    xiiOpenDdlUtils::StoreTime(js, xiiTime::Seconds(2.3), "v1", true);
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

    xiiOpenDdlUtils::StoreAngle(js, xiiAngle::Radian(2.3f), "v1", true);
  }

  // this test also covers all the types that Variant supports
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "StoreVariant")
  {
    alignas(XII_ALIGNMENT_OF(float)) xiiUInt8 rawData[sizeof(float) * 16]; // enough for mat4

    for (xiiUInt8 i = 0; i < XII_ARRAY_SIZE(rawData); ++i)
    {
      rawData[i] = i + 1;
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

static xiiVariant CreateVariant(xiiVariant::Type::Enum t, const void* data)
{
  switch (t)
  {
    case xiiVariant::Type::Bool:
      return xiiVariant(*((bool*)data));
    case xiiVariant::Type::Int8:
      return xiiVariant(*((xiiInt8*)data));
    case xiiVariant::Type::UInt8:
      return xiiVariant(*((xiiUInt8*)data));
    case xiiVariant::Type::Int16:
      return xiiVariant(*((xiiInt16*)data));
    case xiiVariant::Type::UInt16:
      return xiiVariant(*((xiiUInt16*)data));
    case xiiVariant::Type::Int32:
      return xiiVariant(*((xiiInt32*)data));
    case xiiVariant::Type::UInt32:
      return xiiVariant(*((xiiUInt32*)data));
    case xiiVariant::Type::Int64:
      return xiiVariant(*((xiiInt64*)data));
    case xiiVariant::Type::UInt64:
      return xiiVariant(*((xiiUInt64*)data));
    case xiiVariant::Type::Float:
      return xiiVariant(*((float*)data));
    case xiiVariant::Type::Double:
      return xiiVariant(*((double*)data));
    case xiiVariant::Type::Color:
      return xiiVariant(*((xiiColor*)data));
    case xiiVariant::Type::Vector2:
      return xiiVariant(*((xiiVec2*)data));
    case xiiVariant::Type::Vector2d:
      return xiiVariant(*((xiiVec2d*)data));
    case xiiVariant::Type::Vector3:
      return xiiVariant(*((xiiVec3*)data));
    case xiiVariant::Type::Vector3d:
      return xiiVariant(*((xiiVec3d*)data));
    case xiiVariant::Type::Vector4:
      return xiiVariant(*((xiiVec4*)data));
    case xiiVariant::Type::Vector4d:
      return xiiVariant(*((xiiVec4d*)data));
    case xiiVariant::Type::Vector2I:
      return xiiVariant(*((xiiVec2I32*)data));
    case xiiVariant::Type::Vector2I64:
      return xiiVariant(*((xiiVec2I64*)data));
    case xiiVariant::Type::Vector3I:
      return xiiVariant(*((xiiVec3I32*)data));
    case xiiVariant::Type::Vector3I64:
      return xiiVariant(*((xiiVec3I64*)data));
    case xiiVariant::Type::Vector4I:
      return xiiVariant(*((xiiVec4I32*)data));
    case xiiVariant::Type::Vector4I64:
      return xiiVariant(*((xiiVec4I64*)data));
    case xiiVariant::Type::Vector2U:
      return xiiVariant(*((xiiVec2U32*)data));
    case xiiVariant::Type::Vector2U64:
      return xiiVariant(*((xiiVec2U64*)data));
    case xiiVariant::Type::Vector3U:
      return xiiVariant(*((xiiVec3U32*)data));
    case xiiVariant::Type::Vector3U64:
      return xiiVariant(*((xiiVec3U64*)data));
    case xiiVariant::Type::Vector4U:
      return xiiVariant(*((xiiVec4U32*)data));
    case xiiVariant::Type::Vector4U64:
      return xiiVariant(*((xiiVec4U64*)data));
    case xiiVariant::Type::Quaternion:
      return xiiVariant(*((xiiQuat*)data));
    case xiiVariant::Type::Quaterniond:
      return xiiVariant(*((xiiQuatd*)data));
    case xiiVariant::Type::Matrix3:
      return xiiVariant(*((xiiMat3*)data));
    case xiiVariant::Type::Matrix3d:
      return xiiVariant(*((xiiMat3d*)data));
    case xiiVariant::Type::Matrix4:
      return xiiVariant(*((xiiMat4*)data));
    case xiiVariant::Type::Matrix4d:
      return xiiVariant(*((xiiMat4d*)data));
    case xiiVariant::Type::Transform:
      return xiiVariant(*((xiiTransform*)data));
    case xiiVariant::Type::Transformd:
      return xiiVariant(*((xiiTransformd*)data));
    case xiiVariant::Type::String:
    case xiiVariant::Type::StringView: // String Views are stored as full strings as well
      return xiiVariant((const char*)data);
    case xiiVariant::Type::DataBuffer:
    {
      xiiDataBuffer db;
      db.SetCountUninitialized(sizeof(float) * 16);
      for (xiiUInt32 i = 0; i < db.GetCount(); ++i)
        db[i] = ((xiiUInt8*)data)[i];

      return xiiVariant(db);
    }
    case xiiVariant::Type::Time:
      return xiiVariant(*((xiiTime*)data));
    case xiiVariant::Type::Uuid:
      return xiiVariant(*((xiiUuid*)data));
    case xiiVariant::Type::Angle:
      return xiiVariant(*((xiiAngle*)data));
    case xiiVariant::Type::ColorGamma:
      return xiiVariant(*((xiiColorGammaUB*)data));

    default:
      XII_REPORT_FAILURE("Unknown type");
  }

  return xiiVariant();
}
