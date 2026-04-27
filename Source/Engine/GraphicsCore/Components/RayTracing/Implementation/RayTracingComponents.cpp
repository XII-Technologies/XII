#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/RayTracing/RayTracingComponents.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

#define XII_RT_REFLECT(T)                                            \
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(T, 1, xiiRTTIDefaultAllocator<T>) \
  XII_END_DYNAMIC_REFLECTED_TYPE
XII_RT_REFLECT(xiiRayTracingGeometryRenderData);
XII_RT_REFLECT(xiiAccelerationStructureRenderData);
XII_RT_REFLECT(xiiGPUDrivenRenderData);
XII_RT_REFLECT(xiiAsyncComputeRenderData);
XII_RT_REFLECT(xiiRayQueryRenderData);

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiRayTracingGeometryComponent, 1, xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES { XII_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
  XII_ACCESSOR_PROPERTY("Dynamic", GetDynamic, SetDynamic), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/RayTracing"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiAccelerationStructureComponent, 1, xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES { XII_ACCESSOR_PROPERTY("TopLevel", GetTopLevel, SetTopLevel)->AddAttributes(new xiiDefaultValueAttribute(true)),
  XII_ACCESSOR_PROPERTY("Compacted", GetCompacted, SetCompacted)->AddAttributes(new xiiDefaultValueAttribute(true)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/RayTracing"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiGPUDrivenComponent, 1, xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES { XII_ACCESSOR_PROPERTY("MaxDraws", GetMaxDraws, SetMaxDraws)->AddAttributes(new xiiDefaultValueAttribute(1024u)),
  XII_ACCESSOR_PROPERTY("FrustumCulling", GetFrustumCulling, SetFrustumCulling)->AddAttributes(new xiiDefaultValueAttribute(true)),
  XII_ACCESSOR_PROPERTY("OcclusionCulling", GetOcclusionCulling, SetOcclusionCulling)->AddAttributes(new xiiDefaultValueAttribute(true)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GPU"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiAsyncComputeComponent, 1, xiiComponentMode::Static)
{ XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/GPU"); } XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiRayQueryComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES { XII_ACCESSOR_PROPERTY("MaxT", GetMaxT, SetMaxT)->AddAttributes(new xiiDefaultValueAttribute(100.0f)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/RayTracing"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
// clang-format on

#define XII_RT_BOUNDS_ALWAYS(N)                                                               \
  xiiResult N::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) \
  {                                                                                           \
    XII_IGNORE_UNUSED(b);                                                                     \
    XII_IGNORE_UNUSED(m);                                                                     \
    bAV = true;                                                                               \
    return XII_SUCCESS;                                                                       \
  }
#define XII_RT_EXTRACT(ClassName, RDType, ...)                                   \
  void ClassName::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const \
  {                                                                              \
    if (!ref_msg.m_pView || !ref_msg.m_pExtractedRenderData) return;             \
    auto* pWM = GetWorld()->GetModule<xiiRenderWorldModule>();                   \
    if (!pWM) return;                                                            \
    auto* pRD              = pWM->CreateRenderDataForThisFrame<RDType>(this);    \
    pRD->m_GlobalTransform = GetOwner()->GetGlobalTransform();                   \
    pRD->m_GlobalBounds    = GetOwner()->GetGlobalBounds();                      \
    pRD->m_hOwnerObject    = GetOwner()->GetHandle();                            \
    pRD->m_hOwnerComponent = GetHandle();                                        \
    pRD->m_uiSortingKey    = GetUniqueIdForRendering();                          \
    __VA_ARGS__ ref_msg.AddRenderData(pRD, xiiRenderData::Caching::Never);       \
  }

// xiiRayTracingGeometryComponent
xiiRayTracingGeometryComponent::xiiRayTracingGeometryComponent()  = default;
xiiRayTracingGeometryComponent::~xiiRayTracingGeometryComponent() = default;
XII_RT_BOUNDS_ALWAYS(xiiRayTracingGeometryComponent)
void xiiRayTracingGeometryComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_hMesh << m_bDynamic << m_uiGeometryFlags;
}
void xiiRayTracingGeometryComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_hMesh >> m_bDynamic >> m_uiGeometryFlags;
}
void xiiRayTracingGeometryComponent::SetMeshFile(xiiStringView f)
{
  m_hMesh = f.IsEmpty() ? xiiMeshResourceHandle{} : xiiResourceManager::LoadResource<xiiMeshResource>(f);
  InvalidateCachedRenderData();
}
xiiStringView xiiRayTracingGeometryComponent::GetMeshFile() const { return m_hMesh.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hMesh) : xiiStringView{}; }
void          xiiRayTracingGeometryComponent::SetDynamic(bool b) { m_bDynamic = b; }
XII_RT_EXTRACT(xiiRayTracingGeometryComponent, xiiRayTracingGeometryRenderData, pRD->m_hMesh = m_hMesh; pRD->m_bDynamic = m_bDynamic; pRD->m_uiGeometryFlags = m_uiGeometryFlags;)

