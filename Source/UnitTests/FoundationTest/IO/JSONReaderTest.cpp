#include <FoundationTest/FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/JSONReader.h>

namespace JSONReaderTestDetail
{

  class StringStream : public xiiStreamReader
  {
  public:
    StringStream(const void* pData)
    {
      m_pData    = pData;
      m_uiLength = xiiStringUtils::GetStringElementCount((const char*)pData);
    }

    virtual xiiUInt64 ReadBytes(void* pReadBuffer, xiiUInt64 uiBytesToRead)
    {
      uiBytesToRead = xiiMath::Min(uiBytesToRead, m_uiLength);
      m_uiLength -= uiBytesToRead;

      if (uiBytesToRead > 0)
      {
        xiiMemoryUtils::Copy((xiiUInt8*)pReadBuffer, (xiiUInt8*)m_pData, (size_t)uiBytesToRead);
        m_pData = xiiMemoryUtils::AddByteOffset(m_pData, (ptrdiff_t)uiBytesToRead);
      }

      return uiBytesToRead;
    }

  private:
    const void* m_pData;
    xiiUInt64   m_uiLength;
  };

  void TraverseTree(const xiiVariant& var, xiiDeque<xiiString>& ref_compare)
  {
    if (ref_compare.IsEmpty())
      return;

    switch (var.GetType())
    {
      case xiiVariant::Type::VariantDictionary:
      {
        // xiiLog::Printf("Expect: %s - Is: %s\n", "<object>", Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), "<object>");
        ref_compare.PopFront();

        const xiiVariantDictionary& vd = var.Get<xiiVariantDictionary>();

        for (auto it = vd.GetIterator(); it.IsValid(); ++it)
        {
          if (ref_compare.IsEmpty())
            return;

          // xiiLog::Printf("Expect: %s - Is: %s\n", it.Key().GetData(), Compare.PeekFront().GetData());
          XII_TEST_STRING(ref_compare.PeekFront().GetData(), it.Key().GetData());
          ref_compare.PopFront();

          TraverseTree(it.Value(), ref_compare);
        }

        if (ref_compare.IsEmpty())
          return;

        XII_TEST_STRING(ref_compare.PeekFront().GetData(), "</object>");
        // xiiLog::Printf("Expect: %s - Is: %s\n", "</object>", Compare.PeekFront().GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::VariantArray:
      {
        // xiiLog::Printf("Expect: %s - Is: %s\n", "<array>", Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), "<array>");
        ref_compare.PopFront();

        const xiiVariantArray& va = var.Get<xiiVariantArray>();

        for (xiiUInt32 i = 0; i < va.GetCount(); ++i)
        {
          TraverseTree(va[i], ref_compare);
        }

        if (ref_compare.IsEmpty())
          return;

        // xiiLog::Printf("Expect: %s - Is: %s\n", "</array>", Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), "</array>");
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Invalid:
        // xiiLog::Printf("Expect: %s - Is: %s\n", "null", Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), "null");
        ref_compare.PopFront();
        break;

      case xiiVariant::Type::Bool:
        // xiiLog::Printf("Expect: %s - Is: %s\n", var.Get<bool>() ? "bool true" : "bool false", Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), var.Get<bool>() ? "bool true" : "bool false");
        ref_compare.PopFront();
        break;

      case xiiVariant::Type::Int8:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("int8 {0}", var.Get<xiiInt8>());
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::UInt8:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("uint8 {0}", var.Get<xiiUInt8>());
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Int16:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("int16 {0}", var.Get<xiiInt16>());
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::UInt16:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("uint16 {0}", var.Get<xiiUInt16>());
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Int32:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("int32 {0}", var.Get<xiiInt32>());
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::UInt32:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("uint32 {0}", var.Get<xiiUInt32>());
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Int64:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("int64 {0}", var.Get<xiiInt64>());
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::UInt64:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("uint64 {0}", var.Get<xiiUInt64>());
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Float:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("float {0}", xiiArgF(var.Get<float>(), 4));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Double:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("double {0}", xiiArgF(var.Get<double>(), 4));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Time:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("time {0}", xiiArgF(var.Get<xiiTime>().GetSeconds(), 4));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Angle:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("angle {0}", xiiArgF(var.Get<xiiAngle>().GetDegree(), 4));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Angled:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("angled {0}", xiiArgF(var.Get<xiiAngled>().GetDegree(), 4));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::String:
      {
        // xiiLog::Printf("Expect: %s - Is: %s\n", var.Get<xiiString>().GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), var.Get<xiiString>().GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::StringView:
      {
        // xiiLog::Printf("Expect: %s - Is: %s\n", var.Get<xiiString>().GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront(), var.Get<xiiStringView>());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Vector2:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("vec2 ({0}, {1})", xiiArgF(var.Get<xiiVec2>().x, 4), xiiArgF(var.Get<xiiVec2>().y, 4));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Vector2d:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("vec2d ({0}, {1})", xiiArgF(var.Get<xiiVec2d>().x, 8), xiiArgF(var.Get<xiiVec2d>().y, 8));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Vector3:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("vec3 ({0}, {1}, {2})", xiiArgF(var.Get<xiiVec3>().x, 4), xiiArgF(var.Get<xiiVec3>().y, 4), xiiArgF(var.Get<xiiVec3>().z, 4));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Vector3d:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("vec3d ({0}, {1}, {2})", xiiArgF(var.Get<xiiVec3d>().x, 8), xiiArgF(var.Get<xiiVec3d>().y, 8), xiiArgF(var.Get<xiiVec3d>().z, 8));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Vector4:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("vec4 ({0}, {1}, {2}, {3})", xiiArgF(var.Get<xiiVec4>().x, 4), xiiArgF(var.Get<xiiVec4>().y, 4), xiiArgF(var.Get<xiiVec4>().z, 4), xiiArgF(var.Get<xiiVec4>().w, 4));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Vector4d:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("vec4d ({0}, {1}, {2}, {3})", xiiArgF(var.Get<xiiVec4d>().x, 8), xiiArgF(var.Get<xiiVec4d>().y, 8), xiiArgF(var.Get<xiiVec4d>().z, 8), xiiArgF(var.Get<xiiVec4d>().w, 8));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Vector2I:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("vec2i ({0}, {1})", var.Get<xiiVec2I32>().x, var.Get<xiiVec2I32>().y);
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Vector3I:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("vec3i ({0}, {1}, {2})", var.Get<xiiVec3I32>().x, var.Get<xiiVec3I32>().y, var.Get<xiiVec3I32>().z);
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Vector4I:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("vec4i ({0}, {1}, {2}, {3})", var.Get<xiiVec4I32>().x, var.Get<xiiVec4I32>().y, var.Get<xiiVec4I32>().z, var.Get<xiiVec4I32>().w);
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Vector2I64:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("vec2i64 ({0}, {1})", var.Get<xiiVec2I64>().x, var.Get<xiiVec2I64>().y);
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Vector3I64:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("vec3i64 ({0}, {1}, {2})", var.Get<xiiVec3I64>().x, var.Get<xiiVec3I64>().y, var.Get<xiiVec3I64>().z);
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Vector4I64:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("vec4i64 ({0}, {1}, {2}, {3})", var.Get<xiiVec4I64>().x, var.Get<xiiVec4I64>().y, var.Get<xiiVec4I64>().z, var.Get<xiiVec4I64>().w);
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Color:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("color ({0}, {1}, {2}, {3})", xiiArgF(var.Get<xiiColor>().r, 4), xiiArgF(var.Get<xiiColor>().g, 4), xiiArgF(var.Get<xiiColor>().b, 4), xiiArgF(var.Get<xiiColor>().a, 4));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::ColorGamma:
      {
        xiiStringBuilder      sTemp;
        const xiiColorGammaUB c = var.ConvertTo<xiiColorGammaUB>();

        sTemp.Format("gamma ({0}, {1}, {2}, {3})", c.r, c.g, c.b, c.a);
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Quaternion:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("quat ({0}, {1}, {2}, {3})", xiiArgF(var.Get<xiiQuat>().v.x, 4), xiiArgF(var.Get<xiiQuat>().v.y, 4), xiiArgF(var.Get<xiiQuat>().v.z, 4), xiiArgF(var.Get<xiiQuat>().w, 4));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Quaterniond:
      {
        xiiStringBuilder sTemp;
        sTemp.Format("quatd ({0}, {1}, {2}, {3})", xiiArgF(var.Get<xiiQuatd>().v.x, 8), xiiArgF(var.Get<xiiQuatd>().v.y, 8), xiiArgF(var.Get<xiiQuatd>().v.z, 8), xiiArgF(var.Get<xiiQuatd>().w, 8));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Matrix3:
      {
        xiiMat3 m = var.Get<xiiMat3>();

        xiiStringBuilder sTemp;
        sTemp.Format("mat3 ({0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8})", xiiArgF(m.m_fElementsCM[0], 4), xiiArgF(m.m_fElementsCM[1], 4), xiiArgF(m.m_fElementsCM[2], 4), xiiArgF(m.m_fElementsCM[3], 4), xiiArgF(m.m_fElementsCM[4], 4), xiiArgF(m.m_fElementsCM[5], 4), xiiArgF(m.m_fElementsCM[6], 4), xiiArgF(m.m_fElementsCM[7], 4), xiiArgF(m.m_fElementsCM[8], 4));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Matrix3d:
      {
        xiiMat3d m = var.Get<xiiMat3d>();

        xiiStringBuilder sTemp;
        sTemp.Format("mat3d ({0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8})", xiiArgF(m.m_fElementsCM[0], 8), xiiArgF(m.m_fElementsCM[1], 8), xiiArgF(m.m_fElementsCM[2], 8), xiiArgF(m.m_fElementsCM[3], 8), xiiArgF(m.m_fElementsCM[4], 8), xiiArgF(m.m_fElementsCM[5], 8), xiiArgF(m.m_fElementsCM[6], 8), xiiArgF(m.m_fElementsCM[7], 8), xiiArgF(m.m_fElementsCM[8], 8));
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Matrix4:
      {
        xiiMat4 m = var.Get<xiiMat4>();

        xiiStringBuilder sTemp;
        sTemp.Printf("mat4 (%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f)", m.m_fElementsCM[0], m.m_fElementsCM[1], m.m_fElementsCM[2], m.m_fElementsCM[3], m.m_fElementsCM[4], m.m_fElementsCM[5], m.m_fElementsCM[6], m.m_fElementsCM[7], m.m_fElementsCM[8], m.m_fElementsCM[9], m.m_fElementsCM[10], m.m_fElementsCM[11], m.m_fElementsCM[12], m.m_fElementsCM[13], m.m_fElementsCM[14], m.m_fElementsCM[15]);
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Matrix4d:
      {
        xiiMat4d m = var.Get<xiiMat4d>();

        xiiStringBuilder sTemp;
        sTemp.Printf("mat4d (%.8f, %.8f, %.8f, %.8f, %.8f, %.8f, %.8f, %.8f, %.8f, %.8f, %.8f, %.8f, %.8f, %.8f, %.8f, %.8f)", m.m_fElementsCM[0], m.m_fElementsCM[1], m.m_fElementsCM[2], m.m_fElementsCM[3], m.m_fElementsCM[4], m.m_fElementsCM[5], m.m_fElementsCM[6], m.m_fElementsCM[7], m.m_fElementsCM[8], m.m_fElementsCM[9], m.m_fElementsCM[10], m.m_fElementsCM[11], m.m_fElementsCM[12], m.m_fElementsCM[13], m.m_fElementsCM[14], m.m_fElementsCM[15]);
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      case xiiVariant::Type::Transform:
      {
        XII_ASSERT_NOT_IMPLEMENTED;
      }
      break;

      case xiiVariant::Type::Transformd:
      {
        XII_ASSERT_NOT_IMPLEMENTED;
      }
      break;

      case xiiVariant::Type::Uuid:
      {
        xiiUuid          uuid = var.Get<xiiUuid>();
        xiiStringBuilder sTemp;
        xiiConversionUtils::ToString(uuid, sTemp);
        sTemp.Prepend("uuid ");
        // xiiLog::Printf("Expect: %s - Is: %s\n", sTemp.GetData(), Compare.PeekFront().GetData());
        XII_TEST_STRING(ref_compare.PeekFront().GetData(), sTemp.GetData());
        ref_compare.PopFront();
      }
      break;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }
} // namespace JSONReaderTestDetail

XII_CREATE_SIMPLE_TEST(IO, JSONReader)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Test")
  {
    xiiStringUtf8 sTD(L"{\n\
\"myarray2\":[\"\",2.2],\n\
\"myarray\" : [1, 2.2, 3.3, false, \"ende\" ],\n\
\"String\"/**/ : \"testvälue\",\n\
\"double\"/***/ : 43.56,//comment\n\
\"float\" :/**//*a*/ 64/*comment*/.720001,\n\
\"bool\" : tr/*asdf*/ue,\n\
\"int\" : 23,\n\
\"MyNüll\" : nu/*asdf*/ll,\n\
\"object\" :\n\
/* totally \n weird \t stuff \n\n\n going on here // thats a line comment \n */ \
// more line comments \n\n\n\n\
{\n\
  \"variable in object\" : \"bla\\\\\\\"\\/\",\n\
    \"Subobject\" :\n\
  {\n\
    \"variable in subobject\" : \"blub\\r\\f\\n\\b\\t\",\n\
      \"array in sub\" : [\n\
    {\n\
      \"obj var\" : 234\n\
            /*stuff ] */ \
    },\n\
    {\n\
      \"obj var 2\" : -235\n//breakingcomment\n\
    }, true, 4, false ]\n\
  }\n\
},\n\
\"test\" : \"text\"\n\
}");
    const char*   szTestData = sTD.GetData();

    // NOTE: The way this test is implemented, it might break, if the HashMap uses another insertion algorithm.
    // xiiVariantDictionary is a xiiHashmap and this test currently relies on one exact order in of the result.
    // If this should ever change (or be arbitrary at runtime), the test needs to be implemented in a more robust way.

    JSONReaderTestDetail::StringStream stream(szTestData);

    xiiJSONReader reader;
    XII_TEST_BOOL(reader.Parse(stream).Succeeded());

    xiiDeque<xiiString> sCompare;
    sCompare.PushBack("<object>");
    sCompare.PushBack("int");
    sCompare.PushBack("double 23.0000");
    sCompare.PushBack("String");
    sCompare.PushBack(xiiStringUtf8(L"testvälue").GetData()); // unicode literal

    sCompare.PushBack("double");
    sCompare.PushBack("double 43.5600");

    sCompare.PushBack("myarray");
    sCompare.PushBack("<array>");
    sCompare.PushBack("double 1.0000");
    sCompare.PushBack("double 2.2000");
    sCompare.PushBack("double 3.3000");
    sCompare.PushBack("bool false");
    sCompare.PushBack("ende");
    sCompare.PushBack("</array>");

    sCompare.PushBack("object");
    sCompare.PushBack("<object>");

    sCompare.PushBack("Subobject");
    sCompare.PushBack("<object>");

    sCompare.PushBack("array in sub");
    sCompare.PushBack("<array>");

    sCompare.PushBack("<object>");
    sCompare.PushBack("obj var");
    sCompare.PushBack("double 234.0000");
    sCompare.PushBack("</object>");

    sCompare.PushBack("<object>");
    sCompare.PushBack("obj var 2");
    sCompare.PushBack("double -235.0000");
    sCompare.PushBack("</object>");

    sCompare.PushBack("bool true");
    sCompare.PushBack("double 4.0000");
    sCompare.PushBack("bool false");

    sCompare.PushBack("</array>");


    sCompare.PushBack("variable in subobject");
    sCompare.PushBack("blub\r\f\n\b\t"); // escaped special characters

    sCompare.PushBack("</object>");

    sCompare.PushBack("variable in object");
    sCompare.PushBack("bla\\\"/"); // escaped backslash, quotation mark, slash

    sCompare.PushBack("</object>");

    sCompare.PushBack("float");
    sCompare.PushBack("double 64.7200");

    sCompare.PushBack("myarray2");
    sCompare.PushBack("<array>");
    sCompare.PushBack("");
    sCompare.PushBack("double 2.2000");
    sCompare.PushBack("</array>");

    sCompare.PushBack(xiiStringUtf8(L"MyNüll").GetData()); // unicode literal
    sCompare.PushBack("null");

    sCompare.PushBack("test");
    sCompare.PushBack("text");

    sCompare.PushBack("bool");
    sCompare.PushBack("bool true");

    sCompare.PushBack("</object>");

    if (XII_TEST_BOOL(reader.GetTopLevelElementType() == xiiJSONReader::ElementType::Dictionary))
    {
      JSONReaderTestDetail::TraverseTree(reader.GetTopLevelObject(), sCompare);

      XII_TEST_BOOL(sCompare.IsEmpty());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Array document")
  {
    const char* szTestData = "[\"a\",\"b\"]";

    // NOTE: The way this test is implemented, it might break, if the HashMap uses another insertion algorithm.
    // xiiVariantDictionary is an xiiHashmap and this test currently relies on one exact order in of the result.
    // If this should ever change (or be arbitrary at runtime), the test needs to be implemented in a more robust way.

    JSONReaderTestDetail::StringStream stream(szTestData);

    xiiJSONReader reader;
    XII_TEST_BOOL(reader.Parse(stream).Succeeded());

    xiiDeque<xiiString> sCompare;
    sCompare.PushBack("<array>");
    sCompare.PushBack("a");
    sCompare.PushBack("b");
    sCompare.PushBack("</array>");

    if (XII_TEST_BOOL(reader.GetTopLevelElementType() == xiiJSONReader::ElementType::Array))
    {
      JSONReaderTestDetail::TraverseTree(reader.GetTopLevelArray(), sCompare);

      XII_TEST_BOOL(sCompare.IsEmpty());
    }
  }
}
