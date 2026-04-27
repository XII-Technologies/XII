#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/LOD/LODComponents.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

#define XII_LOD_REFLECT(T)                                           \
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(T, 1, xiiRTTIDefaultAllocator<T>) \
  XII_END_DYNAMIC_REFLECTED_TYPE
XII_LOD_REFLECT(xiiLODGroupRenderData);
XII_LOD_REFLECT(xiiOcclusionRenderData);
XII_LOD_REFLECT(xiiStreamingHintRenderData);
XII_LOD_REFLECT(xiiBatchingHintRenderData);
XII_LOD_REFLECT(xiiDistanceFieldRenderData);

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiLODGroupComponent,1,xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES { XII_ACCESSOR_PROPERTY("LODFactor",GetLODFactor,SetLODFactor)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  XII_ACCESSOR_PROPERTY("NumLODs",GetNumLODs,SetNumLODs)->AddAttributes(new xiiDefaultValueAttribute(1u)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData,OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/LOD"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiOcclusionComponent,1,xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES { XII_ACCESSOR_PROPERTY("Occluder",GetOccluder,SetOccluder)->AddAttributes(new xiiDefaultValueAttribute(true)),
  XII_ACCESSOR_PROPERTY("Occludee",GetOccludee,SetOccludee)->AddAttributes(new xiiDefaultValueAttribute(true)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData,OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/Culling"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiStreamingHintComponent,1,xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES { XII_ACCESSOR_PROPERTY("Priority",GetPriority,SetPriority)->AddAttributes(new xiiDefaultValueAttribute(0.5f)),
  XII_ACCESSOR_PROPERTY("StreamRadius",GetStreamRadius,SetStreamRadius)->AddAttributes(new xiiDefaultValueAttribute(50.0f)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData,OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/Streaming"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiBatchingHintComponent,1,xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES { XII_ACCESSOR_PROPERTY("BatchKey",GetBatchKey,SetBatchKey),
  XII_ACCESSOR_PROPERTY("Eligible",GetEligible,SetEligible)->AddAttributes(new xiiDefaultValueAttribute(true)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData,OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/Batching"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiDistanceFieldComponent,1,xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES { XII_ACCESSOR_PROPERTY("SDF",GetSDFFile,SetSDFFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
  XII_ACCESSOR_PROPERTY("WorldScale",GetWorldScale,SetWorldScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData,OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/SDF"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
// clang-format on

#define XII_LOD_BOUNDS(N)                                                                     \
  xiiResult N::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) \
  {                                                                                           \
    XII_IGNORE_UNUSED(b);                                                                     \
    XII_IGNORE_UNUSED(m);                                                                     \
    bAV = true;                                                                               \
    return XII_SUCCESS;                                                                       \
  }
#define XII_LOD_EXTRACT(N, RDT, ...)                                       \
  void N::OnMsgExtractRenderData(xiiMsgExtractRenderData& r) const         \
  {                                                                        \
    if (!r.m_pView || !r.m_pExtractedRenderData) return;                   \
    auto* pWM = GetWorld()->GetModule<xiiRenderWorldModule>();             \
    if (!pWM) return;                                                      \
    auto* pRD              = pWM->CreateRenderDataForThisFrame<RDT>(this); \
    pRD->m_GlobalTransform = GetOwner()->GetGlobalTransform();             \
    pRD->m_GlobalBounds    = GetOwner()->GetGlobalBounds();                \
    pRD->m_hOwnerObject    = GetOwner()->GetHandle();                      \
    pRD->m_hOwnerComponent = GetHandle();                                  \
    pRD->m_uiSortingKey    = GetUniqueIdForRendering();                    \
    __VA_ARGS__ r.AddRenderData(pRD, xiiRenderData::Caching::IfStatic);    \
  }

xiiLODGroupComponent::xiiLODGroupComponent()  = default;
xiiLODGroupComponent::~xiiLODGroupComponent() = default;
XII_LOD_BOUNDS(xiiLODGroupComponent)
void xiiLODGroupComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_fLODFactor << m_uiActiveLOD << m_uiNumLODs;
}
void xiiLODGroupComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_fLODFactor >> m_uiActiveLOD >> m_uiNumLODs;
}
void xiiLODGroupComponent::SetLODFactor(float f) { m_fLODFactor = xiiMath::Max(f, 0.0f); }
void xiiLODGroupComponent::SetNumLODs(xiiUInt8 n) { m_uiNumLODs = xiiMath::Max<xiiUInt8>(n, 1); }
XII_LOD_EXTRACT(xiiLODGroupComponent, xiiLODGroupRenderData, pRD->m_uiActiveLOD = m_uiActiveLOD; pRD->m_fLODFactor = m_fLODFactor; pRD->m_uiNumLODs = m_uiNumLODs;)

xiiOcclusionComponent::xiiOcclusionComponent()  = default;
xiiOcclusionComponent::~xiiOcclusionComponent() = default;
XII_LOD_BOUNDS(xiiOcclusionComponent)
void xiiOcclusionComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_bOccluder << m_bOccludee;
}
void xiiOcclusionComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_bOccluder >> m_bOccludee;
}
void xiiOcclusionComponent::SetOccluder(bool b) { m_bOccluder = b; }
void xiiOcclusionComponent::SetOccludee(bool b) { m_bOccludee = b; }
XII_LOD_EXTRACT(xiiOcclusionComponent, xiiOcclusionRenderData, pRD->m_bOccluder = m_bOccluder; pRD->m_bOccludee = m_bOccludee;)

xiiStreamingHintComponent::xiiStreamingHintComponent()  = default;
xiiStreamingHintComponent::~xiiStreamingHintComponent() = default;
XII_LOD_BOUNDS(xiiStreamingHintComponent)
void xiiStreamingHintComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_fPriority << m_fStreamRadius;
}
void xiiStreamingHintComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_fPriority >> m_fStreamRadius;
}
void xiiStreamingHintComponent::SetPriority(float f) { m_fPriority = xiiMath::Clamp(f, 0.0f, 1.0f); }
void xiiStreamingHintComponent::SetStreamRadius(float f) { m_fStreamRadius = xiiMath::Max(f, 0.0f); }
XII_LOD_EXTRACT(xiiStreamingHintComponent, xiiStreamingHintRenderData, pRD->m_fPriority = m_fPriority; pRD->m_fStreamRadius = m_fStreamRadius;)

