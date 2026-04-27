#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Science/ScienceComponents.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GAL/Device/GALDevice.h>

// ---- RTTI ----
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAtomSphereRenderData,    1, xiiRTTIDefaultAllocator<xiiAtomSphereRenderData>)    XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBondCylinderRenderData,  1, xiiRTTIDefaultAllocator<xiiBondCylinderRenderData>)  XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiIsosurfaceRenderData,    1, xiiRTTIDefaultAllocator<xiiIsosurfaceRenderData>)    XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTrajectoryLineRenderData,1, xiiRTTIDefaultAllocator<xiiTrajectoryLineRenderData>)XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScalarFieldRenderData,   1, xiiRTTIDefaultAllocator<xiiScalarFieldRenderData>)   XII_END_DYNAMIC_REFLECTED_TYPE;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVectorFieldRenderData,   1, xiiRTTIDefaultAllocator<xiiVectorFieldRenderData>)   XII_END_DYNAMIC_REFLECTED_TYPE;

// ---- Shared extract helper ----
#define XII_SCI_EXTRACT_BEGIN(CompType, RDType)                                                   \
  void CompType::OnMsgExtractRenderData(xiiMsgExtractRenderData& r) const {                       \
    if (!r.m_pView || !r.m_pExtractedRenderData) return;                                          \
    auto* pWM = GetWorld()->GetModule<xiiRenderWorldModule>(); if (!pWM) return;                  \
    auto* pRD = pWM->CreateRenderDataForThisFrame<RDType>(this);                                  \
    pRD->m_GlobalTransform = GetOwner()->GetGlobalTransform();                                    \
    pRD->m_GlobalBounds    = GetOwner()->GetGlobalBounds();                                       \
    pRD->m_hOwnerObject    = GetOwner()->GetHandle();                                             \
    pRD->m_hOwnerComponent = GetHandle();                                                         \
    pRD->m_uiSortingKey    = GetUniqueIdForRendering();

#define XII_SCI_EXTRACT_END(cache)  r.AddRenderData(pRD, xiiRenderData::Caching::cache); }

// =========================================================
//  xiiAtomSphereComponent
// =========================================================
// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiAtomSphereComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES{
    XII_ACCESSOR_PROPERTY("RadiusScale",    GetRadiusScale,    SetRadiusScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("ColorByElement", GetColorByElement, SetColorByElement)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("ColorByVelocity",GetColorByVelocity,SetColorByVelocity)->AddAttributes(new xiiDefaultValueAttribute(false)),
    XII_ACCESSOR_PROPERTY("UseImpostors",   GetUseImpostors,   SetUseImpostors)->AddAttributes(new xiiDefaultValueAttribute(true)),
  } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Science/Molecular"), } XII_END_ATTRIBUTES; }
XII_END_COMPONENT_TYPE;
// clang-format on

xiiAtomSphereComponent::xiiAtomSphereComponent()  = default;
xiiAtomSphereComponent::~xiiAtomSphereComponent() = default;

void xiiAtomSphereComponent::SerializeComponent(xiiWorldWriter& s) const   { SUPER::SerializeComponent(s); s.GetStream() << m_fRadiusScale << m_bColorByElement << m_bColorByVelocity << m_bUseImpostors << m_bHasCustomBounds; if (m_bHasCustomBounds) s.GetStream() << m_CustomBounds; }
void xiiAtomSphereComponent::DeserializeComponent(xiiWorldReader& s)       { SUPER::DeserializeComponent(s); s.GetStream() >> m_fRadiusScale >> m_bColorByElement >> m_bColorByVelocity >> m_bUseImpostors >> m_bHasCustomBounds; if (m_bHasCustomBounds) s.GetStream() >> m_CustomBounds; }

xiiResult xiiAtomSphereComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m)
{ XII_IGNORE_UNUSED(m); bAV = false;
  if (m_bHasCustomBounds) { b = m_CustomBounds; return XII_SUCCESS; }
  b = xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), 1e5f)); return XII_SUCCESS; }

