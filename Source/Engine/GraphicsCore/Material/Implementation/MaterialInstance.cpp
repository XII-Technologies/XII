/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Threading/Lock.h>
#include <GraphicsCore/Material/MaterialInstance.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialResourceBinding, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialResourceBinding>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Id", m_Id),
    XII_RESOURCE_MEMBER_PROPERTY("Texture2D", m_hTexture2D),
    XII_RESOURCE_MEMBER_PROPERTY("TextureCube", m_hTextureCube),
    XII_MEMBER_PROPERTY("BindlessIndex", m_uiBindlessIndex),
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
  m_ResourceBindings.SetCount(m_pSchema->GetTextures().GetCount());

  const auto textures = m_pSchema->GetTextures();
  for (xiiUInt32 i = 0U; i < textures.GetCount(); ++i)
    m_ResourceBindings[i].m_Id = textures[i].m_Id;

  m_DirtyFlags                = xiiMaterialDirtyFlags::All;
  m_uiRevision                = 1U;
  m_RuntimeState.m_uiRevision = m_uiRevision;
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
  if (uiPreviousRevision == m_Parameters.GetRevision())
    return XII_SUCCESS;

  m_DirtyFlags.Add(xiiMaterialDirtyFlags::Parameters);
  if (pDefinition->m_Flags.IsSet(xiiMaterialParameterFlags::Specialization))
    m_DirtyFlags.Add(xiiMaterialDirtyFlags::Specialization | xiiMaterialDirtyFlags::Pipeline);

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

xiiResult xiiMaterialInstance::SetBindlessIndex(xiiMaterialParameterId id, xiiUInt32 uiBindlessIndex)
{
  return SetResourceBinding(id, nullptr, nullptr, &uiBindlessIndex);
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

xiiResult xiiMaterialInstance::SetResourceBinding(xiiMaterialParameterId id, const xiiTexture2DResourceHandle* pTexture2D, const xiiTextureCubeResourceHandle* pTextureCube, const xiiUInt32* pBindlessIndex)
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

  xiiMaterialResourceBinding& binding  = m_ResourceBindings[uiIndex];
  bool                        bChanged = false;
  if (pTexture2D != nullptr && binding.m_hTexture2D != *pTexture2D)
  {
    binding.m_hTexture2D = *pTexture2D;
    bChanged             = true;
  }
  if (pTextureCube != nullptr && binding.m_hTextureCube != *pTextureCube)
  {
    binding.m_hTextureCube = *pTextureCube;
    bChanged               = true;
  }
  if (pBindlessIndex != nullptr && binding.m_uiBindlessIndex != *pBindlessIndex)
  {
    binding.m_uiBindlessIndex = *pBindlessIndex;
    bChanged                  = true;
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