xiiBatchingHintComponent::xiiBatchingHintComponent()  = default;
xiiBatchingHintComponent::~xiiBatchingHintComponent() = default;
XII_LOD_BOUNDS(xiiBatchingHintComponent)
void xiiBatchingHintComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_uiBatchKey << m_bEligible;
}
void xiiBatchingHintComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_uiBatchKey >> m_bEligible;
}
void xiiBatchingHintComponent::SetBatchKey(xiiUInt32 k) { m_uiBatchKey = k; }
void xiiBatchingHintComponent::SetEligible(bool b) { m_bEligible = b; }
XII_LOD_EXTRACT(xiiBatchingHintComponent, xiiBatchingHintRenderData, pRD->m_uiBatchKey = m_uiBatchKey; pRD->m_bEligible = m_bEligible;)

xiiDistanceFieldComponent::xiiDistanceFieldComponent()  = default;
xiiDistanceFieldComponent::~xiiDistanceFieldComponent() = default;
XII_LOD_BOUNDS(xiiDistanceFieldComponent)
void xiiDistanceFieldComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_hSDF << m_fWorldScale;
}
void xiiDistanceFieldComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_hSDF >> m_fWorldScale;
}
void          xiiDistanceFieldComponent::SetSDFFile(xiiStringView f) { m_hSDF = f.IsEmpty() ? xiiTexture3DResourceHandle{} : xiiResourceManager::LoadResource<xiiTexture3DResource>(f); }
xiiStringView xiiDistanceFieldComponent::GetSDFFile() const { return m_hSDF.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hSDF) : xiiStringView{}; }
void          xiiDistanceFieldComponent::SetWorldScale(float f) { m_fWorldScale = xiiMath::Max(f, 0.0001f); }
XII_LOD_EXTRACT(xiiDistanceFieldComponent, xiiDistanceFieldRenderData, pRD->m_hSDF = m_hSDF; pRD->m_fWorldScale = m_fWorldScale;)

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_LOD_Implementation_LODComponents);
