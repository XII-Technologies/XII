#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/VFX/VFXComponents.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

#define XII_VFX_REFLECT(T)                                           \
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(T, 1, xiiRTTIDefaultAllocator<T>) \
  XII_END_DYNAMIC_REFLECTED_TYPE
XII_VFX_REFLECT(xiiParticleEmitterRenderData);
XII_VFX_REFLECT(xiiRibbonEmitterRenderData);
XII_VFX_REFLECT(xiiVolumeRendererRenderData);
XII_VFX_REFLECT(xiiSparseVolumeRenderData);
XII_VFX_REFLECT(xiiVFXGraphRenderData);
XII_VFX_REFLECT(xiiLensFlareRenderData);

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiParticleEmitterComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("MaxParticles", GetMaxParticles, SetMaxParticles)->AddAttributes(new xiiDefaultValueAttribute(1000u)),
    XII_ACCESSOR_PROPERTY("Looping",      GetLooping,      SetLooping)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/VFX"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiRibbonEmitterComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Width",    GetWidth,    SetWidth)->AddAttributes(new xiiDefaultValueAttribute(0.1f)),
    XII_ACCESSOR_PROPERTY("Lifetime", GetLifetime, SetLifetime)->AddAttributes(new xiiDefaultValueAttribute(2.0f)),
    XII_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/VFX"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiVolumeRendererComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Density",      GetDensityFile,   SetDensityFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
    XII_ACCESSOR_PROPERTY("DensityScale", GetDensityScale,  SetDensityScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("MaxSteps",     GetMaxSteps,      SetMaxSteps)->AddAttributes(new xiiDefaultValueAttribute(128u)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/VFX"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiSparseVolumeComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("VoxelSize", GetVoxelSize, SetVoxelSize)->AddAttributes(new xiiDefaultValueAttribute(0.1f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/VFX"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiVFXGraphComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("GraphAsset", GetGraphAsset, SetGraphAsset),
    XII_ACCESSOR_PROPERTY("PlayRate",   GetPlayRate,   SetPlayRate)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("Paused",     GetPaused,     SetPaused),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/VFX"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiLensFlareComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Texture",   GetTextureFile, SetTextureFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    XII_ACCESSOR_PROPERTY("Intensity", GetIntensity,   SetIntensity)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("Size",      GetSize,        SetSize)->AddAttributes(new xiiDefaultValueAttribute(0.1f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS { XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/VFX"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

// --- Shared extract helper ---
#define XII_VFX_EXTRACT_BASE(ClassName, RDType)                                  \
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
    pRD->m_uiSortingKey    = GetUniqueIdForRendering();

#define XII_VFX_EXTRACT_END                                  \
  ref_msg.AddRenderData(pRD, xiiRenderData::Caching::Never); \
  }

// ========= xiiParticleEmitterComponent =========
xiiParticleEmitterComponent::xiiParticleEmitterComponent()  = default;
xiiParticleEmitterComponent::~xiiParticleEmitterComponent() = default;
void xiiParticleEmitterComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_uiMaxParticles << m_bLooping;
}
void xiiParticleEmitterComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_uiMaxParticles >> m_bLooping;
}
xiiResult xiiParticleEmitterComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m)
{
  XII_IGNORE_UNUSED(b);
  XII_IGNORE_UNUSED(m);
  bAV = true;
  return XII_SUCCESS;
}
void xiiParticleEmitterComponent::SetMaxParticles(xiiUInt32 n) { m_uiMaxParticles = xiiMath::Max(n, 1u); }
void xiiParticleEmitterComponent::SetLooping(bool b) { m_bLooping = b; }
void xiiParticleEmitterComponent::Play() { m_bPlaying = true; }
void xiiParticleEmitterComponent::Stop() { m_bPlaying = false; }
XII_VFX_EXTRACT_BASE(xiiParticleEmitterComponent, xiiParticleEmitterRenderData)
pRD->m_uiActiveParticles = m_bPlaying ? m_uiMaxParticles : 0;
pRD->m_fSimTime          = m_fSimTime;
XII_VFX_EXTRACT_END

// ========= xiiRibbonEmitterComponent =========
xiiRibbonEmitterComponent::xiiRibbonEmitterComponent()  = default;
xiiRibbonEmitterComponent::~xiiRibbonEmitterComponent() = default;
void xiiRibbonEmitterComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_uiSegments << m_fWidth << m_fLifetime << m_hMaterial;
}
void xiiRibbonEmitterComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_uiSegments >> m_fWidth >> m_fLifetime >> m_hMaterial;
}
xiiResult xiiRibbonEmitterComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m)
{
  XII_IGNORE_UNUSED(b);
  XII_IGNORE_UNUSED(m);
  bAV = true;
  return XII_SUCCESS;
}
void          xiiRibbonEmitterComponent::SetWidth(float f) { m_fWidth = xiiMath::Max(f, 0.0f); }
void          xiiRibbonEmitterComponent::SetLifetime(float f) { m_fLifetime = xiiMath::Max(f, 0.0f); }
void          xiiRibbonEmitterComponent::SetMaterialFile(xiiStringView s) { m_hMaterial = s.IsEmpty() ? xiiMaterialResourceHandle{} : xiiResourceManager::LoadResource<xiiMaterialResource>(s); }
xiiStringView xiiRibbonEmitterComponent::GetMaterialFile() const { return m_hMaterial.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hMaterial) : xiiStringView{}; }
XII_VFX_EXTRACT_BASE(xiiRibbonEmitterComponent, xiiRibbonEmitterRenderData)
pRD->m_uiSegments = m_uiSegments;
pRD->m_fWidth     = m_fWidth;
pRD->m_fLifetime  = m_fLifetime;
pRD->m_hMaterial  = m_hMaterial;
XII_VFX_EXTRACT_END

