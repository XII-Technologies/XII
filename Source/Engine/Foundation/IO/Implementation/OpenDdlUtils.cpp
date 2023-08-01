#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/RTTI.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Types/VariantTypeRegistry.h>

xiiResult xiiOpenDdlUtils::ConvertToColor(const xiiOpenDdlReaderElement* pElement, xiiColor& out_result)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = pValues[3];

      return XII_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = 1.0f;

      return XII_SUCCESS;
    }
  }
  else if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::UInt8)
  {
    const xiiUInt8* pValues = pElement->GetPrimitivesUInt8();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result = xiiColorGammaUB(pValues[0], pValues[1], pValues[2], pValues[3]);

      return XII_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result = xiiColorGammaUB(pValues[0], pValues[1], pValues[2]);

      return XII_SUCCESS;
    }
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToColorGamma(const xiiOpenDdlReaderElement* pElement, xiiColorGammaUB& out_result)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result = xiiColor(pValues[0], pValues[1], pValues[2], pValues[3]);

      return XII_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result = xiiColor(pValues[0], pValues[1], pValues[2]);

      return XII_SUCCESS;
    }
  }
  else if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::UInt8)
  {
    const xiiUInt8* pValues = pElement->GetPrimitivesUInt8();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = pValues[3];

      return XII_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = 255;

      return XII_SUCCESS;
    }
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToTime(const xiiOpenDdlReaderElement* pElement, xiiTime& out_result)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result = xiiTime::Seconds(pValues[0]);

    return XII_SUCCESS;
  }

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Double)
  {
    const double* pValues = pElement->GetPrimitivesDouble();

    out_result = xiiTime::Seconds(pValues[0]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec2(const xiiOpenDdlReaderElement* pElement, xiiVec2& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_vResult.Set(pValues[0], pValues[1]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec2d(const xiiOpenDdlReaderElement* pElement, xiiVec2d& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Double)
  {
    const double* pValues = pElement->GetPrimitivesDouble();

    out_vResult.Set(pValues[0], pValues[1]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec3(const xiiOpenDdlReaderElement* pElement, xiiVec3& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 3)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_vResult.Set(pValues[0], pValues[1], pValues[2]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec3d(const xiiOpenDdlReaderElement* pElement, xiiVec3d& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 3)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Double)
  {
    const double* pValues = pElement->GetPrimitivesDouble();

    out_vResult.Set(pValues[0], pValues[1], pValues[2]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec4(const xiiOpenDdlReaderElement* pElement, xiiVec4& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_vResult.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec4d(const xiiOpenDdlReaderElement* pElement, xiiVec4d& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Double)
  {
    const double* pValues = pElement->GetPrimitivesDouble();

    out_vResult.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec2I(const xiiOpenDdlReaderElement* pElement, xiiVec2I32& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Int32)
  {
    const xiiInt32* pValues = pElement->GetPrimitivesInt32();

    out_vResult.Set(pValues[0], pValues[1]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec2I64(const xiiOpenDdlReaderElement* pElement, xiiVec2I64& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Int64)
  {
    const xiiInt64* pValues = pElement->GetPrimitivesInt64();

    out_vResult.Set(pValues[0], pValues[1]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec3I(const xiiOpenDdlReaderElement* pElement, xiiVec3I32& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 3)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Int32)
  {
    const xiiInt32* pValues = pElement->GetPrimitivesInt32();

    out_vResult.Set(pValues[0], pValues[1], pValues[2]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec3I64(const xiiOpenDdlReaderElement* pElement, xiiVec3I64& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 3)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Int64)
  {
    const xiiInt64* pValues = pElement->GetPrimitivesInt64();

    out_vResult.Set(pValues[0], pValues[1], pValues[2]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec4I(const xiiOpenDdlReaderElement* pElement, xiiVec4I32& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Int32)
  {
    const xiiInt32* pValues = pElement->GetPrimitivesInt32();

    out_vResult.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec4I64(const xiiOpenDdlReaderElement* pElement, xiiVec4I64& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Int64)
  {
    const xiiInt64* pValues = pElement->GetPrimitivesInt64();

    out_vResult.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec2U(const xiiOpenDdlReaderElement* pElement, xiiVec2U32& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::UInt32)
  {
    const xiiUInt32* pValues = pElement->GetPrimitivesUInt32();

    out_vResult.Set(pValues[0], pValues[1]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec2U64(const xiiOpenDdlReaderElement* pElement, xiiVec2U64& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::UInt64)
  {
    const xiiUInt64* pValues = pElement->GetPrimitivesUInt64();

    out_vResult.Set(pValues[0], pValues[1]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec3U(const xiiOpenDdlReaderElement* pElement, xiiVec3U32& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 3)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::UInt32)
  {
    const xiiUInt32* pValues = pElement->GetPrimitivesUInt32();

    out_vResult.Set(pValues[0], pValues[1], pValues[2]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec3U64(const xiiOpenDdlReaderElement* pElement, xiiVec3U64& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 3)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::UInt64)
  {
    const xiiUInt64* pValues = pElement->GetPrimitivesUInt64();

    out_vResult.Set(pValues[0], pValues[1], pValues[2]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec4U(const xiiOpenDdlReaderElement* pElement, xiiVec4U32& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::UInt32)
  {
    const xiiUInt32* pValues = pElement->GetPrimitivesUInt32();

    out_vResult.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec4U64(const xiiOpenDdlReaderElement* pElement, xiiVec4U64& out_vResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::UInt64)
  {
    const xiiUInt64* pValues = pElement->GetPrimitivesUInt64();

    out_vResult.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToMat3(const xiiOpenDdlReaderElement* pElement, xiiMat3& out_mResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 9)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_mResult.SetFromArray(pValues, xiiMatrixLayout::ColumnMajor);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToMat3d(const xiiOpenDdlReaderElement* pElement, xiiMat3d& out_mResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 9)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Double)
  {
    const double* pValues = pElement->GetPrimitivesDouble();

    out_mResult.SetFromArray(pValues, xiiMatrixLayout::ColumnMajor);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToMat4(const xiiOpenDdlReaderElement* pElement, xiiMat4& out_mResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 16)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_mResult.SetFromArray(pValues, xiiMatrixLayout::ColumnMajor);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToMat4d(const xiiOpenDdlReaderElement* pElement, xiiMat4d& out_mResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 16)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Double)
  {
    const double* pValues = pElement->GetPrimitivesDouble();

    out_mResult.SetFromArray(pValues, xiiMatrixLayout::ColumnMajor);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToTransform(const xiiOpenDdlReaderElement* pElement, xiiTransform& out_result)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 10)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result.m_vPosition.x   = pValues[0];
    out_result.m_vPosition.y   = pValues[1];
    out_result.m_vPosition.z   = pValues[2];
    out_result.m_qRotation.v.x = pValues[3];
    out_result.m_qRotation.v.y = pValues[4];
    out_result.m_qRotation.v.z = pValues[5];
    out_result.m_qRotation.w   = pValues[6];
    out_result.m_vScale.x      = pValues[7];
    out_result.m_vScale.y      = pValues[8];
    out_result.m_vScale.z      = pValues[9];

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToTransformd(const xiiOpenDdlReaderElement* pElement, xiiTransformd& out_result)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 10)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Double)
  {
    const double* pValues = pElement->GetPrimitivesDouble();

    out_result.m_vPosition.x   = pValues[0];
    out_result.m_vPosition.y   = pValues[1];
    out_result.m_vPosition.z   = pValues[2];
    out_result.m_qRotation.v.x = pValues[3];
    out_result.m_qRotation.v.y = pValues[4];
    out_result.m_qRotation.v.z = pValues[5];
    out_result.m_qRotation.w   = pValues[6];
    out_result.m_vScale.x      = pValues[7];
    out_result.m_vScale.y      = pValues[8];
    out_result.m_vScale.z      = pValues[9];

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToQuat(const xiiOpenDdlReaderElement* pElement, xiiQuat& out_qResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_qResult.SetElements(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToQuatd(const xiiOpenDdlReaderElement* pElement, xiiQuatd& out_qResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Double)
  {
    const double* pValues = pElement->GetPrimitivesDouble();

    out_qResult.SetElements(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToUuid(const xiiOpenDdlReaderElement* pElement, xiiUuid& out_result)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::UInt64)
  {
    const xiiUInt64* pValues = pElement->GetPrimitivesUInt64();

    out_result = xiiUuid(pValues[0], pValues[1]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToAngle(const xiiOpenDdlReaderElement* pElement, xiiAngle& out_result)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    // have to use radians to prevent precision loss
    out_result = xiiAngle::Radian(pValues[0]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToAngle(const xiiOpenDdlReaderElement* pElement, xiiAngled& out_result)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::Double)
  {
    const double* pValues = pElement->GetPrimitivesDouble();

    // have to use radians to prevent precision loss
    out_result = xiiAngled::Radian(pValues[0]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToHashedString(const xiiOpenDdlReaderElement* pElement, xiiHashedString& out_sResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::String)
  {
    const xiiStringView* pValues = pElement->GetPrimitivesString();

    out_sResult.Assign(pValues[0]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToTempHashedString(const xiiOpenDdlReaderElement* pElement, xiiTempHashedString& out_sResult)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return XII_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return XII_FAILURE;

  if (pElement->GetPrimitivesType() == xiiOpenDdlPrimitiveType::UInt64)
  {
    const xiiUInt64* pValues = pElement->GetPrimitivesUInt64();

    out_sResult = xiiTempHashedString(pValues[0]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVariant(const xiiOpenDdlReaderElement* pElement, xiiVariant& out_result)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // expect a custom type
  if (pElement->IsCustomType())
  {
    if (pElement->GetCustomType() == "VarArray")
    {
      xiiVariantArray value;
      xiiVariant      varChild;

      /// \test This is just quickly hacked
      /// \todo Store array size for reserving var array length

      for (const xiiOpenDdlReaderElement* pChild = pElement->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
      {
        if (ConvertToVariant(pChild, varChild).Failed())
          return XII_FAILURE;

        value.PushBack(varChild);
      }

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "VarDict")
    {
      xiiVariantDictionary value;
      xiiVariant           varChild;

      /// \test This is just quickly hacked
      /// \todo Store array size for reserving var array length

      for (const xiiOpenDdlReaderElement* pChild = pElement->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
      {
        // no name -> invalid dictionary entry
        if (!pChild->HasName())
          continue;

        if (ConvertToVariant(pChild, varChild).Failed())
          return XII_FAILURE;

        value[pChild->GetName()] = varChild;
      }

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "VarDataBuffer")
    {
      /// \test This is just quickly hacked

      xiiDataBuffer value;

      const xiiOpenDdlReaderElement* pString = pElement->GetFirstChild();

      if (!pString->HasPrimitives(xiiOpenDdlPrimitiveType::String))
        return XII_FAILURE;

      const xiiStringView* pValues = pString->GetPrimitivesString();

      value.SetCountUninitialized(pValues[0].GetElementCount() / 2);
      xiiConversionUtils::ConvertHexToBinary(pValues[0].GetStartPointer(), value.GetData(), value.GetCount());

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Color")
    {
      xiiColor value;
      if (ConvertToColor(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "ColorGamma")
    {
      xiiColorGammaUB value;
      if (ConvertToColorGamma(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Time")
    {
      xiiTime value;
      if (ConvertToTime(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec2")
    {
      xiiVec2 value;
      if (ConvertToVec2(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec2d")
    {
      xiiVec2d value;
      if (ConvertToVec2d(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec3")
    {
      xiiVec3 value;
      if (ConvertToVec3(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec3d")
    {
      xiiVec3d value;
      if (ConvertToVec3d(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec4")
    {
      xiiVec4 value;
      if (ConvertToVec4(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec4d")
    {
      xiiVec4d value;
      if (ConvertToVec4d(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec2i")
    {
      xiiVec2I32 value;
      if (ConvertToVec2I(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec2i64")
    {
      xiiVec2I64 value;
      if (ConvertToVec2I64(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec3i")
    {
      xiiVec3I32 value;
      if (ConvertToVec3I(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec3i64")
    {
      xiiVec3I64 value;
      if (ConvertToVec3I64(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec4i")
    {
      xiiVec4I32 value;
      if (ConvertToVec4I(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec4i64")
    {
      xiiVec4I64 value;
      if (ConvertToVec4I64(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec2u")
    {
      xiiVec2U32 value;
      if (ConvertToVec2U(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec2u64")
    {
      xiiVec2U64 value;
      if (ConvertToVec2U64(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec3u")
    {
      xiiVec3U32 value;
      if (ConvertToVec3U(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec3u64")
    {
      xiiVec3U64 value;
      if (ConvertToVec3U64(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec4u")
    {
      xiiVec4U32 value;
      if (ConvertToVec4U(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec4u64")
    {
      xiiVec4U64 value;
      if (ConvertToVec4U64(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Mat3")
    {
      xiiMat3 value;
      if (ConvertToMat3(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Mat3d")
    {
      xiiMat3d value;
      if (ConvertToMat3d(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Mat4")
    {
      xiiMat4 value;
      if (ConvertToMat4(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Mat4d")
    {
      xiiMat4d value;
      if (ConvertToMat4d(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Transform")
    {
      xiiTransform value;
      if (ConvertToTransform(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Transformd")
    {
      xiiTransformd value;
      if (ConvertToTransformd(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Quat")
    {
      xiiQuat value;
      if (ConvertToQuat(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Quatd")
    {
      xiiQuatd value;
      if (ConvertToQuatd(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Uuid")
    {
      xiiUuid value;
      if (ConvertToUuid(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Angle")
    {
      xiiAngle value;
      if (ConvertToAngle(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Angled")
    {
      xiiAngled value;
      if (ConvertToAngle(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "HashedString")
    {
      xiiHashedString value;
      if (ConvertToHashedString(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "TempHashedString")
    {
      xiiTempHashedString value;
      if (ConvertToTempHashedString(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (pElement->GetCustomType() == "Invalid")
    {
      out_result = xiiVariant();
      return XII_SUCCESS;
    }

    if (const xiiRTTI* pRTTI = xiiRTTI::FindTypeByName(pElement->GetCustomType()))
    {
      if (xiiVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pRTTI))
      {
        if (pElement == nullptr)
          return XII_FAILURE;

        void* pObject = pRTTI->GetAllocator()->Allocate<void>();

        for (const xiiOpenDdlReaderElement* pChildElement = pElement->GetFirstChild(); pChildElement != nullptr; pChildElement = pChildElement->GetSibling())
        {
          if (!pChildElement->HasName())
            continue;

          if (xiiAbstractProperty* pProp = pRTTI->FindPropertyByName(pChildElement->GetName()))
          {
            // Custom types should be POD and only consist of member properties.
            if (pProp->GetCategory() == xiiPropertyCategory::Member)
            {
              xiiVariant subValue;
              if (ConvertToVariant(pChildElement, subValue).Succeeded())
              {
                xiiReflectionUtils::SetMemberPropertyValue(static_cast<xiiAbstractMemberProperty*>(pProp), pObject, subValue);
              }
            }
          }
        }
        out_result.MoveTypedObject(pObject, pRTTI);
        return XII_SUCCESS;
      }
      else
      {
        xiiLog::Error("The type '{0}' was declared but not defined, add XII_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", pElement->GetCustomType());
      }
    }
    else
    {
      xiiLog::Error("The type '{0}' is unknown.", pElement->GetCustomType());
    }
  }
  else
  {
    // always expect exactly one value
    if (pElement->GetNumPrimitives() != 1)
      return XII_FAILURE;

    switch (pElement->GetPrimitivesType())
    {
      case xiiOpenDdlPrimitiveType::Bool:
        out_result = pElement->GetPrimitivesBool()[0];
        return XII_SUCCESS;

      case xiiOpenDdlPrimitiveType::Int8:
        out_result = pElement->GetPrimitivesInt8()[0];
        return XII_SUCCESS;

      case xiiOpenDdlPrimitiveType::Int16:
        out_result = pElement->GetPrimitivesInt16()[0];
        return XII_SUCCESS;

      case xiiOpenDdlPrimitiveType::Int32:
        out_result = pElement->GetPrimitivesInt32()[0];
        return XII_SUCCESS;

      case xiiOpenDdlPrimitiveType::Int64:
        out_result = pElement->GetPrimitivesInt64()[0];
        return XII_SUCCESS;

      case xiiOpenDdlPrimitiveType::UInt8:
        out_result = pElement->GetPrimitivesUInt8()[0];
        return XII_SUCCESS;

      case xiiOpenDdlPrimitiveType::UInt16:
        out_result = pElement->GetPrimitivesUInt16()[0];
        return XII_SUCCESS;

      case xiiOpenDdlPrimitiveType::UInt32:
        out_result = pElement->GetPrimitivesUInt32()[0];
        return XII_SUCCESS;

      case xiiOpenDdlPrimitiveType::UInt64:
        out_result = pElement->GetPrimitivesUInt64()[0];
        return XII_SUCCESS;

      case xiiOpenDdlPrimitiveType::Float:
        out_result = pElement->GetPrimitivesFloat()[0];
        return XII_SUCCESS;

      case xiiOpenDdlPrimitiveType::Double:
        out_result = pElement->GetPrimitivesDouble()[0];
        return XII_SUCCESS;

      case xiiOpenDdlPrimitiveType::String:
        out_result = xiiString(pElement->GetPrimitivesString()[0]); // Ensure this isn't stored as a string view by copying to to a xiiString first
        return XII_SUCCESS;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }

  return XII_FAILURE;
}

void xiiOpenDdlUtils::StoreColor(xiiOpenDdlWriter& ref_writer, const xiiColor& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Color", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreColorGamma(xiiOpenDdlWriter& ref_writer, const xiiColorGammaUB& value, xiiStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("ColorGamma", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt8);
    ref_writer.WriteUInt8(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreTime(xiiOpenDdlWriter& ref_writer, const xiiTime& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Time", sName, bGlobalName, true);
  {
    const double d = value.GetSeconds();

    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);
    ref_writer.WriteDouble(&d, 1);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec2(xiiOpenDdlWriter& ref_writer, const xiiVec2& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec2", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.GetData(), 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec2d(xiiOpenDdlWriter& ref_writer, const xiiVec2d& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec2d", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);
    ref_writer.WriteDouble(value.GetData(), 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec3(xiiOpenDdlWriter& ref_writer, const xiiVec3& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec3", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.GetData(), 3);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec3d(xiiOpenDdlWriter& ref_writer, const xiiVec3d& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec3d", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);
    ref_writer.WriteDouble(value.GetData(), 3);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec4(xiiOpenDdlWriter& ref_writer, const xiiVec4& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec4", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec4d(xiiOpenDdlWriter& ref_writer, const xiiVec4d& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec4d", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);
    ref_writer.WriteDouble(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec2I(xiiOpenDdlWriter& ref_writer, const xiiVec2I32& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec2i", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int32);
    ref_writer.WriteInt32(value.GetData(), 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec2I64(xiiOpenDdlWriter& ref_writer, const xiiVec2I64& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec2i64", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int64);
    ref_writer.WriteInt64(value.GetData(), 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec3I(xiiOpenDdlWriter& ref_writer, const xiiVec3I32& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec3i", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int32);
    ref_writer.WriteInt32(value.GetData(), 3);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec3I64(xiiOpenDdlWriter& ref_writer, const xiiVec3I64& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec3i64", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int64);
    ref_writer.WriteInt64(value.GetData(), 3);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec4I(xiiOpenDdlWriter& ref_writer, const xiiVec4I32& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec4i", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int32);
    ref_writer.WriteInt32(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec4I64(xiiOpenDdlWriter& ref_writer, const xiiVec4I64& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec4i64", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int64);
    ref_writer.WriteInt64(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec2U(xiiOpenDdlWriter& ref_writer, const xiiVec2U32& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec2u", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt32);
    ref_writer.WriteUInt32(value.GetData(), 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec2U64(xiiOpenDdlWriter& ref_writer, const xiiVec2U64& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec2u64", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt64);
    ref_writer.WriteUInt64(value.GetData(), 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec3U(xiiOpenDdlWriter& ref_writer, const xiiVec3U32& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec3u", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt32);
    ref_writer.WriteUInt32(value.GetData(), 3);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec3U64(xiiOpenDdlWriter& ref_writer, const xiiVec3U64& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec3u64", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt64);
    ref_writer.WriteUInt64(value.GetData(), 3);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec4U(xiiOpenDdlWriter& ref_writer, const xiiVec4U32& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec4u", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt32);
    ref_writer.WriteUInt32(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec4U64(xiiOpenDdlWriter& ref_writer, const xiiVec4U64& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec4u64", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt64);
    ref_writer.WriteUInt64(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreMat3(xiiOpenDdlWriter& ref_writer, const xiiMat3& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Mat3", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);

    float f[9];
    value.GetAsArray(f, xiiMatrixLayout::ColumnMajor);
    ref_writer.WriteFloat(f, 9);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreMat3d(xiiOpenDdlWriter& ref_writer, const xiiMat3d& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Mat3d", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);

    double f[9];
    value.GetAsArray(f, xiiMatrixLayout::ColumnMajor);
    ref_writer.WriteDouble(f, 9);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreMat4(xiiOpenDdlWriter& ref_writer, const xiiMat4& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Mat4", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);

    float f[16];
    value.GetAsArray(f, xiiMatrixLayout::ColumnMajor);
    ref_writer.WriteFloat(f, 16);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreMat4d(xiiOpenDdlWriter& ref_writer, const xiiMat4d& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Mat4d", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);

    double f[16];
    value.GetAsArray(f, xiiMatrixLayout::ColumnMajor);
    ref_writer.WriteDouble(f, 16);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreTransform(xiiOpenDdlWriter& ref_writer, const xiiTransform& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Transform", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);

    float f[10];

    f[0] = value.m_vPosition.x;
    f[1] = value.m_vPosition.y;
    f[2] = value.m_vPosition.z;

    f[3] = value.m_qRotation.v.x;
    f[4] = value.m_qRotation.v.y;
    f[5] = value.m_qRotation.v.z;
    f[6] = value.m_qRotation.w;

    f[7] = value.m_vScale.x;
    f[8] = value.m_vScale.y;
    f[9] = value.m_vScale.z;

    ref_writer.WriteFloat(f, 10);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreTransformd(xiiOpenDdlWriter& ref_writer, const xiiTransformd& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Transformd", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);

    double f[10];

    f[0] = value.m_vPosition.x;
    f[1] = value.m_vPosition.y;
    f[2] = value.m_vPosition.z;

    f[3] = value.m_qRotation.v.x;
    f[4] = value.m_qRotation.v.y;
    f[5] = value.m_qRotation.v.z;
    f[6] = value.m_qRotation.w;

    f[7] = value.m_vScale.x;
    f[8] = value.m_vScale.y;
    f[9] = value.m_vScale.z;

    ref_writer.WriteDouble(f, 10);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreQuat(xiiOpenDdlWriter& ref_writer, const xiiQuat& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Quat", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.v.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreQuatd(xiiOpenDdlWriter& ref_writer, const xiiQuatd& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Quatd", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);
    ref_writer.WriteDouble(value.v.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreUuid(xiiOpenDdlWriter& ref_writer, const xiiUuid& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Uuid", sName, bGlobalName, true);
  {
    xiiUInt64 ui[2];
    value.GetValues(ui[0], ui[1]);

    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt64);
    ref_writer.WriteUInt64(ui, 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreAngle(xiiOpenDdlWriter& ref_writer, const xiiAngle& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Angle", sName, bGlobalName, true);
  {
    // have to use radians to prevent precision loss
    const float f = value.GetRadian();

    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(&f, 1);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreAngle(xiiOpenDdlWriter& ref_writer, const xiiAngled& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Angled", sName, bGlobalName, true);
  {
    // have to use radians to prevent precision loss
    const double f = value.GetRadian();

    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);
    ref_writer.WriteDouble(&f, 1);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreHashedString(xiiOpenDdlWriter& ref_writer, const xiiHashedString& value, xiiStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("HashedString", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::String);
    ref_writer.WriteString(value.GetView());
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreTempHashedString(xiiOpenDdlWriter& ref_writer, const xiiTempHashedString& value, xiiStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("TempHashedString", sName, bGlobalName, true);
  {
    const xiiUInt64 uiHash = value.GetHash();

    ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt64);
    ref_writer.WriteUInt64(&uiHash);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void xiiOpenDdlUtils::StoreVariant(xiiOpenDdlWriter& ref_writer, const xiiVariant& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  switch (value.GetType())
  {
    case xiiVariant::Type::Invalid:
      StoreInvalid(ref_writer, sName, bGlobalName);
      return;

    case xiiVariant::Type::Bool:
      StoreBool(ref_writer, value.Get<bool>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Int8:
      StoreInt8(ref_writer, value.Get<xiiInt8>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::UInt8:
      StoreUInt8(ref_writer, value.Get<xiiUInt8>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Int16:
      StoreInt16(ref_writer, value.Get<xiiInt16>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::UInt16:
      StoreUInt16(ref_writer, value.Get<xiiUInt16>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Int32:
      StoreInt32(ref_writer, value.Get<xiiInt32>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::UInt32:
      StoreUInt32(ref_writer, value.Get<xiiUInt32>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Int64:
      StoreInt64(ref_writer, value.Get<xiiInt64>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::UInt64:
      StoreUInt64(ref_writer, value.Get<xiiUInt64>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Float:
      StoreFloat(ref_writer, value.Get<float>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Double:
      StoreDouble(ref_writer, value.Get<double>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::String:
      xiiOpenDdlUtils::StoreString(ref_writer, value.Get<xiiString>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::StringView:
      xiiOpenDdlUtils::StoreString(ref_writer, value.Get<xiiString>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Color:
      StoreColor(ref_writer, value.Get<xiiColor>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector2:
      StoreVec2(ref_writer, value.Get<xiiVec2>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector2d:
      StoreVec2d(ref_writer, value.Get<xiiVec2d>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector3:
      StoreVec3(ref_writer, value.Get<xiiVec3>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector3d:
      StoreVec3d(ref_writer, value.Get<xiiVec3d>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector4:
      StoreVec4(ref_writer, value.Get<xiiVec4>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector4d:
      StoreVec4d(ref_writer, value.Get<xiiVec4d>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector2I:
      StoreVec2I(ref_writer, value.Get<xiiVec2I32>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector2I64:
      StoreVec2I64(ref_writer, value.Get<xiiVec2I64>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector3I:
      StoreVec3I(ref_writer, value.Get<xiiVec3I32>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector3I64:
      StoreVec3I64(ref_writer, value.Get<xiiVec3I64>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector4I:
      StoreVec4I(ref_writer, value.Get<xiiVec4I32>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector4I64:
      StoreVec4I64(ref_writer, value.Get<xiiVec4I64>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector2U:
      StoreVec2U(ref_writer, value.Get<xiiVec2U32>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector2U64:
      StoreVec2U64(ref_writer, value.Get<xiiVec2U64>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector3U:
      StoreVec3U(ref_writer, value.Get<xiiVec3U32>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector3U64:
      StoreVec3U64(ref_writer, value.Get<xiiVec3U64>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector4U:
      StoreVec4U(ref_writer, value.Get<xiiVec4U32>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Vector4U64:
      StoreVec4U64(ref_writer, value.Get<xiiVec4U64>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Quaternion:
      StoreQuat(ref_writer, value.Get<xiiQuat>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Quaterniond:
      StoreQuatd(ref_writer, value.Get<xiiQuatd>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Matrix3:
      StoreMat3(ref_writer, value.Get<xiiMat3>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Matrix3d:
      StoreMat3d(ref_writer, value.Get<xiiMat3d>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Matrix4:
      StoreMat4(ref_writer, value.Get<xiiMat4>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Matrix4d:
      StoreMat4d(ref_writer, value.Get<xiiMat4d>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Transform:
      StoreTransform(ref_writer, value.Get<xiiTransform>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Transformd:
      StoreTransformd(ref_writer, value.Get<xiiTransformd>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Time:
      StoreTime(ref_writer, value.Get<xiiTime>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Uuid:
      StoreUuid(ref_writer, value.Get<xiiUuid>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Angle:
      StoreAngle(ref_writer, value.Get<xiiAngle>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::Angled:
      StoreAngle(ref_writer, value.Get<xiiAngled>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::ColorGamma:
      StoreColorGamma(ref_writer, value.Get<xiiColorGammaUB>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::HashedString:
      StoreHashedString(ref_writer, value.Get<xiiHashedString>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::TempHashedString:
      StoreTempHashedString(ref_writer, value.Get<xiiTempHashedString>(), sName, bGlobalName);
      return;

    case xiiVariant::Type::VariantArray:
    {
      /// \test This is just quickly hacked

      ref_writer.BeginObject("VarArray", sName, bGlobalName);

      const xiiVariantArray& arr = value.Get<xiiVariantArray>();
      for (xiiUInt32 i = 0; i < arr.GetCount(); ++i)
      {
        xiiOpenDdlUtils::StoreVariant(ref_writer, arr[i]);
      }

      ref_writer.EndObject();
    }
      return;

    case xiiVariant::Type::VariantDictionary:
    {
      /// \test This is just quickly hacked

      ref_writer.BeginObject("VarDict", sName, bGlobalName);

      const xiiVariantDictionary& dict = value.Get<xiiVariantDictionary>();
      for (auto it = dict.GetIterator(); it.IsValid(); ++it)
      {
        xiiOpenDdlUtils::StoreVariant(ref_writer, it.Value(), it.Key(), false);
      }

      ref_writer.EndObject();
    }
      return;

    case xiiVariant::Type::DataBuffer:
    {
      /// \test This is just quickly hacked

      ref_writer.BeginObject("VarDataBuffer", sName, bGlobalName);
      ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::String);

      const xiiDataBuffer& db = value.Get<xiiDataBuffer>();
      ref_writer.WriteBinaryAsString(db.GetData(), db.GetCount());

      ref_writer.EndPrimitiveList();
      ref_writer.EndObject();
    }
      return;

    case xiiVariant::Type::TypedObject:
    {
      xiiTypedObject obj = value.Get<xiiTypedObject>();
      if (xiiVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(obj.m_pType))
      {
        ref_writer.BeginObject(obj.m_pType->GetTypeName(), sName, bGlobalName);
        {
          xiiHybridArray<xiiAbstractProperty*, 32> properties;
          obj.m_pType->GetAllProperties(properties);
          for (const xiiAbstractProperty* pProp : properties)
          {
            // Custom types should be POD and only consist of member properties.
            switch (pProp->GetCategory())
            {
              case xiiPropertyCategory::Member:
              {
                xiiVariant subValue = xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(pProp), obj.m_pObject);
                StoreVariant(ref_writer, subValue, pProp->GetPropertyName(), false);
              }
              break;
              case xiiPropertyCategory::Array:
              case xiiPropertyCategory::Set:
              case xiiPropertyCategory::Map:
                XII_REPORT_FAILURE("Only member properties are supported in custom variant types!");
                break;
              case xiiPropertyCategory::Constant:
              case xiiPropertyCategory::Function:
                break;
            }
          }
        }
        ref_writer.EndObject();
      }
      else
      {
        xiiLog::Error("The type '{0}' was declared but not defined, add XII_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", obj.m_pType->GetTypeName());
      }
    }
      return;
    default:
      XII_REPORT_FAILURE("Can't write this type of Variant");
  }
}

void xiiOpenDdlUtils::StoreString(xiiOpenDdlWriter& ref_writer, const xiiStringView& value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::String, sName, bGlobalName);
  ref_writer.WriteString(value);
  ref_writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreBool(xiiOpenDdlWriter& ref_writer, bool value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Bool, sName, bGlobalName);
  ref_writer.WriteBool(&value);
  ref_writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreFloat(xiiOpenDdlWriter& ref_writer, float value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float, sName, bGlobalName);
  ref_writer.WriteFloat(&value);
  ref_writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreDouble(xiiOpenDdlWriter& ref_writer, double value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double, sName, bGlobalName);
  ref_writer.WriteDouble(&value);
  ref_writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreInt8(xiiOpenDdlWriter& ref_writer, xiiInt8 value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int8, sName, bGlobalName);
  ref_writer.WriteInt8(&value);
  ref_writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreInt16(xiiOpenDdlWriter& ref_writer, xiiInt16 value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int16, sName, bGlobalName);
  ref_writer.WriteInt16(&value);
  ref_writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreInt32(xiiOpenDdlWriter& ref_writer, xiiInt32 value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int32, sName, bGlobalName);
  ref_writer.WriteInt32(&value);
  ref_writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreInt64(xiiOpenDdlWriter& ref_writer, xiiInt64 value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int64, sName, bGlobalName);
  ref_writer.WriteInt64(&value);
  ref_writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreUInt8(xiiOpenDdlWriter& ref_writer, xiiUInt8 value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt8, sName, bGlobalName);
  ref_writer.WriteUInt8(&value);
  ref_writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreUInt16(xiiOpenDdlWriter& ref_writer, xiiUInt16 value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt16, sName, bGlobalName);
  ref_writer.WriteUInt16(&value);
  ref_writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreUInt32(xiiOpenDdlWriter& ref_writer, xiiUInt32 value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt32, sName, bGlobalName);
  ref_writer.WriteUInt32(&value);
  ref_writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreUInt64(xiiOpenDdlWriter& ref_writer, xiiUInt64 value, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt64, sName, bGlobalName);
  ref_writer.WriteUInt64(&value);
  ref_writer.EndPrimitiveList();
}

XII_FOUNDATION_DLL void xiiOpenDdlUtils::StoreInvalid(xiiOpenDdlWriter& ref_writer, xiiStringView sName /*={}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Invalid", sName, bGlobalName, true);
  ref_writer.EndObject();
}

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_OpenDdlUtils);