void xiiAtomSphereComponent::SetAtomBuffer(xiiGALBufferHandle h, xiiUInt32 n)   { m_hAtomBuffer = h; m_uiAtomCount = n; InvalidateCachedRenderData(); }
void xiiAtomSphereComponent::SetRadiusScale(float f)                             { m_fRadiusScale = xiiMath::Max(f, 0.001f); InvalidateCachedRenderData(); }
void xiiAtomSphereComponent::SetColorByElement(bool b)                           { m_bColorByElement = b; InvalidateCachedRenderData(); }
void xiiAtomSphereComponent::SetColorByVelocity(bool b)                          { m_bColorByVelocity = b; InvalidateCachedRenderData(); }
void xiiAtomSphereComponent::SetUseImpostors(bool b)                             { m_bUseImpostors = b; InvalidateCachedRenderData(); }
void xiiAtomSphereComponent::SetCustomBounds(const xiiBoundingBoxSphere& b)      { m_CustomBounds = b; m_bHasCustomBounds = true; TriggerLocalBoundsUpdate(); }

XII_SCI_EXTRACT_BEGIN(xiiAtomSphereComponent, xiiAtomSphereRenderData)
  pRD->m_hAtomBuffer      = m_hAtomBuffer;
  pRD->m_uiAtomCount      = m_uiAtomCount;
  pRD->m_fRadiusScale     = m_fRadiusScale;
  pRD->m_bColorByElement  = m_bColorByElement;
  pRD->m_bColorByVelocity = m_bColorByVelocity;
  pRD->m_bUseImpostors    = m_bUseImpostors;
XII_SCI_EXTRACT_END(Never)

// =========================================================
//  xiiBondCylinderComponent
// =========================================================
// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiBondCylinderComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES{ XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(0.1f)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Science/Molecular"), } XII_END_ATTRIBUTES; }
XII_END_COMPONENT_TYPE;
// clang-format on

xiiBondCylinderComponent::xiiBondCylinderComponent()  = default;
xiiBondCylinderComponent::~xiiBondCylinderComponent() = default;

void xiiBondCylinderComponent::SerializeComponent(xiiWorldWriter& s) const  { SUPER::SerializeComponent(s); s.GetStream() << m_fRadius; }
void xiiBondCylinderComponent::DeserializeComponent(xiiWorldReader& s)      { SUPER::DeserializeComponent(s); s.GetStream() >> m_fRadius; }
xiiResult xiiBondCylinderComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) { XII_IGNORE_UNUSED(m); bAV=false; b=xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(),1e5f)); return XII_SUCCESS; }

void xiiBondCylinderComponent::SetBondBuffer(xiiGALBufferHandle hB, xiiGALBufferHandle hA, xiiUInt32 n) { m_hBondBuffer=hB; m_hAtomBuffer=hA; m_uiBondCount=n; InvalidateCachedRenderData(); }
void xiiBondCylinderComponent::SetRadius(float f) { m_fRadius = xiiMath::Max(f, 0.001f); InvalidateCachedRenderData(); }

XII_SCI_EXTRACT_BEGIN(xiiBondCylinderComponent, xiiBondCylinderRenderData)
  pRD->m_hBondBuffer = m_hBondBuffer;
  pRD->m_hAtomBuffer = m_hAtomBuffer;
  pRD->m_uiBondCount = m_uiBondCount;
  pRD->m_fRadius     = m_fRadius;
XII_SCI_EXTRACT_END(Never)

// =========================================================
//  xiiIsosurfaceComponent
// =========================================================
// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiIsosurfaceComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES{
    XII_ACCESSOR_PROPERTY("DensityField", GetDensityFieldFile, SetDensityFieldFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture3D")),
    XII_ACCESSOR_PROPERTY("Material",     GetMaterialFile,     SetMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_ACCESSOR_PROPERTY("Isovalue",     GetIsovalue,         SetIsovalue)->AddAttributes(new xiiDefaultValueAttribute(0.5f)),
  } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Science/Volumetric"), } XII_END_ATTRIBUTES; }
XII_END_COMPONENT_TYPE;
// clang-format on

xiiIsosurfaceComponent::xiiIsosurfaceComponent()  = default;
xiiIsosurfaceComponent::~xiiIsosurfaceComponent() = default;

void xiiIsosurfaceComponent::SerializeComponent(xiiWorldWriter& s) const  { SUPER::SerializeComponent(s); s.GetStream() << m_hDensityField << m_hMaterial << m_fIsovalue << m_uiGridRes[0] << m_uiGridRes[1] << m_uiGridRes[2]; }
void xiiIsosurfaceComponent::DeserializeComponent(xiiWorldReader& s)      { SUPER::DeserializeComponent(s); s.GetStream() >> m_hDensityField >> m_hMaterial >> m_fIsovalue >> m_uiGridRes[0] >> m_uiGridRes[1] >> m_uiGridRes[2]; }
xiiResult xiiIsosurfaceComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) { XII_IGNORE_UNUSED(m); bAV=false; b=xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), 0.5f*xiiVec3((float)m_uiGridRes[0],(float)m_uiGridRes[1],(float)m_uiGridRes[2]).GetLength())); return XII_SUCCESS; }