// xiiAccelerationStructureComponent
xiiAccelerationStructureComponent::xiiAccelerationStructureComponent()  = default;
xiiAccelerationStructureComponent::~xiiAccelerationStructureComponent() = default;
XII_RT_BOUNDS_ALWAYS(xiiAccelerationStructureComponent)
void xiiAccelerationStructureComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_bTopLevel << m_bCompacted;
}
void xiiAccelerationStructureComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_bTopLevel >> m_bCompacted;
}
void xiiAccelerationStructureComponent::SetTopLevel(bool b) { m_bTopLevel = b; }
void xiiAccelerationStructureComponent::SetCompacted(bool b) { m_bCompacted = b; }
XII_RT_EXTRACT(xiiAccelerationStructureComponent, xiiAccelerationStructureRenderData, pRD->m_bTopLevel = m_bTopLevel; pRD->m_bCompacted = m_bCompacted;)

// xiiGPUDrivenComponent
xiiGPUDrivenComponent::xiiGPUDrivenComponent()  = default;
xiiGPUDrivenComponent::~xiiGPUDrivenComponent() = default;
XII_RT_BOUNDS_ALWAYS(xiiGPUDrivenComponent)
void xiiGPUDrivenComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_uiMaxDraws << m_bFrustumCulling << m_bOcclusionCulling;
}
void xiiGPUDrivenComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_uiMaxDraws >> m_bFrustumCulling >> m_bOcclusionCulling;
}
void xiiGPUDrivenComponent::SetMaxDraws(xiiUInt32 n) { m_uiMaxDraws = xiiMath::Max(n, 1u); }
void xiiGPUDrivenComponent::SetFrustumCulling(bool b) { m_bFrustumCulling = b; }
void xiiGPUDrivenComponent::SetOcclusionCulling(bool b) { m_bOcclusionCulling = b; }
XII_RT_EXTRACT(xiiGPUDrivenComponent, xiiGPUDrivenRenderData, pRD->m_uiMaxDraws = m_uiMaxDraws; pRD->m_bFrustumCulling = m_bFrustumCulling; pRD->m_bOcclusionCulling = m_bOcclusionCulling;)

// xiiAsyncComputeComponent
xiiAsyncComputeComponent::xiiAsyncComputeComponent()  = default;
xiiAsyncComputeComponent::~xiiAsyncComputeComponent() = default;
XII_RT_BOUNDS_ALWAYS(xiiAsyncComputeComponent)
void xiiAsyncComputeComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_sPassName << m_uiTGX << m_uiTGY << m_uiTGZ;
}
void xiiAsyncComputeComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_sPassName >> m_uiTGX >> m_uiTGY >> m_uiTGZ;
}
void xiiAsyncComputeComponent::SetPassName(xiiStringView s) { m_sPassName = s; }
void xiiAsyncComputeComponent::SetDispatch(xiiUInt32 x, xiiUInt32 y, xiiUInt32 z)
{
  m_uiTGX = x;
  m_uiTGY = y;
  m_uiTGZ = z;
}
XII_RT_EXTRACT(xiiAsyncComputeComponent, xiiAsyncComputeRenderData, pRD->m_sPassName = m_sPassName; pRD->m_uiThreadGroupsX = m_uiTGX; pRD->m_uiThreadGroupsY = m_uiTGY; pRD->m_uiThreadGroupsZ = m_uiTGZ;)

// xiiRayQueryComponent
xiiRayQueryComponent::xiiRayQueryComponent()  = default;
xiiRayQueryComponent::~xiiRayQueryComponent() = default;
XII_RT_BOUNDS_ALWAYS(xiiRayQueryComponent)
void xiiRayQueryComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_fMaxT;
}
void xiiRayQueryComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_fMaxT;
}
void xiiRayQueryComponent::SetMaxT(float f) { m_fMaxT = xiiMath::Max(f, 0.0f); }
XII_RT_EXTRACT(xiiRayQueryComponent, xiiRayQueryRenderData, pRD->m_vRayOrigin = GetOwner()->GetGlobalPosition(); pRD->m_vRayDir = GetOwner()->GetGlobalRotation() * xiiVec3(0, 0, -1); pRD->m_fMaxT = m_fMaxT;)

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_RayTracing_Implementation_RayTracingComponents);