// ========= xiiVolumeRendererComponent =========
xiiVolumeRendererComponent::xiiVolumeRendererComponent()  = default;
xiiVolumeRendererComponent::~xiiVolumeRendererComponent() = default;
void xiiVolumeRendererComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_hDensity << m_fDensityScale << m_uiMaxSteps;
}
void xiiVolumeRendererComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_hDensity >> m_fDensityScale >> m_uiMaxSteps;
}
xiiResult xiiVolumeRendererComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m)
{
  XII_IGNORE_UNUSED(b);
  XII_IGNORE_UNUSED(m);
  bAV = true;
  return XII_SUCCESS;
}
void xiiVolumeRendererComponent::SetDensityFile(xiiStringView s)
{
  m_hDensity = s.IsEmpty() ? xiiTexture3DResourceHandle{} : xiiResourceManager::LoadResource<xiiTexture3DResource>(s);
  InvalidateCachedRenderData();
}
xiiStringView xiiVolumeRendererComponent::GetDensityFile() const { return m_hDensity.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hDensity) : xiiStringView{}; }
void          xiiVolumeRendererComponent::SetDensityScale(float f) { m_fDensityScale = xiiMath::Max(f, 0.0f); }
void          xiiVolumeRendererComponent::SetMaxSteps(xiiUInt16 n) { m_uiMaxSteps = xiiMath::Max<xiiUInt16>(n, 1); }
XII_VFX_EXTRACT_BASE(xiiVolumeRendererComponent, xiiVolumeRendererRenderData)
pRD->m_hDensity      = m_hDensity;
pRD->m_fDensityScale = m_fDensityScale;
pRD->m_uiMaxSteps    = m_uiMaxSteps;
XII_VFX_EXTRACT_END

// ========= xiiSparseVolumeComponent =========
xiiSparseVolumeComponent::xiiSparseVolumeComponent()  = default;
xiiSparseVolumeComponent::~xiiSparseVolumeComponent() = default;
void xiiSparseVolumeComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_fVoxelSize;
}
void xiiSparseVolumeComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_fVoxelSize;
}
xiiResult xiiSparseVolumeComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m)
{
  XII_IGNORE_UNUSED(b);
  XII_IGNORE_UNUSED(m);
  bAV = true;
  return XII_SUCCESS;
}
void xiiSparseVolumeComponent::SetVoxelSize(float f) { m_fVoxelSize = xiiMath::Max(f, 0.0001f); }
XII_VFX_EXTRACT_BASE(xiiSparseVolumeComponent, xiiSparseVolumeRenderData)
pRD->m_fVoxelSize = m_fVoxelSize;
XII_VFX_EXTRACT_END

// ========= xiiVFXGraphComponent =========
xiiVFXGraphComponent::xiiVFXGraphComponent()  = default;
xiiVFXGraphComponent::~xiiVFXGraphComponent() = default;
void xiiVFXGraphComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_sGraphAsset << m_fPlayRate << m_bPaused;
}
void xiiVFXGraphComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_sGraphAsset >> m_fPlayRate >> m_bPaused;
}
xiiResult xiiVFXGraphComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m)
{
  XII_IGNORE_UNUSED(b);
  XII_IGNORE_UNUSED(m);
  bAV = true;
  return XII_SUCCESS;
}
void xiiVFXGraphComponent::SetGraphAsset(xiiStringView s) { m_sGraphAsset = s; }
void xiiVFXGraphComponent::SetPlayRate(float f) { m_fPlayRate = xiiMath::Max(f, 0.0f); }
void xiiVFXGraphComponent::SetPaused(bool b) { m_bPaused = b; }
XII_VFX_EXTRACT_BASE(xiiVFXGraphComponent, xiiVFXGraphRenderData)
pRD->m_sGraphAsset = m_sGraphAsset;
pRD->m_fPlayRate   = m_fPlayRate;
pRD->m_bPaused     = m_bPaused;
XII_VFX_EXTRACT_END

// ========= xiiLensFlareComponent =========
xiiLensFlareComponent::xiiLensFlareComponent()  = default;
xiiLensFlareComponent::~xiiLensFlareComponent() = default;
void xiiLensFlareComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_hFlareTexture << m_fIntensity << m_fSize;
}
void xiiLensFlareComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_hFlareTexture >> m_fIntensity >> m_fSize;
}
xiiResult xiiLensFlareComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m)
{
  XII_IGNORE_UNUSED(b);
  XII_IGNORE_UNUSED(m);
  bAV = true;
  return XII_SUCCESS;
}
void xiiLensFlareComponent::SetTextureFile(xiiStringView s)
{
  m_hFlareTexture = s.IsEmpty() ? xiiTexture2DResourceHandle{} : xiiResourceManager::LoadResource<xiiTexture2DResource>(s);
  InvalidateCachedRenderData();
}
xiiStringView xiiLensFlareComponent::GetTextureFile() const { return m_hFlareTexture.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hFlareTexture) : xiiStringView{}; }
void          xiiLensFlareComponent::SetIntensity(float f) { m_fIntensity = xiiMath::Max(f, 0.0f); }
void          xiiLensFlareComponent::SetSize(float f) { m_fSize = xiiMath::Max(f, 0.0f); }
XII_VFX_EXTRACT_BASE(xiiLensFlareComponent, xiiLensFlareRenderData)
pRD->m_hFlareTexture = m_hFlareTexture;
pRD->m_fIntensity    = m_fIntensity;
pRD->m_fSize         = m_fSize;
XII_VFX_EXTRACT_END

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_VFX_Implementation_VFXComponents);