void xiiIsosurfaceComponent::SetDensityFieldFile(xiiStringView s) { m_hDensityField = s.IsEmpty() ? xiiTexture3DResourceHandle{} : xiiResourceManager::LoadResource<xiiTexture3DResource>(s); InvalidateCachedRenderData(); }
xiiStringView xiiIsosurfaceComponent::GetDensityFieldFile() const { return m_hDensityField.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hDensityField) : xiiStringView{}; }
void xiiIsosurfaceComponent::SetMaterialFile(xiiStringView s)     { m_hMaterial = s.IsEmpty() ? xiiMaterialResourceHandle{} : xiiResourceManager::LoadResource<xiiMaterialResource>(s); InvalidateCachedRenderData(); }
xiiStringView xiiIsosurfaceComponent::GetMaterialFile() const     { return m_hMaterial.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hMaterial) : xiiStringView{}; }
void xiiIsosurfaceComponent::SetIsovalue(float f)                 { m_fIsovalue = f; InvalidateCachedRenderData(); }
void xiiIsosurfaceComponent::SetGridResolution(xiiUInt32 x, xiiUInt32 y, xiiUInt32 z) { m_uiGridRes[0]=x; m_uiGridRes[1]=y; m_uiGridRes[2]=z; TriggerLocalBoundsUpdate(); }

XII_SCI_EXTRACT_BEGIN(xiiIsosurfaceComponent, xiiIsosurfaceRenderData)
  pRD->m_hDensityField   = m_hDensityField;
  pRD->m_hMaterial       = m_hMaterial;
  pRD->m_fIsovalue       = m_fIsovalue;
  pRD->m_uiGridRes[0]    = m_uiGridRes[0];
  pRD->m_uiGridRes[1]    = m_uiGridRes[1];
  pRD->m_uiGridRes[2]    = m_uiGridRes[2];
XII_SCI_EXTRACT_END(Never)

// =========================================================
//  xiiTrajectoryLineComponent
// =========================================================
// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiTrajectoryLineComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES{
    XII_ACCESSOR_PROPERTY("HistoryLength", GetHistoryLength, SetHistoryLength)->AddAttributes(new xiiDefaultValueAttribute(1024u)),
    XII_ACCESSOR_PROPERTY("LineWidth",     GetLineWidth,     SetLineWidth)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("StartColor",    GetStartColor,    SetStartColor),
    XII_ACCESSOR_PROPERTY("EndColor",      GetEndColor,      SetEndColor),
  } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Science/Simulation"), } XII_END_ATTRIBUTES; }
XII_END_COMPONENT_TYPE;
// clang-format on

xiiTrajectoryLineComponent::xiiTrajectoryLineComponent()  = default;
xiiTrajectoryLineComponent::~xiiTrajectoryLineComponent()
{
  xiiGALDevice* pDev = xiiGALDevice::GetDefaultDevice();
  if (pDev && m_hPositionRing.IsValid()) pDev->DestroyBuffer(m_hPositionRing);
}

void xiiTrajectoryLineComponent::SerializeComponent(xiiWorldWriter& s) const  { SUPER::SerializeComponent(s); s.GetStream() << m_uiHistoryLength << m_fLineWidth << m_StartColor << m_EndColor; }
void xiiTrajectoryLineComponent::DeserializeComponent(xiiWorldReader& s)      { SUPER::DeserializeComponent(s); s.GetStream() >> m_uiHistoryLength >> m_fLineWidth >> m_StartColor >> m_EndColor; }
xiiResult xiiTrajectoryLineComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) { XII_IGNORE_UNUSED(m); bAV=true; b=xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(),1e6f)); return XII_SUCCESS; }

void xiiTrajectoryLineComponent::SetHistoryLength(xiiUInt32 n)
{
  if (n == m_uiHistoryLength) return;
  m_uiHistoryLength = xiiMath::Max(n, 2u);
  xiiGALDevice* pDev = xiiGALDevice::GetDefaultDevice();
  if (!pDev) return;
  if (m_hPositionRing.IsValid()) pDev->DestroyBuffer(m_hPositionRing);
  xiiGALBufferCreationDescription bd;
  bd.m_sDebugName = "TrajectoryRingBuffer";
  bd.m_uiSize = m_uiHistoryLength * sizeof(xiiVec3);
  bd.m_BindFlags = xiiGALBindFlags::ShaderResource;
  bd.m_ResourceUsage = xiiGALResourceUsage::Dynamic;
  bd.m_CPUAccessFlags = xiiGALCPUAccessFlags::Write;
  m_hPositionRing = pDev->CreateBuffer(bd);
  m_uiWriteHead = m_uiLiveCount = 0;
}

