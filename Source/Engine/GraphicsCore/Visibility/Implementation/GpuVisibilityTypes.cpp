/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Visibility/GpuVisibilityTypes.h>

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVisibilityView, xiiNoBase, 3, xiiRTTIDefaultAllocator<xiiGpuVisibilityView>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("ViewProjectionMatrix", m_ViewProjectionMatrix),
      XII_MEMBER_PROPERTY("OcclusionViewProjectionMatrix", m_OcclusionViewProjectionMatrix),
      XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("FrustumPlanes", GetFrustumPlaneCount, GetFrustumPlane),
      XII_MEMBER_PROPERTY("CameraPosition", m_CameraPosition),
      XII_MEMBER_PROPERTY("ViewportAndHiZ", m_ViewportAndHiZ),
      XII_MEMBER_PROPERTY("LodParameters", m_LodParameters),
      XII_MEMBER_PROPERTY("InstanceCount", m_uiInstanceCount),
      XII_MEMBER_PROPERTY("VisibilityMask", m_uiVisibilityMask),
      XII_MEMBER_PROPERTY("RequiredFlags", m_uiRequiredFlags),
      XII_MEMBER_PROPERTY("ExcludedFlags", m_uiExcludedFlags),
      XII_MEMBER_PROPERTY("GeometryBaseIndex", m_uiGeometryBaseIndex),
      XII_MEMBER_PROPERTY("MaxVisibleMeshlets", m_uiMaxVisibleMeshlets),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Visibility_Implementation_GpuVisibilityTypes);
