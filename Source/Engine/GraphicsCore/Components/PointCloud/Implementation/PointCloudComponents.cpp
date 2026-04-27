#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/PointCloud/PointCloudComponents.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GAL/Device/GALDevice.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPointCloudRenderData,         1, xiiRTTIDefaultAllocator<xiiPointCloudRenderData>)         XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGaussianSplatRenderData,      1, xiiRTTIDefaultAllocator<xiiGaussianSplatRenderData>)      XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLiDARPointBufferRenderData,   1, xiiRTTIDefaultAllocator<xiiLiDARPointBufferRenderData>)   XII_END_DYNAMIC_REFLECTED_TYPE;

// ---- Shared macros ----
#define XII_PC_BOUNDS_LARGE(Comp) xiiResult Comp::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m){ XII_IGNORE_UNUSED(m); bAV=false; b=m_Bounds.IsValid()?m_Bounds:xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(),1e5f)); return XII_SUCCESS;}
#define XII_PC_EXTRACT_BEGIN(Comp, RDT) void Comp::OnMsgExtractRenderData(xiiMsgExtractRenderData& r) const { if(!r.m_pView||!r.m_pExtractedRenderData) return; auto* pWM=GetWorld()->GetModule<xiiRenderWorldModule>(); if(!pWM) return; auto* pRD=pWM->CreateRenderDataForThisFrame<RDT>(this); pRD->m_GlobalTransform=GetOwner()->GetGlobalTransform(); pRD->m_GlobalBounds=GetOwner()->GetGlobalBounds(); pRD->m_hOwnerObject=GetOwner()->GetHandle(); pRD->m_hOwnerComponent=GetHandle(); pRD->m_uiSortingKey=GetUniqueIdForRendering();
#define XII_PC_EXTRACT_END  r.AddRenderData(pRD, xiiRenderData::Caching::Never); }

// =========================================================
//  xiiPointCloudComponent
// =========================================================
// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiPointCloudComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES{
    XII_ACCESSOR_PROPERTY("SplatSize",       GetSplatSize,       SetSplatSize)->AddAttributes(new xiiDefaultValueAttribute(2.0f)),
    XII_ACCESSOR_PROPERTY("ColorByClass",    GetColorByClass,    SetColorByClass)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_ACCESSOR_PROPERTY("ColorByIntensity",GetColorByIntensity,SetColorByIntensity)->AddAttributes(new xiiDefaultValueAttribute(false)),
  } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/PointCloud"), } XII_END_ATTRIBUTES; }
XII_END_COMPONENT_TYPE;
// clang-format on

xiiPointCloudComponent::xiiPointCloudComponent()  = default;
xiiPointCloudComponent::~xiiPointCloudComponent()
{
  xiiGALDevice* pDev = xiiGALDevice::GetDefaultDevice();
  if (pDev && m_hPointBuffer.IsValid()) pDev->DestroyBuffer(m_hPointBuffer);
}

void xiiPointCloudComponent::SerializeComponent(xiiWorldWriter& s) const  { SUPER::SerializeComponent(s); s.GetStream() << m_fSplatSize << m_bColorByClass << m_bColorByIntensity; }
void xiiPointCloudComponent::DeserializeComponent(xiiWorldReader& s)      { SUPER::DeserializeComponent(s); s.GetStream() >> m_fSplatSize >> m_bColorByClass >> m_bColorByIntensity; }

xiiResult xiiPointCloudComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m)
{ XII_IGNORE_UNUSED(m); bAV=false; b=m_Bounds.IsValid()?m_Bounds:xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(),1e5f)); return XII_SUCCESS; }

void xiiPointCloudComponent::EnsureBufferCapacity(xiiUInt32 uiRequired)
{
  if (uiRequired <= m_uiBufferCapacity) return;
  xiiGALDevice* pDev = xiiGALDevice::GetDefaultDevice(); if (!pDev) return;
  if (m_hPointBuffer.IsValid()) pDev->DestroyBuffer(m_hPointBuffer);
  xiiGALBufferCreationDescription bd;
  bd.m_sDebugName = "PointCloudBuffer";
  bd.m_uiSize = uiRequired * sizeof(xiiPointCloudPoint);
  bd.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  bd.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
  bd.m_CPUAccessFlags = xiiGALCPUAccessFlags::Write;
  bd.m_Mode = xiiGALBufferMode::Structured;
  bd.m_uiElementByteStride = sizeof(xiiPointCloudPoint);
  m_hPointBuffer = pDev->CreateBuffer(bd);
  m_uiBufferCapacity = uiRequired;
}

