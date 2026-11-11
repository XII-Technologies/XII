/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Meshes/VisibilityRenderSettings.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisibilityRenderSettings, 1, xiiRTTIDefaultAllocator<xiiVisibilityRenderSettings>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("LODHysteresis", m_fLODHysteresis)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiVisibilityViewPass", "LodHysteresis", 0), new xiiRenderSettingsBindingAttribute("xiiMatrixVisibilityViewPass", "LodHysteresis", 0), new xiiClampValueAttribute(0, 1)),
    XII_MEMBER_PROPERTY("LODScreenScale", m_fLODScreenScale)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiVisibilityViewPass", "LodScreenScale", 0), new xiiRenderSettingsBindingAttribute("xiiMatrixVisibilityViewPass", "LodScreenScale", 0), new xiiClampValueAttribute(0.001, 1000)),
    XII_MEMBER_PROPERTY("UseMeshShaders", m_bUseMeshShaders)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiGBufferRasterPass", "UseMeshShaders", 0), new xiiRenderSettingsBindingAttribute("xiiDepthRasterPass", "UseMeshShaders", 0), new xiiRenderSettingsBindingAttribute("xiiSurfaceQueryPass", "UseMeshShaders", 0)),
    XII_MEMBER_PROPERTY("OcclusionDepthBias", m_fOcclusionDepthBias)->AddAttributes(new xiiRenderSettingsBindingAttribute("xiiInstanceOcclusionPass", "DepthBias", 0), new xiiRenderSettingsBindingAttribute("xiiMeshletOcclusionPass", "DepthBias", 0), new xiiClampValueAttribute(0, 1)),
  } XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Visibility and Geometry"),
  } XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_VisibilityRenderSettings);
