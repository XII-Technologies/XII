/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <GraphicsCore/Material/MaterialParameterBlock.h>

namespace
{
  template <typename T>
  void StoreValue(xiiArrayPtr<xiiUInt8> destination, const T& value)
  {
    XII_ASSERT_DEV(destination.GetCount() >= sizeof(T), "Material parameter destination is too small.");
    xiiMemoryUtils::Copy(reinterpret_cast<T*>(destination.GetPtr()), &value, 1U);
  }
} // namespace

xiiResult xiiMaterialParameterBlock::Initialize(xiiSharedPtr<const xiiMaterialSchema> pSchema)
{
  Clear();

  if (pSchema == nullptr || !pSchema->IsValid())
    return XII_FAILURE;

  m_pSchema = std::move(pSchema);
  m_Data.SetCount(m_pSchema->GetParameterBlockSize());
  xiiMemoryUtils::ZeroFill(m_Data.GetData(), m_Data.GetCount());
  m_Values.SetCount(m_pSchema->GetParameters().GetCount());

  const auto parameters = m_pSchema->GetParameters();
  for (xiiUInt32 i = 0U; i < parameters.GetCount(); ++i)
  {
    if (!parameters[i].m_DefaultValue.IsValid())
      continue;

    if (PackValue(parameters[i], parameters[i].m_DefaultValue).Failed())
    {
      Clear();
      return XII_FAILURE;
    }

    m_Values[i] = parameters[i].m_DefaultValue;
  }

  m_uiRevision = 1U;
  if (!m_Data.IsEmpty())
  {
    m_uiDirtyStart = 0U;
    m_uiDirtyEnd   = m_Data.GetCount();
  }
  return XII_SUCCESS;
}

void xiiMaterialParameterBlock::Clear()
{
  m_pSchema.Clear();
  m_Data.Clear();
  m_Values.Clear();
  m_uiRevision   = 0U;
  m_uiDirtyStart = xiiInvalidIndex;
  m_uiDirtyEnd   = 0U;
}

xiiArrayPtr<const xiiUInt8> xiiMaterialParameterBlock::GetDirtyData() const
{
  if (!IsDirty())
    return {};

  return xiiArrayPtr<const xiiUInt8>(m_Data.GetData() + m_uiDirtyStart, m_uiDirtyEnd - m_uiDirtyStart);
}

xiiResult xiiMaterialParameterBlock::SetValue(xiiMaterialParameterId id, const xiiVariant& value)
{
  if (m_pSchema == nullptr)
    return XII_FAILURE;

  const xiiUInt32 uiIndex = m_pSchema->FindParameterIndex(id);
  if (uiIndex == xiiInvalidIndex)
    return XII_FAILURE;

  const auto& definition = m_pSchema->GetParameters()[uiIndex];
  if (!definition.m_Flags.IsSet(xiiMaterialParameterFlags::RuntimeWritable) || !xiiMaterialSchema::IsValueCompatible(definition.m_Type, value))
    return XII_FAILURE;

  if (m_Values[uiIndex] == value)
    return XII_SUCCESS;

  XII_SUCCEED_OR_RETURN(PackValue(definition, value));
  m_Values[uiIndex] = value;
  MarkDirty(definition.m_uiOffset, definition.m_uiSize);
  ++m_uiRevision;
  return XII_SUCCESS;
}

xiiResult xiiMaterialParameterBlock::SetValue(const xiiTempHashedString& sName, const xiiVariant& value)
{
  xiiMaterialParameterId id;
  id.m_uiValue = sName.GetHash();
  return SetValue(id, value);
}

const xiiVariant* xiiMaterialParameterBlock::GetValue(xiiMaterialParameterId id) const
{
  if (m_pSchema == nullptr)
    return nullptr;

  const xiiUInt32 uiIndex = m_pSchema->FindParameterIndex(id);
  return uiIndex != xiiInvalidIndex ? &m_Values[uiIndex] : nullptr;
}

const xiiVariant* xiiMaterialParameterBlock::GetValue(const xiiTempHashedString& sName) const
{
  xiiMaterialParameterId id;
  id.m_uiValue = sName.GetHash();
  return GetValue(id);
}

void xiiMaterialParameterBlock::ClearDirtyRange()
{
  m_uiDirtyStart = xiiInvalidIndex;
  m_uiDirtyEnd   = 0U;
}

xiiResult xiiMaterialParameterBlock::PackValue(const xiiMaterialParameterDefinition& definition, const xiiVariant& value)
{
  if (!xiiMaterialSchema::IsValueCompatible(definition.m_Type, value))
    return XII_FAILURE;

  xiiArrayPtr<xiiUInt8> destination(m_Data.GetData() + definition.m_uiOffset, definition.m_uiSize);
  switch (definition.m_Type)
  {
    case xiiMaterialParameterType::Bool:
    {
      const xiiUInt32 uiValue = value.ConvertTo<bool>() ? 1U : 0U;
      StoreValue(destination, uiValue);
      break;
    }
    case xiiMaterialParameterType::Int: StoreValue(destination, value.ConvertTo<xiiInt32>()); break;
    case xiiMaterialParameterType::UInt: StoreValue(destination, value.ConvertTo<xiiUInt32>()); break;
    case xiiMaterialParameterType::Float: StoreValue(destination, value.ConvertTo<float>()); break;
    case xiiMaterialParameterType::Float2: StoreValue(destination, value.ConvertTo<xiiVec2>()); break;
    case xiiMaterialParameterType::Float3:
    {
      const xiiVec3 source = value.ConvertTo<xiiVec3>();
      const xiiVec4 packed(source.x, source.y, source.z, 0.0f);
      StoreValue(destination, packed);
      break;
    }
    case xiiMaterialParameterType::Float4: StoreValue(destination, value.ConvertTo<xiiVec4>()); break;
    case xiiMaterialParameterType::Color: StoreValue(destination, value.ConvertTo<xiiColor>()); break;
    case xiiMaterialParameterType::Matrix3:
    {
      const xiiMat3 source = value.ConvertTo<xiiMat3>();
      for (xiiUInt32 column = 0U; column < 3U; ++column)
      {
        const xiiVec4 packed(source.Element(column, 0), source.Element(column, 1), source.Element(column, 2), 0.0f);
        StoreValue(destination.GetSubArray(column * 16U, 16U), packed);
      }
      break;
    }
    case xiiMaterialParameterType::Matrix4: StoreValue(destination, value.ConvertTo<xiiMat4>()); break;
    default: return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiMaterialParameterBlock::MarkDirty(xiiUInt32 uiOffset, xiiUInt32 uiSize)
{
  m_uiDirtyStart = m_uiDirtyStart == xiiInvalidIndex ? uiOffset : xiiMath::Min(m_uiDirtyStart, uiOffset);
  m_uiDirtyEnd   = xiiMath::Max(m_uiDirtyEnd, uiOffset + uiSize);
}