void xiiTrajectoryLineComponent::AppendPosition(const xiiVec3& pos)
{
  xiiGALDevice* pDev = xiiGALDevice::GetDefaultDevice();
  if (!pDev || !m_hPositionRing.IsValid()) return;
  xiiVec3* p = static_cast<xiiVec3*>(pDev->MapBuffer(m_hPositionRing, xiiGALMapType::Write, xiiGALMapFlags::NoOverwrite));
  if (p) { p[m_uiWriteHead] = pos; pDev->UnmapBuffer(m_hPositionRing, xiiGALMapType::Write); }
  m_uiWriteHead = (m_uiWriteHead + 1) % m_uiHistoryLength;
  m_uiLiveCount = xiiMath::Min(m_uiLiveCount + 1, m_uiHistoryLength);
  InvalidateCachedRenderData();
}

void xiiTrajectoryLineComponent::SetLineWidth(float f) { m_fLineWidth = xiiMath::Max(f,0.1f); InvalidateCachedRenderData(); }
void xiiTrajectoryLineComponent::SetStartColor(const xiiColor& c) { m_StartColor = c; InvalidateCachedRenderData(); }
void xiiTrajectoryLineComponent::SetEndColor(const xiiColor& c)   { m_EndColor   = c; InvalidateCachedRenderData(); }
void xiiTrajectoryLineComponent::Clear() { m_uiWriteHead = m_uiLiveCount = 0; }

XII_SCI_EXTRACT_BEGIN(xiiTrajectoryLineComponent, xiiTrajectoryLineRenderData)
  pRD->m_hPositionRing = m_hPositionRing;
  pRD->m_uiCapacity    = m_uiHistoryLength;
  pRD->m_uiWriteHead   = m_uiWriteHead;
  pRD->m_uiLiveCount   = m_uiLiveCount;
  pRD->m_fLineWidth    = m_fLineWidth;
  pRD->m_StartColor    = m_StartColor;
  pRD->m_EndColor      = m_EndColor;
XII_SCI_EXTRACT_END(Never)

// =========================================================
//  xiiScalarFieldComponent
// =========================================================
// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiScalarFieldComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES{
    XII_ACCESSOR_PROPERTY("ScalarField",      GetFieldFile,           SetFieldFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture3D")),
    XII_ACCESSOR_PROPERTY("TransferFunction", GetTransferFunctionFile,SetTransferFunctionFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture2D")),
    XII_ACCESSOR_PROPERTY("DensityScale",     GetDensityScale,        SetDensityScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("StepSize",         GetStepSize,            SetStepSize)->AddAttributes(new xiiDefaultValueAttribute(0.01f)),
    XII_ACCESSOR_PROPERTY("MaxSteps",         GetMaxSteps,            SetMaxSteps)->AddAttributes(new xiiDefaultValueAttribute((xiiUInt16)512)),
  } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Science/Volumetric"), } XII_END_ATTRIBUTES; }
XII_END_COMPONENT_TYPE;
// clang-format on

xiiScalarFieldComponent::xiiScalarFieldComponent()  = default;
xiiScalarFieldComponent::~xiiScalarFieldComponent() = default;

void xiiScalarFieldComponent::SerializeComponent(xiiWorldWriter& s) const  { SUPER::SerializeComponent(s); s.GetStream() << m_hField << m_hTransferFunction << m_fDensityScale << m_fStepSize << m_uiMaxSteps; }
void xiiScalarFieldComponent::DeserializeComponent(xiiWorldReader& s)      { SUPER::DeserializeComponent(s); s.GetStream() >> m_hField >> m_hTransferFunction >> m_fDensityScale >> m_fStepSize >> m_uiMaxSteps; }
xiiResult xiiScalarFieldComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) { XII_IGNORE_UNUSED(m); bAV=false; b=xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(),1.0f)); return XII_SUCCESS; }

