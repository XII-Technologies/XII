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

xiiResult xiiOpenDdlUtils::ConvertToVec2(const xiiOpenDdlReaderElement* pElement, xiiVec2& out_result)
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

    out_result.Set(pValues[0], pValues[1]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec2d(const xiiOpenDdlReaderElement* pElement, xiiVec2d& out_result)
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

    out_result.Set(pValues[0], pValues[1]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec3(const xiiOpenDdlReaderElement* pElement, xiiVec3& out_result)
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

    out_result.Set(pValues[0], pValues[1], pValues[2]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec3d(const xiiOpenDdlReaderElement* pElement, xiiVec3d& out_result)
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

    out_result.Set(pValues[0], pValues[1], pValues[2]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec4(const xiiOpenDdlReaderElement* pElement, xiiVec4& out_result)
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

    out_result.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec4d(const xiiOpenDdlReaderElement* pElement, xiiVec4d& out_result)
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

    out_result.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec2I(const xiiOpenDdlReaderElement* pElement, xiiVec2I32& out_result)
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

    out_result.Set(pValues[0], pValues[1]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec2I64(const xiiOpenDdlReaderElement* pElement, xiiVec2I64& out_result)
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

    out_result.Set(pValues[0], pValues[1]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec3I(const xiiOpenDdlReaderElement* pElement, xiiVec3I32& out_result)
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

    out_result.Set(pValues[0], pValues[1], pValues[2]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec3I64(const xiiOpenDdlReaderElement* pElement, xiiVec3I64& out_result)
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

    out_result.Set(pValues[0], pValues[1], pValues[2]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec4I(const xiiOpenDdlReaderElement* pElement, xiiVec4I32& out_result)
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

    out_result.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec4I64(const xiiOpenDdlReaderElement* pElement, xiiVec4I64& out_result)
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

    out_result.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec2U(const xiiOpenDdlReaderElement* pElement, xiiVec2U32& out_result)
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

    out_result.Set(pValues[0], pValues[1]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec2U64(const xiiOpenDdlReaderElement* pElement, xiiVec2U64& out_result)
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

    out_result.Set(pValues[0], pValues[1]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec3U(const xiiOpenDdlReaderElement* pElement, xiiVec3U32& out_result)
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

    out_result.Set(pValues[0], pValues[1], pValues[2]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec3U64(const xiiOpenDdlReaderElement* pElement, xiiVec3U64& out_result)
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

    out_result.Set(pValues[0], pValues[1], pValues[2]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec4U(const xiiOpenDdlReaderElement* pElement, xiiVec4U32& out_result)
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

    out_result.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToVec4U64(const xiiOpenDdlReaderElement* pElement, xiiVec4U64& out_result)
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

    out_result.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToMat3(const xiiOpenDdlReaderElement* pElement, xiiMat3& out_result)
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

    out_result.SetFromArray(pValues, xiiMatrixLayout::ColumnMajor);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToMat3d(const xiiOpenDdlReaderElement* pElement, xiiMat3d& out_result)
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

    out_result.SetFromArray(pValues, xiiMatrixLayout::ColumnMajor);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToMat4(const xiiOpenDdlReaderElement* pElement, xiiMat4& out_result)
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

    out_result.SetFromArray(pValues, xiiMatrixLayout::ColumnMajor);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToMat4d(const xiiOpenDdlReaderElement* pElement, xiiMat4d& out_result)
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

    out_result.SetFromArray(pValues, xiiMatrixLayout::ColumnMajor);

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

xiiResult xiiOpenDdlUtils::ConvertToQuat(const xiiOpenDdlReaderElement* pElement, xiiQuat& out_result)
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

    out_result.SetElements(pValues[0], pValues[1], pValues[2], pValues[3]);

    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiOpenDdlUtils::ConvertToQuatd(const xiiOpenDdlReaderElement* pElement, xiiQuatd& out_result)
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

    out_result.SetElements(pValues[0], pValues[1], pValues[2], pValues[3]);

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

xiiResult xiiOpenDdlUtils::ConvertToVariant(const xiiOpenDdlReaderElement* pElement, xiiVariant& out_result)
{
  if (pElement == nullptr)
    return XII_FAILURE;

  // expect a custom type
  if (pElement->IsCustomType())
  {
    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "VarArray"))
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

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "VarDict"))
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

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "VarDataBuffer"))
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

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Color"))
    {
      xiiColor value;
      if (ConvertToColor(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "ColorGamma"))
    {
      xiiColorGammaUB value;
      if (ConvertToColorGamma(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Time"))
    {
      xiiTime value;
      if (ConvertToTime(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec2"))
    {
      xiiVec2 value;
      if (ConvertToVec2(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec2d"))
    {
      xiiVec2d value;
      if (ConvertToVec2d(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec3"))
    {
      xiiVec3 value;
      if (ConvertToVec3(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec3d"))
    {
      xiiVec3d value;
      if (ConvertToVec3d(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec4"))
    {
      xiiVec4 value;
      if (ConvertToVec4(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec4d"))
    {
      xiiVec4d value;
      if (ConvertToVec4d(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec2i"))
    {
      xiiVec2I32 value;
      if (ConvertToVec2I(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec2i64"))
    {
      xiiVec2I64 value;
      if (ConvertToVec2I64(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec3i"))
    {
      xiiVec3I32 value;
      if (ConvertToVec3I(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec3i64"))
    {
      xiiVec3I64 value;
      if (ConvertToVec3I64(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec4i"))
    {
      xiiVec4I32 value;
      if (ConvertToVec4I(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec4i64"))
    {
      xiiVec4I64 value;
      if (ConvertToVec4I64(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec2u"))
    {
      xiiVec2U32 value;
      if (ConvertToVec2U(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec2u64"))
    {
      xiiVec2U64 value;
      if (ConvertToVec2U64(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec3u"))
    {
      xiiVec3U32 value;
      if (ConvertToVec3U(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec3u64"))
    {
      xiiVec3U64 value;
      if (ConvertToVec3U64(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec4u"))
    {
      xiiVec4U32 value;
      if (ConvertToVec4U(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Vec4u64"))
    {
      xiiVec4U64 value;
      if (ConvertToVec4U64(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Mat3"))
    {
      xiiMat3 value;
      if (ConvertToMat3(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Mat3d"))
    {
      xiiMat3d value;
      if (ConvertToMat3d(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Mat4"))
    {
      xiiMat4 value;
      if (ConvertToMat4(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Mat4d"))
    {
      xiiMat4d value;
      if (ConvertToMat4d(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Transform"))
    {
      xiiTransform value;
      if (ConvertToTransform(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Transformd"))
    {
      xiiTransformd value;
      if (ConvertToTransformd(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Quat"))
    {
      xiiQuat value;
      if (ConvertToQuat(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Quatd"))
    {
      xiiQuatd value;
      if (ConvertToQuatd(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Uuid"))
    {
      xiiUuid value;
      if (ConvertToUuid(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
      return XII_SUCCESS;
    }

    if (xiiStringUtils::IsEqual(pElement->GetCustomType(), "Angle"))
    {
      xiiAngle value;
      if (ConvertToAngle(pElement, value).Failed())
        return XII_FAILURE;

      out_result = value;
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
        out_result = xiiString(pElement->GetPrimitivesString()[0]); // make sure this isn't stored as a string view by copying to to a xiiString first
        return XII_SUCCESS;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }

  return XII_FAILURE;
}

void xiiOpenDdlUtils::StoreColor(xiiOpenDdlWriter& writer, const xiiColor& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Color", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);
    writer.WriteFloat(value.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreColorGamma(
  xiiOpenDdlWriter&      writer,
  const xiiColorGammaUB& value,
  const char*            szName /*= nullptr*/,
  bool                   bGlobalName /*= false*/)
{
  writer.BeginObject("ColorGamma", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt8);
    writer.WriteUInt8(value.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreTime(xiiOpenDdlWriter& writer, const xiiTime& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Time", szName, bGlobalName, true);
  {
    const double d = value.GetSeconds();

    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);
    writer.WriteDouble(&d, 1);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec2(xiiOpenDdlWriter& writer, const xiiVec2& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec2", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);
    writer.WriteFloat(value.GetData(), 2);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec2d(xiiOpenDdlWriter& writer, const xiiVec2d& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec2d", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);
    writer.WriteDouble(value.GetData(), 2);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec3(xiiOpenDdlWriter& writer, const xiiVec3& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec3", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);
    writer.WriteFloat(value.GetData(), 3);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec3d(xiiOpenDdlWriter& writer, const xiiVec3d& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec3d", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);
    writer.WriteDouble(value.GetData(), 3);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec4(xiiOpenDdlWriter& writer, const xiiVec4& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec4", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);
    writer.WriteFloat(value.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec4d(xiiOpenDdlWriter& writer, const xiiVec4d& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec4d", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);
    writer.WriteDouble(value.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec2I(xiiOpenDdlWriter& writer, const xiiVec2I32& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec2i", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int32);
    writer.WriteInt32(value.GetData(), 2);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec2I64(xiiOpenDdlWriter& writer, const xiiVec2I64& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec2i64", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int64);
    writer.WriteInt64(value.GetData(), 2);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec3I(xiiOpenDdlWriter& writer, const xiiVec3I32& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec3i", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int32);
    writer.WriteInt32(value.GetData(), 3);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec3I64(xiiOpenDdlWriter& writer, const xiiVec3I64& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec3i64", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int64);
    writer.WriteInt64(value.GetData(), 3);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec4I(xiiOpenDdlWriter& writer, const xiiVec4I32& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec4i", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int32);
    writer.WriteInt32(value.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec4I64(xiiOpenDdlWriter& writer, const xiiVec4I64& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec4i64", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int64);
    writer.WriteInt64(value.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec2U(xiiOpenDdlWriter& writer, const xiiVec2U32& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec2u", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt32);
    writer.WriteUInt32(value.GetData(), 2);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec2U64(xiiOpenDdlWriter& writer, const xiiVec2U64& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec2u64", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt64);
    writer.WriteUInt64(value.GetData(), 2);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec3U(xiiOpenDdlWriter& writer, const xiiVec3U32& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec3u", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt32);
    writer.WriteUInt32(value.GetData(), 3);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec3U64(xiiOpenDdlWriter& writer, const xiiVec3U64& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec3u64", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt64);
    writer.WriteUInt64(value.GetData(), 3);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec4U(xiiOpenDdlWriter& writer, const xiiVec4U32& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec4u", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt32);
    writer.WriteUInt32(value.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVec4U64(xiiOpenDdlWriter& writer, const xiiVec4U64& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Vec4u64", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt64);
    writer.WriteUInt64(value.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreMat3(xiiOpenDdlWriter& writer, const xiiMat3& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Mat3", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);

    float f[9];
    value.GetAsArray(f, xiiMatrixLayout::ColumnMajor);
    writer.WriteFloat(f, 9);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreMat3d(xiiOpenDdlWriter& writer, const xiiMat3d& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Mat3d", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);

    double f[9];
    value.GetAsArray(f, xiiMatrixLayout::ColumnMajor);
    writer.WriteDouble(f, 9);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreMat4(xiiOpenDdlWriter& writer, const xiiMat4& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Mat4", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);

    float f[16];
    value.GetAsArray(f, xiiMatrixLayout::ColumnMajor);
    writer.WriteFloat(f, 16);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreMat4d(xiiOpenDdlWriter& writer, const xiiMat4d& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Mat4d", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);

    double f[16];
    value.GetAsArray(f, xiiMatrixLayout::ColumnMajor);
    writer.WriteDouble(f, 16);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreTransform(xiiOpenDdlWriter& writer, const xiiTransform& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Transform", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);

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

    writer.WriteFloat(f, 10);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreTransformd(xiiOpenDdlWriter& writer, const xiiTransformd& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Transformd", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);

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

    writer.WriteDouble(f, 10);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreQuat(xiiOpenDdlWriter& writer, const xiiQuat& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Quat", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);
    writer.WriteFloat(value.v.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreQuatd(xiiOpenDdlWriter& writer, const xiiQuatd& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Quatd", szName, bGlobalName, true);
  {
    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double);
    writer.WriteDouble(value.v.GetData(), 4);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreUuid(xiiOpenDdlWriter& writer, const xiiUuid& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Uuid", szName, bGlobalName, true);
  {
    xiiUInt64 ui[2];
    value.GetValues(ui[0], ui[1]);

    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt64);
    writer.WriteUInt64(ui, 2);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreAngle(xiiOpenDdlWriter& writer, const xiiAngle& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginObject("Angle", szName, bGlobalName, true);
  {
    // have to use radians to prevent precision loss
    const float f = value.GetRadian();

    writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float);
    writer.WriteFloat(&f, 1);
    writer.EndPrimitiveList();
  }
  writer.EndObject();
}

void xiiOpenDdlUtils::StoreVariant(xiiOpenDdlWriter& writer, const xiiVariant& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  switch (value.GetType())
  {
    case xiiVariant::Type::Invalid:
      return; // store anything ?

    case xiiVariant::Type::Bool:
      StoreBool(writer, value.Get<bool>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Int8:
      StoreInt8(writer, value.Get<xiiInt8>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::UInt8:
      StoreUInt8(writer, value.Get<xiiUInt8>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Int16:
      StoreInt16(writer, value.Get<xiiInt16>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::UInt16:
      StoreUInt16(writer, value.Get<xiiUInt16>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Int32:
      StoreInt32(writer, value.Get<xiiInt32>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::UInt32:
      StoreUInt32(writer, value.Get<xiiUInt32>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Int64:
      StoreInt64(writer, value.Get<xiiInt64>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::UInt64:
      StoreUInt64(writer, value.Get<xiiUInt64>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Float:
      StoreFloat(writer, value.Get<float>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Double:
      StoreDouble(writer, value.Get<double>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::String:
    {
      const xiiString& var = value.Get<xiiString>();
      xiiOpenDdlUtils::StoreString(writer, var, szName, bGlobalName);
    }
      return;

    case xiiVariant::Type::StringView:
    {
      const xiiStringView& var = value.Get<xiiStringView>();
      xiiOpenDdlUtils::StoreString(writer, var, szName, bGlobalName);
    }
      return;

    case xiiVariant::Type::Color:
      StoreColor(writer, value.Get<xiiColor>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector2:
      StoreVec2(writer, value.Get<xiiVec2>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector2d:
      StoreVec2d(writer, value.Get<xiiVec2d>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector3:
      StoreVec3(writer, value.Get<xiiVec3>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector3d:
      StoreVec3d(writer, value.Get<xiiVec3d>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector4:
      StoreVec4(writer, value.Get<xiiVec4>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector4d:
      StoreVec4d(writer, value.Get<xiiVec4d>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector2I:
      StoreVec2I(writer, value.Get<xiiVec2I32>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector2I64:
      StoreVec2I64(writer, value.Get<xiiVec2I64>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector3I:
      StoreVec3I(writer, value.Get<xiiVec3I32>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector3I64:
      StoreVec3I64(writer, value.Get<xiiVec3I64>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector4I:
      StoreVec4I(writer, value.Get<xiiVec4I32>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector4I64:
      StoreVec4I64(writer, value.Get<xiiVec4I64>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector2U:
      StoreVec2U(writer, value.Get<xiiVec2U32>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector2U64:
      StoreVec2U64(writer, value.Get<xiiVec2U64>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector3U:
      StoreVec3U(writer, value.Get<xiiVec3U32>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector3U64:
      StoreVec3U64(writer, value.Get<xiiVec3U64>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector4U:
      StoreVec4U(writer, value.Get<xiiVec4U32>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Vector4U64:
      StoreVec4U64(writer, value.Get<xiiVec4U64>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Quaternion:
      StoreQuat(writer, value.Get<xiiQuat>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Quaterniond:
      StoreQuatd(writer, value.Get<xiiQuatd>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Matrix3:
      StoreMat3(writer, value.Get<xiiMat3>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Matrix3d:
      StoreMat3d(writer, value.Get<xiiMat3d>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Matrix4:
      StoreMat4(writer, value.Get<xiiMat4>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Matrix4d:
      StoreMat4d(writer, value.Get<xiiMat4d>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Transform:
      StoreTransform(writer, value.Get<xiiTransform>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Transformd:
      StoreTransformd(writer, value.Get<xiiTransformd>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Time:
      StoreTime(writer, value.Get<xiiTime>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Uuid:
      StoreUuid(writer, value.Get<xiiUuid>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::Angle:
      StoreAngle(writer, value.Get<xiiAngle>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::ColorGamma:
      StoreColorGamma(writer, value.Get<xiiColorGammaUB>(), szName, bGlobalName);
      return;

    case xiiVariant::Type::VariantArray:
    {
      /// \test This is just quickly hacked

      writer.BeginObject("VarArray", szName, bGlobalName);

      const xiiVariantArray& arr = value.Get<xiiVariantArray>();
      for (xiiUInt32 i = 0; i < arr.GetCount(); ++i)
      {
        xiiOpenDdlUtils::StoreVariant(writer, arr[i]);
      }

      writer.EndObject();
    }
      return;

    case xiiVariant::Type::VariantDictionary:
    {
      /// \test This is just quickly hacked

      writer.BeginObject("VarDict", szName, bGlobalName);

      const xiiVariantDictionary& dict = value.Get<xiiVariantDictionary>();
      for (auto it = dict.GetIterator(); it.IsValid(); ++it)
      {
        xiiOpenDdlUtils::StoreVariant(writer, it.Value(), it.Key(), false);
      }

      writer.EndObject();
    }
      return;

    case xiiVariant::Type::DataBuffer:
    {
      /// \test This is just quickly hacked

      writer.BeginObject("VarDataBuffer", szName, bGlobalName);
      writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::String);

      const xiiDataBuffer& db = value.Get<xiiDataBuffer>();
      writer.WriteBinaryAsString(db.GetData(), db.GetCount());

      writer.EndPrimitiveList();
      writer.EndObject();
    }
      return;

    case xiiVariant::Type::TypedObject:
    {
      xiiTypedObject obj = value.Get<xiiTypedObject>();
      if (xiiVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(obj.m_pType))
      {
        writer.BeginObject(obj.m_pType->GetTypeName(), szName, bGlobalName);
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
                StoreVariant(writer, subValue, pProp->GetPropertyName(), false);
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
        writer.EndObject();
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

void xiiOpenDdlUtils::StoreString(xiiOpenDdlWriter& writer, const xiiStringView& value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::String, szName, bGlobalName);
  writer.WriteString(value);
  writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreBool(xiiOpenDdlWriter& writer, bool value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Bool, szName, bGlobalName);
  writer.WriteBool(&value);
  writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreFloat(xiiOpenDdlWriter& writer, float value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Float, szName, bGlobalName);
  writer.WriteFloat(&value);
  writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreDouble(xiiOpenDdlWriter& writer, double value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Double, szName, bGlobalName);
  writer.WriteDouble(&value);
  writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreInt8(xiiOpenDdlWriter& writer, xiiInt8 value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int8, szName, bGlobalName);
  writer.WriteInt8(&value);
  writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreInt16(xiiOpenDdlWriter& writer, xiiInt16 value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int16, szName, bGlobalName);
  writer.WriteInt16(&value);
  writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreInt32(xiiOpenDdlWriter& writer, xiiInt32 value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int32, szName, bGlobalName);
  writer.WriteInt32(&value);
  writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreInt64(xiiOpenDdlWriter& writer, xiiInt64 value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::Int64, szName, bGlobalName);
  writer.WriteInt64(&value);
  writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreUInt8(xiiOpenDdlWriter& writer, xiiUInt8 value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt8, szName, bGlobalName);
  writer.WriteUInt8(&value);
  writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreUInt16(xiiOpenDdlWriter& writer, xiiUInt16 value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt16, szName, bGlobalName);
  writer.WriteUInt16(&value);
  writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreUInt32(xiiOpenDdlWriter& writer, xiiUInt32 value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt32, szName, bGlobalName);
  writer.WriteUInt32(&value);
  writer.EndPrimitiveList();
}

void xiiOpenDdlUtils::StoreUInt64(xiiOpenDdlWriter& writer, xiiUInt64 value, const char* szName /*= nullptr*/, bool bGlobalName /*= false*/)
{
  writer.BeginPrimitiveList(xiiOpenDdlPrimitiveType::UInt64, szName, bGlobalName);
  writer.WriteUInt64(&value);
  writer.EndPrimitiveList();
}



XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_OpenDdlUtils);