void xiiPointCloudComponent::SetPointBuffer(xiiGALBufferHandle h, xiiUInt32 n, const xiiBoundingBoxSphere& bounds)
{ m_hPointBuffer=h; m_uiPointCount=n; m_Bounds=bounds; TriggerLocalBoundsUpdate(); InvalidateCachedRenderData(); }

xiiPointCloudPoint* xiiPointCloudComponent::BeginWritePoints(xiiUInt32 n)
{
  EnsureBufferCapacity(n); m_uiPointCount=n;
  xiiGALDevice* pDev = xiiGALDevice::GetDefaultDevice();
  if (!pDev || !m_hPointBuffer.IsValid()) return nullptr;
  return static_cast<xiiPointCloudPoint*>(pDev->MapBuffer(m_hPointBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard));
}
void xiiPointCloudComponent::EndWritePoints(const xiiBoundingBoxSphere& bounds)
{
  xiiGALDevice* pDev = xiiGALDevice::GetDefaultDevice();
  if (pDev && m_hPointBuffer.IsValid()) pDev->UnmapBuffer(m_hPointBuffer, xiiGALMapType::Write);
  m_Bounds=bounds; TriggerLocalBoundsUpdate(); InvalidateCachedRenderData();
}

void xiiPointCloudComponent::SetSplatSize(float f)       { m_fSplatSize=xiiMath::Max(f,0.1f); InvalidateCachedRenderData(); }
void xiiPointCloudComponent::SetColorByClass(bool b)     { m_bColorByClass=b; InvalidateCachedRenderData(); }
void xiiPointCloudComponent::SetColorByIntensity(bool b) { m_bColorByIntensity=b; InvalidateCachedRenderData(); }

XII_PC_EXTRACT_BEGIN(xiiPointCloudComponent, xiiPointCloudRenderData)
  pRD->m_hPointBuffer       = m_hPointBuffer;
  pRD->m_uiPointCount       = m_uiPointCount;
  pRD->m_fSplatSize         = m_fSplatSize;
  pRD->m_bColorByClass      = m_bColorByClass;
  pRD->m_bColorByIntensity  = m_bColorByIntensity;
XII_PC_EXTRACT_END

// =========================================================
//  xiiGaussianSplattingComponent
// =========================================================
// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiGaussianSplattingComponent, 1, xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES{
    XII_ACCESSOR_PROPERTY("SplatFile", GetSplatFile, SetSplatFile),
    XII_ACCESSOR_PROPERTY("SHDegree",  GetSHDegree,  SetSHDegree)->AddAttributes(new xiiDefaultValueAttribute((xiiUInt8)3), new xiiClampValueAttribute((xiiUInt8)0,(xiiUInt8)3)),
  } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/PointCloud"), } XII_END_ATTRIBUTES; }
XII_END_COMPONENT_TYPE;
// clang-format on

xiiGaussianSplattingComponent::xiiGaussianSplattingComponent()  = default;
xiiGaussianSplattingComponent::~xiiGaussianSplattingComponent() = default;

void xiiGaussianSplattingComponent::SerializeComponent(xiiWorldWriter& s) const  { SUPER::SerializeComponent(s); s.GetStream() << m_sSplatFile << m_uiSHDegree; }
void xiiGaussianSplattingComponent::DeserializeComponent(xiiWorldReader& s)      { SUPER::DeserializeComponent(s); s.GetStream() >> m_sSplatFile >> m_uiSHDegree; }
xiiResult xiiGaussianSplattingComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m)
{ XII_IGNORE_UNUSED(m); bAV=false; b=m_Bounds.IsValid()?m_Bounds:xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(),1.0f)); return XII_SUCCESS; }

void xiiGaussianSplattingComponent::SetSplatFile(xiiStringView s) { m_sSplatFile=s; /* load splat binary and fill GPU buffer */ InvalidateCachedRenderData(); TriggerLocalBoundsUpdate(); }
xiiStringView xiiGaussianSplattingComponent::GetSplatFile() const { return m_sSplatFile; }
void xiiGaussianSplattingComponent::SetSHDegree(xiiUInt8 n)      { m_uiSHDegree=xiiMath::Clamp<xiiUInt8>(n,0,3); InvalidateCachedRenderData(); }

XII_PC_EXTRACT_BEGIN(xiiGaussianSplattingComponent, xiiGaussianSplatRenderData)
  pRD->m_hSplatBuffer = m_hSplatBuffer;
  pRD->m_hSortedKeys  = m_hSortedKeys;
  pRD->m_uiSplatCount = m_uiSplatCount;
  pRD->m_uiSHDegree   = m_uiSHDegree;
