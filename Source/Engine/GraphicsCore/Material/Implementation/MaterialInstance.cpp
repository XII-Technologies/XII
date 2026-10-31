/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Threading/Lock.h>
#include <GraphicsCore/Material/MaterialInstance.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialResourceBinding, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialResourceBinding>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Id", m_Id),
    XII_RESOURCE_MEMBER_PROPERTY("Texture2D", m_hTexture2D),
    XII_RESOURCE_MEMBER_PROPERTY("TextureCube", m_hTextureCube),
    XII_MEMBER_PROPERTY("BindlessIndex", m_uiBindlessIndex),
    XII_MEMBER_PROPERTY("SamplerIndex", m_uiSamplerIndex),
    XII_MEMBER_PROPERTY("Revision", m_uiRevision),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiMaterialInstance::Initialize(xiiSharedPtr<const xiiMaterialSchema> pSchema, const xiiMaterialRuntimeState& runtimeState)
{
  XII_LOCK(m_Mutex);

  if (pSchema == nullptr || !pSchema->IsValid())
    return XII_FAILURE;

  xiiMaterialParameterBlock parameterBlock;
  XII_SUCCEED_OR_RETURN(parameterBlock.Initialize(pSchema));

  m_pSchema                     = std::move(pSchema);
  m_Parameters                  = std::move(parameterBlock);
  m_RuntimeState                = runtimeState;
  m_RuntimeState.m_Domain       = m_pSchema->GetDomain();
  m_RuntimeState.m_ShadingModel = m_pSchema->GetShadingModel();
  m_RuntimeState.m_uiLayoutHash = m_pSchema->GetLayoutHash();
  m_ParameterOverrides.Clear();
  m_ResourceOverrides.Clear();
  m_ResourceBindings.Clear();
  m_ResourceBindings.SetCount(m_pSchema->GetTextures().GetCount());

  xiiArrayPtr<const xiiMaterialTextureDefinition> pTextureDefinitions = m_pSchema->GetTextures();
  for (xiiUInt32 i = 0U; i < pTextureDefinitions.GetCount(); ++i)
  {
    m_ResourceBindings[i].m_Id = pTextureDefinitions[i].m_Id;
  }

  m_DirtyFlags                = xiiMaterialDirtyFlags::All;
  ++m_uiRevision;
  m_RuntimeState.m_uiRevision = m_uiRevision;
  return XII_SUCCESS;
}

xiiResult xiiMaterialInstance::RebindSchema(xiiSharedPtr<const xiiMaterialSchema> pSchema, const xiiMaterialRuntimeState& runtimeState, xiiDynamicArray<xiiString>* out_pWarnings)
{
  xiiMaterialInstance replacement;
  XII_SUCCEED_OR_RETURN(replacement.Initialize(pSchema, runtimeState));
  XII_LOCK(m_Mutex);
  xiiDynamicArray<xiiString> warnings;
  for (const auto id : m_ParameterOverrides)
  {
    const auto* oldDefinition = m_pSchema->FindParameter(id);
    const auto* newDefinition = pSchema->FindParameter(id);
    const auto* value         = m_Parameters.GetValue(id);
    if (oldDefinition != nullptr && newDefinition != nullptr && oldDefinition->m_Type == newDefinition->m_Type && value != nullptr)
    {
      XII_SUCCEED_OR_RETURN(replacement.SetParameter(id, *value));
    }
    else
    {
      xiiStringBuilder message;
      message.SetFormat("Material override '{}' was removed or changed type.", oldDefinition != nullptr ? oldDefinition->m_sName.GetString().GetView() : xiiStringView("<unknown>"));
      warnings.PushBack(message);
    }
  }
  for (const auto id : m_ResourceOverrides)
  {
    const auto* oldDefinition = m_pSchema->FindTexture(id);
    const auto* newDefinition = pSchema->FindTexture(id);
    if (oldDefinition != nullptr && newDefinition != nullptr && oldDefinition->m_TextureType == newDefinition->m_TextureType)
    {
      const auto oldIndex                                        = m_pSchema->FindTextureIndex(id);
      const auto newIndex                                        = pSchema->FindTextureIndex(id);
      replacement.m_ResourceBindings[newIndex]                   = m_ResourceBindings[oldIndex];
      replacement.m_ResourceBindings[newIndex].m_uiBindlessIndex = xiiInvalidIndex;
      replacement.m_ResourceBindings[newIndex].m_uiSamplerIndex  = xiiInvalidIndex;
      ++replacement.m_ResourceBindings[newIndex].m_uiRevision;
      replacement.m_ResourceOverrides.PushBack(id);
    }
    else
    {
      xiiStringBuilder message;
      message.SetFormat("Material resource override '{}' was removed or changed dimension.", oldDefinition != nullptr ? oldDefinition->m_sName.GetString().GetView() : xiiStringView("<unknown>"));
      warnings.PushBack(message);
    }
  }
  m_pSchema            = std::move(replacement.m_pSchema);
  m_Parameters         = std::move(replacement.m_Parameters);
  m_ResourceBindings   = std::move(replacement.m_ResourceBindings);
  m_ParameterOverrides = std::move(replacement.m_ParameterOverrides);
  m_ResourceOverrides  = std::move(replacement.m_ResourceOverrides);
  m_RuntimeState       = replacement.m_RuntimeState;
  ++m_uiRevision;
  m_RuntimeState.m_uiRevision = m_uiRevision;
  m_DirtyFlags                = xiiMaterialDirtyFlags::All;
  if (out_pWarnings != nullptr)
    *out_pWarnings = std::move(warnings);
  return XII_SUCCESS;
}

