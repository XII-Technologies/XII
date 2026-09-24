/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Material/MaterialTypes.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMaterialDomain, 1)
  XII_ENUM_CONSTANTS(xiiMaterialDomain::Surface, xiiMaterialDomain::Decal, xiiMaterialDomain::Volume, xiiMaterialDomain::PostProcess, xiiMaterialDomain::Sensor, xiiMaterialDomain::Compute)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMaterialShadingModel, 2)
  XII_ENUM_CONSTANTS(xiiMaterialShadingModel::Lit, xiiMaterialShadingModel::Subsurface, xiiMaterialShadingModel::ClearCoat, xiiMaterialShadingModel::Cloth)
  XII_ENUM_CONSTANTS(xiiMaterialShadingModel::Hair, xiiMaterialShadingModel::Eye, xiiMaterialShadingModel::Unlit, xiiMaterialShadingModel::ParticipatingMedia)
  XII_ENUM_CONSTANTS(xiiMaterialShadingModel::XRayAttenuation, xiiMaterialShadingModel::SensorResponse, xiiMaterialShadingModel::Custom)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMaterialBlendMode, 1)
  XII_ENUM_CONSTANTS(xiiMaterialBlendMode::Opaque, xiiMaterialBlendMode::Masked, xiiMaterialBlendMode::Translucent, xiiMaterialBlendMode::Additive, xiiMaterialBlendMode::Modulate)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMaterialAlphaMode, 1)
  XII_ENUM_CONSTANTS(xiiMaterialAlphaMode::Opaque, xiiMaterialAlphaMode::Mask, xiiMaterialAlphaMode::Blend)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiMaterialFeatureFlags, 2)
  XII_BITFLAGS_CONSTANTS(xiiMaterialFeatureFlags::NormalTexture, xiiMaterialFeatureFlags::MetallicRoughnessTexture, xiiMaterialFeatureFlags::OcclusionTexture, xiiMaterialFeatureFlags::EmissiveTexture)
  XII_BITFLAGS_CONSTANTS(xiiMaterialFeatureFlags::HeightTexture, xiiMaterialFeatureFlags::ClearCoat, xiiMaterialFeatureFlags::Transmission, xiiMaterialFeatureFlags::Sheen, xiiMaterialFeatureFlags::Anisotropy)
  XII_BITFLAGS_CONSTANTS(xiiMaterialFeatureFlags::VertexColor, xiiMaterialFeatureFlags::TwoSided, xiiMaterialFeatureFlags::RuntimeGenerated)
  XII_BITFLAGS_CONSTANTS(xiiMaterialFeatureFlags::ReceivesLighting, xiiMaterialFeatureFlags::CastsShadows, xiiMaterialFeatureFlags::WritesVelocity, xiiMaterialFeatureFlags::UsesBindlessResources)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMaterialTextureSlot, 1)
  XII_ENUM_CONSTANTS(xiiMaterialTextureSlot::BaseColor, xiiMaterialTextureSlot::Normal, xiiMaterialTextureSlot::MetallicRoughness, xiiMaterialTextureSlot::Occlusion)
  XII_ENUM_CONSTANTS(xiiMaterialTextureSlot::Emissive, xiiMaterialTextureSlot::Height, xiiMaterialTextureSlot::ClearCoat, xiiMaterialTextureSlot::Transmission)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMaterialParameterType, 1)
  XII_ENUM_CONSTANTS(xiiMaterialParameterType::Bool, xiiMaterialParameterType::Int, xiiMaterialParameterType::UInt, xiiMaterialParameterType::Float)
  XII_ENUM_CONSTANTS(xiiMaterialParameterType::Float2, xiiMaterialParameterType::Float3, xiiMaterialParameterType::Float4, xiiMaterialParameterType::Color)
  XII_ENUM_CONSTANTS(xiiMaterialParameterType::Matrix3, xiiMaterialParameterType::Matrix4)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiMaterialUpdateFrequency, 1)
  XII_ENUM_CONSTANTS(xiiMaterialUpdateFrequency::Static, xiiMaterialUpdateFrequency::PerFrame, xiiMaterialUpdateFrequency::PerView, xiiMaterialUpdateFrequency::PerInstance)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiMaterialParameterFlags, 1)
  XII_BITFLAGS_CONSTANTS(xiiMaterialParameterFlags::EditorVisible, xiiMaterialParameterFlags::Animatable, xiiMaterialParameterFlags::RuntimeWritable)
  XII_BITFLAGS_CONSTANTS(xiiMaterialParameterFlags::Specialization, xiiMaterialParameterFlags::Normalized, xiiMaterialParameterFlags::HighPrecision, xiiMaterialParameterFlags::NoSerialize)
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiMaterialDirtyFlags, 1)
  XII_BITFLAGS_CONSTANTS(xiiMaterialDirtyFlags::Parameters, xiiMaterialDirtyFlags::Resources, xiiMaterialDirtyFlags::Specialization, xiiMaterialDirtyFlags::Pipeline)
XII_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialParameterId, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialParameterId>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Value", m_uiValue),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialGpuHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialGpuHandle>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Slot", m_uiSlot),
      XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiMaterialRuntimeState, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiMaterialRuntimeState>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ENUM_MEMBER_PROPERTY("Domain", xiiMaterialDomain, m_Domain),
      XII_ENUM_MEMBER_PROPERTY("ShadingModel", xiiMaterialShadingModel, m_ShadingModel),
      XII_ENUM_MEMBER_PROPERTY("BlendMode", xiiMaterialBlendMode, m_BlendMode),
      XII_ENUM_MEMBER_PROPERTY("AlphaMode", xiiMaterialAlphaMode, m_AlphaMode),
      XII_BITFLAGS_MEMBER_PROPERTY("Features", xiiMaterialFeatureFlags, m_FeatureFlags),
      XII_MEMBER_PROPERTY("PipelineKey", m_uiPipelineKey),
      XII_MEMBER_PROPERTY("LayoutHash", m_uiLayoutHash),
      XII_MEMBER_PROPERTY("RuntimeHash", m_uiRuntimeHash),
      XII_MEMBER_PROPERTY("TextureMask", m_uiTextureMask),
      XII_MEMBER_PROPERTY("Revision", m_uiRevision),
      XII_MEMBER_PROPERTY("SortPriority", m_iSortPriority),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

xiiMaterialParameterId xiiMaterialParameterId::Make(xiiStringView sName)
{
  xiiMaterialParameterId id;
  id.m_uiValue = xiiHashingUtils::StringHash(sName);

  // Zero is reserved as an invalid sentinel.
  if (id.m_uiValue == 0U)
    id.m_uiValue = 1U;

  return id;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Material_Implementation_MaterialTypes);