XII_PC_EXTRACT_END

// =========================================================
//  xiiLiDARPointBufferComponent
// =========================================================
// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiLiDARPointBufferComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES{
    XII_ACCESSOR_PROPERTY("MaxCapacity", GetMaxCapacity, SetMaxCapacity)->AddAttributes(new xiiDefaultValueAttribute(1000000u)),
    XII_ACCESSOR_PROPERTY("MaxRange",    GetMaxRange,    SetMaxRange)->AddAttributes(new xiiDefaultValueAttribute(100.0f)),
    XII_ACCESSOR_PROPERTY("SplatSize",   GetSplatSize,   SetSplatSize)->AddAttributes(new xiiDefaultValueAttribute(1.5f)),
  } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/PointCloud"), } XII_END_ATTRIBUTES; }
XII_END_COMPONENT_TYPE;
// clang-format on

xiiLiDARPointBufferComponent::xiiLiDARPointBufferComponent()  = default;
xiiLiDARPointBufferComponent::~xiiLiDARPointBufferComponent()
{
  xiiGALDevice* pDev = xiiGALDevice::GetDefaultDevice();
  if (pDev) { for (auto& h : m_hBuffers) if (h.IsValid()) pDev->DestroyBuffer(h); }
}

void xiiLiDARPointBufferComponent::SerializeComponent(xiiWorldWriter& s) const  { SUPER::SerializeComponent(s); s.GetStream() << m_uiCapacity << m_fMaxRange << m_fSplatSize; }
void xiiLiDARPointBufferComponent::DeserializeComponent(xiiWorldReader& s)      { SUPER::DeserializeComponent(s); s.GetStream() >> m_uiCapacity >> m_fMaxRange >> m_fSplatSize; }
xiiResult xiiLiDARPointBufferComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m)
{ XII_IGNORE_UNUSED(m); bAV=true; b=xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(),m_fMaxRange)); return XII_SUCCESS; }

void xiiLiDARPointBufferComponent::AllocateBuffers()
{
  xiiGALDevice* pDev = xiiGALDevice::GetDefaultDevice(); if (!pDev) return;
  for (auto& h : m_hBuffers) { if (h.IsValid()) pDev->DestroyBuffer(h); }
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    xiiGALBufferCreationDescription bd;
    bd.m_sDebugName = i==0 ? "LiDAR_BufferA" : "LiDAR_BufferB";
    bd.m_uiSize = m_uiCapacity * sizeof(xiiPointCloudPoint);
    bd.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    bd.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
    bd.m_CPUAccessFlags = xiiGALCPUAccessFlags::Write;
    bd.m_Mode = xiiGALBufferMode::Structured;
    bd.m_uiElementByteStride = sizeof(xiiPointCloudPoint);
    m_hBuffers[i] = pDev->CreateBuffer(bd);
  }
}

void xiiLiDARPointBufferComponent::SetMaxCapacity(xiiUInt32 n) { m_uiCapacity=xiiMath::Max(n,1u); AllocateBuffers(); }
void xiiLiDARPointBufferComponent::SetMaxRange(float f)        { m_fMaxRange=xiiMath::Max(f,1.0f); TriggerLocalBoundsUpdate(); }
void xiiLiDARPointBufferComponent::SetSplatSize(float f)       { m_fSplatSize=xiiMath::Max(f,0.5f); InvalidateCachedRenderData(); }

void xiiLiDARPointBufferComponent::SwapBuffers(xiiUInt32 uiReturnCount)
{
  xiiMath::Swap(m_uiReadIdx, m_uiWriteIdx);
  m_uiReturnCount = xiiMath::Min(uiReturnCount, m_uiCapacity);
  InvalidateCachedRenderData();
}

XII_PC_EXTRACT_BEGIN(xiiLiDARPointBufferComponent, xiiLiDARPointBufferRenderData)
  pRD->m_hCurrentBuffer  = m_hBuffers[m_uiReadIdx];
  pRD->m_hPreviousBuffer = m_hBuffers[m_uiWriteIdx];
  pRD->m_uiReturnCount   = m_uiReturnCount;
  pRD->m_fMaxRange       = m_fMaxRange;
  pRD->m_fSplatSize      = m_fSplatSize;
XII_PC_EXTRACT_END

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_PointCloud_Implementation_PointCloudComponents);