xiiResult xiiMaterialInstance::SetParameter(xiiMaterialParameterId id, const xiiVariant& value)
{
  XII_LOCK(m_Mutex);

  const xiiMaterialParameterDefinition* pDefinition = m_pSchema != nullptr ? m_pSchema->FindParameter(id) : nullptr;
  if (pDefinition == nullptr)
    return XII_FAILURE;

  const xiiUInt32 uiPreviousRevision = m_Parameters.GetRevision();
  XII_SUCCEED_OR_RETURN(m_Parameters.SetValue(id, value));
  if (!m_ParameterOverrides.Contains(id))
    m_ParameterOverrides.PushBack(id);
  if (uiPreviousRevision == m_Parameters.GetRevision())
    return XII_SUCCESS;

  m_DirtyFlags.Add(xiiMaterialDirtyFlags::Parameters);
  if (pDefinition->m_Flags.IsSet(xiiMaterialParameterFlags::Specialization))
  {
    m_DirtyFlags.Add(xiiMaterialDirtyFlags::Specialization | xiiMaterialDirtyFlags::Pipeline);
  }

  ++m_uiRevision;
  m_RuntimeState.m_uiRevision = m_uiRevision;
  return XII_SUCCESS;
}

xiiResult xiiMaterialInstance::SetParameter(xiiStringView sName, const xiiVariant& value)
{
  return SetParameter(xiiMaterialParameterId::Make(sName), value);
}

xiiVariant xiiMaterialInstance::GetParameter(xiiMaterialParameterId id) const
{
  XII_LOCK(m_Mutex);
  const xiiVariant* pValue = m_Parameters.GetValue(id);
  return pValue != nullptr ? *pValue : xiiVariant();
}

xiiResult xiiMaterialInstance::SetTexture2D(xiiMaterialParameterId id, const xiiTexture2DResourceHandle& hTexture)
{
  return SetResourceBinding(id, &hTexture, nullptr, nullptr);
}

xiiResult xiiMaterialInstance::SetTextureCube(xiiMaterialParameterId id, const xiiTextureCubeResourceHandle& hTexture)
{
  return SetResourceBinding(id, nullptr, &hTexture, nullptr);
}

xiiResult xiiMaterialInstance::SetBindlessIndex(xiiMaterialParameterId id, xiiUInt32 uiBindlessIndex, xiiUInt32 uiSamplerIndex)
{
  return SetResourceBinding(id, nullptr, nullptr, &uiBindlessIndex, &uiSamplerIndex);
}

void xiiMaterialInstance::SetRuntimeState(const xiiMaterialRuntimeState& runtimeState)
{
  XII_LOCK(m_Mutex);
  m_RuntimeState = runtimeState;
  if (m_pSchema != nullptr)
  {
    m_RuntimeState.m_Domain       = m_pSchema->GetDomain();
    m_RuntimeState.m_ShadingModel = m_pSchema->GetShadingModel();
    m_RuntimeState.m_uiLayoutHash = m_pSchema->GetLayoutHash();
  }
  ++m_uiRevision;
  m_RuntimeState.m_uiRevision = m_uiRevision;
  m_DirtyFlags.Add(xiiMaterialDirtyFlags::Pipeline | xiiMaterialDirtyFlags::Specialization);
}

xiiMaterialRuntimeState xiiMaterialInstance::GetRuntimeState() const
{
  XII_LOCK(m_Mutex);
  return m_RuntimeState;
}

