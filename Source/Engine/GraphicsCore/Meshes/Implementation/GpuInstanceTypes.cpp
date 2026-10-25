/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Meshes/GpuInstanceTypes.h>
// clang-format off
XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGpuInstanceFlags, 1)
  XII_BITFLAGS_CONSTANTS(xiiGpuInstanceFlags::Enabled, xiiGpuInstanceFlags::Static, xiiGpuInstanceFlags::CastShadows, xiiGpuInstanceFlags::ReceiveShadows)
  XII_BITFLAGS_CONSTANTS(xiiGpuInstanceFlags::AlwaysVisible, xiiGpuInstanceFlags::Occluder, xiiGpuInstanceFlags::Transparent, xiiGpuInstanceFlags::SensorVisible)
  XII_BITFLAGS_CONSTANTS(xiiGpuInstanceFlags::RayTracingVisible, xiiGpuInstanceFlags::TwoSided)
XII_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuSceneInstance, xiiNoBase, 7, xiiRTTIDefaultAllocator<xiiGpuSceneInstance>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("GlobalTransform", m_GlobalTransform),
      XII_MEMBER_PROPERTY("PreviousGlobalTransform", m_PreviousGlobalTransform),
      XII_MEMBER_PROPERTY("NormalTransformRow0", m_NormalTransformRow0),
      XII_MEMBER_PROPERTY("NormalTransformRow1", m_NormalTransformRow1),
      XII_MEMBER_PROPERTY("NormalTransformRow2", m_NormalTransformRow2),
      XII_MEMBER_PROPERTY("BoundsCenterRadius", m_BoundsCenterRadius),
      XII_MEMBER_PROPERTY("LodSelectionIndex", m_uiLodSelectionIndex),
      XII_MEMBER_PROPERTY("BoundsExtents", m_BoundsExtents),
      XII_MEMBER_PROPERTY("GeometryIndex", m_uiGeometryIndex),
      XII_MEMBER_PROPERTY("MaterialIndex", m_uiMaterialIndex),
      XII_MEMBER_PROPERTY("ObjectId", m_uiObjectId),
      XII_MEMBER_PROPERTY("Flags", m_uiFlags),
      XII_MEMBER_PROPERTY("VisibilityMask", m_uiVisibilityMask),
      XII_MEMBER_PROPERTY("UserData", m_uiUserData),
      XII_MEMBER_PROPERTY("FirstPrimitive", m_uiFirstPrimitive),
      XII_MEMBER_PROPERTY("PrimitiveCount", m_uiPrimitiveCount),
      XII_MEMBER_PROPERTY("ForcedLod", m_uiForcedLod),
      XII_MEMBER_PROPERTY("DrawLod", m_uiDrawLod),
      XII_MEMBER_PROPERTY("LodBias", m_fLodBias),
      XII_MEMBER_PROPERTY("SkinningOffset", m_uiSkinningOffset),
      XII_MEMBER_PROPERTY("SkinningCount", m_uiSkinningCount),
      XII_MEMBER_PROPERTY("PreviousSkinningOffset", m_uiPreviousSkinningOffset),
      XII_MEMBER_PROPERTY("PreviousSkinningCount", m_uiPreviousSkinningCount),
      XII_MEMBER_PROPERTY("BoneIndicesOffset", m_uiBoneIndicesOffset),
      XII_MEMBER_PROPERTY("BoneWeightsOffset", m_uiBoneWeightsOffset),
      XII_MEMBER_PROPERTY("SkinningVertexStride", m_uiSkinningVertexStride),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuInstanceUploadRange, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuInstanceUploadRange>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("FirstInstance", m_uiFirstInstance),
      XII_MEMBER_PROPERTY("InstanceCount", m_uiInstanceCount),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_GpuInstanceTypes);