void xiiScalarFieldComponent::SetFieldFile(xiiStringView s)              { m_hField = s.IsEmpty() ? xiiTexture3DResourceHandle{} : xiiResourceManager::LoadResource<xiiTexture3DResource>(s); InvalidateCachedRenderData(); }
xiiStringView xiiScalarFieldComponent::GetFieldFile() const              { return m_hField.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hField) : xiiStringView{}; }
void xiiScalarFieldComponent::SetTransferFunctionFile(xiiStringView s)   { m_hTransferFunction = s.IsEmpty() ? xiiTexture2DResourceHandle{} : xiiResourceManager::LoadResource<xiiTexture2DResource>(s); InvalidateCachedRenderData(); }
xiiStringView xiiScalarFieldComponent::GetTransferFunctionFile() const   { return m_hTransferFunction.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hTransferFunction) : xiiStringView{}; }
void xiiScalarFieldComponent::SetDensityScale(float f)                   { m_fDensityScale = xiiMath::Max(f,0.0f); InvalidateCachedRenderData(); }
void xiiScalarFieldComponent::SetStepSize(float f)                       { m_fStepSize = xiiMath::Max(f,0.0001f); InvalidateCachedRenderData(); }
void xiiScalarFieldComponent::SetMaxSteps(xiiUInt16 n)                   { m_uiMaxSteps = xiiMath::Max<xiiUInt16>(n,1); InvalidateCachedRenderData(); }

XII_SCI_EXTRACT_BEGIN(xiiScalarFieldComponent, xiiScalarFieldRenderData)
  pRD->m_hField            = m_hField;
  pRD->m_hTransferFunction = m_hTransferFunction;
  pRD->m_fDensityScale     = m_fDensityScale;
  pRD->m_fStepSize         = m_fStepSize;
  pRD->m_uiMaxSteps        = m_uiMaxSteps;
XII_SCI_EXTRACT_END(Never)

// =========================================================
//  xiiVectorFieldComponent
// =========================================================
// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiVectorFieldComponent, 1, xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES{
    XII_ACCESSOR_PROPERTY("VectorField",  GetFieldFile,    SetFieldFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture3D")),
    XII_ACCESSOR_PROPERTY("ArrowDensity", GetArrowDensity, SetArrowDensity)->AddAttributes(new xiiDefaultValueAttribute(16u)),
    XII_ACCESSOR_PROPERTY("ArrowScale",   GetArrowScale,   SetArrowScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("BaseColor",    GetBaseColor,    SetBaseColor),
  } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Science/Simulation"), } XII_END_ATTRIBUTES; }
XII_END_COMPONENT_TYPE;
// clang-format on

xiiVectorFieldComponent::xiiVectorFieldComponent()  = default;
xiiVectorFieldComponent::~xiiVectorFieldComponent() = default;

void xiiVectorFieldComponent::SerializeComponent(xiiWorldWriter& s) const  { SUPER::SerializeComponent(s); s.GetStream() << m_hField << m_uiArrowDensity << m_fArrowScale << m_BaseColor; }
void xiiVectorFieldComponent::DeserializeComponent(xiiWorldReader& s)      { SUPER::DeserializeComponent(s); s.GetStream() >> m_hField >> m_uiArrowDensity >> m_fArrowScale >> m_BaseColor; }
xiiResult xiiVectorFieldComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m) { XII_IGNORE_UNUSED(m); bAV=false; b=xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(),1.0f)); return XII_SUCCESS; }

void xiiVectorFieldComponent::SetFieldFile(xiiStringView s)     { m_hField = s.IsEmpty() ? xiiTexture3DResourceHandle{} : xiiResourceManager::LoadResource<xiiTexture3DResource>(s); InvalidateCachedRenderData(); }
xiiStringView xiiVectorFieldComponent::GetFieldFile() const     { return m_hField.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hField) : xiiStringView{}; }
void xiiVectorFieldComponent::SetArrowDensity(xiiUInt32 n)      { m_uiArrowDensity = xiiMath::Max(n,1u); InvalidateCachedRenderData(); }
void xiiVectorFieldComponent::SetArrowScale(float f)            { m_fArrowScale = xiiMath::Max(f,0.0f); InvalidateCachedRenderData(); }
void xiiVectorFieldComponent::SetBaseColor(const xiiColor& c)   { m_BaseColor = c; InvalidateCachedRenderData(); }

XII_SCI_EXTRACT_BEGIN(xiiVectorFieldComponent, xiiVectorFieldRenderData)
  pRD->m_hField         = m_hField;
  pRD->m_uiArrowDensity = m_uiArrowDensity;
  pRD->m_fArrowScale    = m_fArrowScale;
  pRD->m_BaseColor      = m_BaseColor;
XII_SCI_EXTRACT_END(Never)

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Science_Implementation_ScienceComponents);