xiiSharedPtr<const xiiMaterialSchema> xiiMaterialInstance::GetSchema() const
{
  XII_LOCK(m_Mutex);
  return m_pSchema;
}

xiiUInt32 xiiMaterialInstance::GetRevision() const
{
  XII_LOCK(m_Mutex);
  return m_uiRevision;
}

void xiiMaterialInstance::CreateSnapshot(xiiMaterialInstanceSnapshot& out_snapshot) const
{
  XII_LOCK(m_Mutex);
  CreateSnapshotLocked(out_snapshot);
}

void xiiMaterialInstance::ConsumeSnapshot(xiiMaterialInstanceSnapshot& out_snapshot)
{
  XII_LOCK(m_Mutex);
  CreateSnapshotLocked(out_snapshot);
  m_DirtyFlags = xiiMaterialDirtyFlags::None;
  m_Parameters.ClearDirtyRange();
}

xiiResult xiiMaterialInstance::SetResourceBinding(xiiMaterialParameterId id, const xiiTexture2DResourceHandle* pTexture2D, const xiiTextureCubeResourceHandle* pTextureCube, const xiiUInt32* pBindlessIndex, const xiiUInt32* pSamplerIndex)
{
  XII_LOCK(m_Mutex);

  if (m_pSchema == nullptr)
    return XII_FAILURE;

  const xiiUInt32 uiIndex = m_pSchema->FindTextureIndex(id);
  if (uiIndex == xiiInvalidIndex)
    return XII_FAILURE;

  const xiiMaterialTextureDefinition& definition = m_pSchema->GetTextures()[uiIndex];
  if (pTexture2D != nullptr && definition.m_TextureType != xiiGALShaderTextureType::Texture2D && definition.m_TextureType != xiiGALShaderTextureType::Texture2DArray)
    return XII_FAILURE;
  if (pTextureCube != nullptr && definition.m_TextureType != xiiGALShaderTextureType::TextureCube && definition.m_TextureType != xiiGALShaderTextureType::TextureCubeArray)
    return XII_FAILURE;
  if (pBindlessIndex != nullptr && !definition.m_bBindless)
    return XII_FAILURE;

  if ((pTexture2D != nullptr || pTextureCube != nullptr) && !m_ResourceOverrides.Contains(id))
    m_ResourceOverrides.PushBack(id);

  xiiMaterialResourceBinding& binding  = m_ResourceBindings[uiIndex];
  bool                        bChanged = false;
  if (pTexture2D != nullptr && binding.m_hTexture2D != *pTexture2D)
  {
    binding.m_hTexture2D      = *pTexture2D;
    binding.m_uiBindlessIndex = xiiInvalidIndex;
    binding.m_uiSamplerIndex  = xiiInvalidIndex;
    bChanged                  = true;
  }
  if (pTextureCube != nullptr && binding.m_hTextureCube != *pTextureCube)
  {
    binding.m_hTextureCube    = *pTextureCube;
    binding.m_uiBindlessIndex = xiiInvalidIndex;
    binding.m_uiSamplerIndex  = xiiInvalidIndex;
    bChanged                  = true;
  }
  if (pBindlessIndex != nullptr && binding.m_uiBindlessIndex != *pBindlessIndex)
  {
    binding.m_uiBindlessIndex = *pBindlessIndex;
    bChanged                  = true;
  }
  if (pSamplerIndex != nullptr && binding.m_uiSamplerIndex != *pSamplerIndex)
  {
    binding.m_uiSamplerIndex = *pSamplerIndex;
    bChanged                 = true;
  }

  if (!bChanged)
    return XII_SUCCESS;

  ++binding.m_uiRevision;
  ++m_uiRevision;
  m_RuntimeState.m_uiRevision = m_uiRevision;
  m_DirtyFlags.Add(xiiMaterialDirtyFlags::Resources);
  return XII_SUCCESS;
}

void xiiMaterialInstance::CreateSnapshotLocked(xiiMaterialInstanceSnapshot& out_snapshot) const
{
  out_snapshot.m_pSchema          = m_pSchema;
  out_snapshot.m_ParameterData    = m_Parameters.GetData();
  out_snapshot.m_ResourceBindings = m_ResourceBindings;
  out_snapshot.m_RuntimeState     = m_RuntimeState;
  out_snapshot.m_DirtyFlags       = m_DirtyFlags;
  out_snapshot.m_uiRevision       = m_uiRevision;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Material_Implementation_MaterialInstance);
