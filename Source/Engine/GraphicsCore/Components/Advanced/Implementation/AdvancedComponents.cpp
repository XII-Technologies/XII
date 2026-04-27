#include <GraphicsCore/GraphicsCorePCH.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Advanced/AdvancedComponents.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

#define XII_ADV_REFLECT(T) XII_BEGIN_DYNAMIC_REFLECTED_TYPE(T,1,xiiRTTIDefaultAllocator<T>) XII_END_DYNAMIC_REFLECTED_TYPE
XII_ADV_REFLECT(xiiSpectralMaterialRenderData);
XII_ADV_REFLECT(xiiHybridPathTracerRenderData);
XII_ADV_REFLECT(xiiNeuralMaterialRenderData);
XII_ADV_REFLECT(xiiDifferentiableRenderData);
XII_ADV_REFLECT(xiiProceduralTerrainRenderData);
XII_ADV_REFLECT(xiiClothRenderData);
XII_ADV_REFLECT(xiiHairFurRenderData);

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSpectralMaterialComponent,1,xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES{ XII_ACCESSOR_PROPERTY("Dispersion",GetDispersion,SetDispersion)->AddAttributes(new xiiDefaultValueAttribute(0.01f)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData,OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/Advanced"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiHybridPathTracerComponent,1,xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES{ XII_ACCESSOR_PROPERTY("SPP",GetSPP,SetSPP)->AddAttributes(new xiiDefaultValueAttribute(1u)),
  XII_ACCESSOR_PROPERTY("MaxBounces",GetMaxBounces,SetMaxBounces)->AddAttributes(new xiiDefaultValueAttribute(4u)),
  XII_ACCESSOR_PROPERTY("FireflyClamp",GetFireflyClamp,SetFireflyClamp)->AddAttributes(new xiiDefaultValueAttribute(10.0f)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData,OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/Advanced"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiNeuralMaterialComponent,1,xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES{ XII_ACCESSOR_PROPERTY("ModelPath",GetModelPath,SetModelPath),
  XII_ACCESSOR_PROPERTY("LatentDim",GetLatentDim,SetLatentDim)->AddAttributes(new xiiDefaultValueAttribute(32u)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData,OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/Advanced"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiDifferentiableRenderComponent,1,xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES{ XII_ACCESSOR_PROPERTY("Enabled",GetEnabled,SetEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData,OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/Advanced"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiProceduralTerrainComponent,1,xiiComponentMode::Static)
{ XII_BEGIN_PROPERTIES{ XII_ACCESSOR_PROPERTY("Amplitude",GetAmplitude,SetAmplitude)->AddAttributes(new xiiDefaultValueAttribute(100.0f)),
  XII_ACCESSOR_PROPERTY("Frequency",GetFrequency,SetFrequency)->AddAttributes(new xiiDefaultValueAttribute(0.01f)),
  XII_ACCESSOR_PROPERTY("Octaves",GetOctaves,SetOctaves)->AddAttributes(new xiiDefaultValueAttribute(6u)), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData,OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/Advanced"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiClothComponent,1,xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES{ XII_ACCESSOR_PROPERTY("Stiffness",GetStiffness,SetStiffness)->AddAttributes(new xiiDefaultValueAttribute(0.8f)),
  XII_ACCESSOR_PROPERTY("Material",GetMaterialFile,SetMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData,OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/Advanced"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
XII_BEGIN_COMPONENT_TYPE(xiiHairFurComponent,1,xiiComponentMode::Dynamic)
{ XII_BEGIN_PROPERTIES{ XII_ACCESSOR_PROPERTY("Length",GetLength,SetLength)->AddAttributes(new xiiDefaultValueAttribute(0.05f)),
  XII_ACCESSOR_PROPERTY("Thickness",GetThickness,SetThickness)->AddAttributes(new xiiDefaultValueAttribute(0.001f)),
  XII_ACCESSOR_PROPERTY("StrandCount",GetStrandCount,SetStrandCount)->AddAttributes(new xiiDefaultValueAttribute(10000u)),
  XII_ACCESSOR_PROPERTY("BaseColor",GetBaseColor,SetBaseColor), } XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS{ XII_MESSAGE_HANDLER(xiiMsgExtractRenderData,OnMsgExtractRenderData), } XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES{ new xiiCategoryAttribute("Rendering/Advanced"); } XII_END_ATTRIBUTES; } XII_END_COMPONENT_TYPE;
// clang-format on

#define XII_ADV_BOUNDS(N) xiiResult N::GetLocalBounds(xiiBoundingBoxSphere& b,bool& bAV,xiiMsgUpdateLocalBounds& m){XII_IGNORE_UNUSED(b);XII_IGNORE_UNUSED(m);bAV=true;return XII_SUCCESS;}
#define XII_ADV_EXTRACT(N,RDT,...) void N::OnMsgExtractRenderData(xiiMsgExtractRenderData& r) const { if(!r.m_pView||!r.m_pExtractedRenderData)return; auto*pWM=GetWorld()->GetModule<xiiRenderWorldModule>();if(!pWM)return; auto*pRD=pWM->CreateRenderDataForThisFrame<RDT>(this); pRD->m_GlobalTransform=GetOwner()->GetGlobalTransform();pRD->m_GlobalBounds=GetOwner()->GetGlobalBounds();pRD->m_hOwnerObject=GetOwner()->GetHandle();pRD->m_hOwnerComponent=GetHandle();pRD->m_uiSortingKey=GetUniqueIdForRendering(); __VA_ARGS__ r.AddRenderData(pRD,xiiRenderData::Caching::Never);}

xiiSpectralMaterialComponent::xiiSpectralMaterialComponent()=default; xiiSpectralMaterialComponent::~xiiSpectralMaterialComponent()=default;
XII_ADV_BOUNDS(xiiSpectralMaterialComponent)
void xiiSpectralMaterialComponent::SerializeComponent(xiiWorldWriter& s) const{SUPER::SerializeComponent(s);s.GetStream()<<m_uiWavelengthSamples<<m_fDispersion;}
void xiiSpectralMaterialComponent::DeserializeComponent(xiiWorldReader& s){SUPER::DeserializeComponent(s);s.GetStream()>>m_uiWavelengthSamples>>m_fDispersion;}
void xiiSpectralMaterialComponent::SetDispersion(float f){m_fDispersion=xiiMath::Max(f,0.0f);}
XII_ADV_EXTRACT(xiiSpectralMaterialComponent,xiiSpectralMaterialRenderData, pRD->m_uiWavelengthSamples=m_uiWavelengthSamples; pRD->m_fDispersion=m_fDispersion;)

xiiHybridPathTracerComponent::xiiHybridPathTracerComponent()=default; xiiHybridPathTracerComponent::~xiiHybridPathTracerComponent()=default;
XII_ADV_BOUNDS(xiiHybridPathTracerComponent)
void xiiHybridPathTracerComponent::SerializeComponent(xiiWorldWriter& s) const{SUPER::SerializeComponent(s);s.GetStream()<<m_uiSPP<<m_uiMaxBounces<<m_fFireflyClamp;}
void xiiHybridPathTracerComponent::DeserializeComponent(xiiWorldReader& s){SUPER::DeserializeComponent(s);s.GetStream()>>m_uiSPP>>m_uiMaxBounces>>m_fFireflyClamp;}
void xiiHybridPathTracerComponent::SetSPP(xiiUInt16 n)       {m_uiSPP=xiiMath::Max<xiiUInt16>(n,1);}
void xiiHybridPathTracerComponent::SetMaxBounces(xiiUInt8 n)  {m_uiMaxBounces=xiiMath::Clamp<xiiUInt8>(n,1,32);}
void xiiHybridPathTracerComponent::SetFireflyClamp(float f)   {m_fFireflyClamp=xiiMath::Max(f,0.0f);}
XII_ADV_EXTRACT(xiiHybridPathTracerComponent,xiiHybridPathTracerRenderData, pRD->m_uiSPP=m_uiSPP; pRD->m_uiMaxBounces=m_uiMaxBounces; pRD->m_fFireflyClamp=m_fFireflyClamp;)

xiiNeuralMaterialComponent::xiiNeuralMaterialComponent()=default; xiiNeuralMaterialComponent::~xiiNeuralMaterialComponent()=default;
XII_ADV_BOUNDS(xiiNeuralMaterialComponent)
void xiiNeuralMaterialComponent::SerializeComponent(xiiWorldWriter& s) const{SUPER::SerializeComponent(s);s.GetStream()<<m_sModelPath<<m_uiLatentDim;}
void xiiNeuralMaterialComponent::DeserializeComponent(xiiWorldReader& s){SUPER::DeserializeComponent(s);s.GetStream()>>m_sModelPath>>m_uiLatentDim;}
void xiiNeuralMaterialComponent::SetModelPath(xiiStringView s){m_sModelPath=s;}
void xiiNeuralMaterialComponent::SetLatentDim(xiiUInt16 n){m_uiLatentDim=xiiMath::Max<xiiUInt16>(n,1);}
XII_ADV_EXTRACT(xiiNeuralMaterialComponent,xiiNeuralMaterialRenderData, pRD->m_sModelPath=m_sModelPath; pRD->m_uiLatentDim=m_uiLatentDim;)

xiiDifferentiableRenderComponent::xiiDifferentiableRenderComponent()=default; xiiDifferentiableRenderComponent::~xiiDifferentiableRenderComponent()=default;
XII_ADV_BOUNDS(xiiDifferentiableRenderComponent)
void xiiDifferentiableRenderComponent::SerializeComponent(xiiWorldWriter& s) const{SUPER::SerializeComponent(s);s.GetStream()<<m_uiDerivOrder<<m_bEnabled;}
void xiiDifferentiableRenderComponent::DeserializeComponent(xiiWorldReader& s){SUPER::DeserializeComponent(s);s.GetStream()>>m_uiDerivOrder>>m_bEnabled;}
void xiiDifferentiableRenderComponent::SetEnabled(bool b){m_bEnabled=b;}
XII_ADV_EXTRACT(xiiDifferentiableRenderComponent,xiiDifferentiableRenderData, pRD->m_uiDerivOrder=m_uiDerivOrder; pRD->m_bEnabled=m_bEnabled;)

xiiProceduralTerrainComponent::xiiProceduralTerrainComponent()=default; xiiProceduralTerrainComponent::~xiiProceduralTerrainComponent()=default;
XII_ADV_BOUNDS(xiiProceduralTerrainComponent)
void xiiProceduralTerrainComponent::SerializeComponent(xiiWorldWriter& s) const{SUPER::SerializeComponent(s);s.GetStream()<<m_fAmplitude<<m_fFrequency<<m_uiOctaves;}
void xiiProceduralTerrainComponent::DeserializeComponent(xiiWorldReader& s){SUPER::DeserializeComponent(s);s.GetStream()>>m_fAmplitude>>m_fFrequency>>m_uiOctaves;}
void xiiProceduralTerrainComponent::SetAmplitude(float f){m_fAmplitude=xiiMath::Max(f,0.0f);TriggerLocalBoundsUpdate();}
void xiiProceduralTerrainComponent::SetFrequency(float f){m_fFrequency=xiiMath::Max(f,0.00001f);}
void xiiProceduralTerrainComponent::SetOctaves(xiiUInt8 n){m_uiOctaves=xiiMath::Clamp<xiiUInt8>(n,1,16);}
XII_ADV_EXTRACT(xiiProceduralTerrainComponent,xiiProceduralTerrainRenderData, pRD->m_fAmplitude=m_fAmplitude; pRD->m_fFrequency=m_fFrequency; pRD->m_uiOctaves=m_uiOctaves;)

xiiClothComponent::xiiClothComponent()=default; xiiClothComponent::~xiiClothComponent()=default;
XII_ADV_BOUNDS(xiiClothComponent)
void xiiClothComponent::SerializeComponent(xiiWorldWriter& s) const{SUPER::SerializeComponent(s);s.GetStream()<<m_fStiffness<<m_hMaterial;}
void xiiClothComponent::DeserializeComponent(xiiWorldReader& s){SUPER::DeserializeComponent(s);s.GetStream()>>m_fStiffness>>m_hMaterial;}
void xiiClothComponent::SetStiffness(float f){m_fStiffness=xiiMath::Clamp(f,0.0f,1.0f);}
void xiiClothComponent::SetMaterialFile(xiiStringView s){m_hMaterial=s.IsEmpty()?xiiMaterialResourceHandle{}:xiiResourceManager::LoadResource<xiiMaterialResource>(s);}
xiiStringView xiiClothComponent::GetMaterialFile() const{return m_hMaterial.IsValid()?xiiResourceManager::GetResourceIDOrDescription(m_hMaterial):xiiStringView{};}
XII_ADV_EXTRACT(xiiClothComponent,xiiClothRenderData, pRD->m_fStiffness=m_fStiffness; pRD->m_hMaterial=m_hMaterial;)

xiiHairFurComponent::xiiHairFurComponent()=default; xiiHairFurComponent::~xiiHairFurComponent()=default;
XII_ADV_BOUNDS(xiiHairFurComponent)
void xiiHairFurComponent::SerializeComponent(xiiWorldWriter& s) const{SUPER::SerializeComponent(s);s.GetStream()<<m_fLength<<m_fThickness<<m_uiStrandCount<<m_BaseColor;}
void xiiHairFurComponent::DeserializeComponent(xiiWorldReader& s){SUPER::DeserializeComponent(s);s.GetStream()>>m_fLength>>m_fThickness>>m_uiStrandCount>>m_BaseColor;}
void xiiHairFurComponent::SetLength(float f)      {m_fLength=xiiMath::Max(f,0.0f);}
void xiiHairFurComponent::SetThickness(float f)   {m_fThickness=xiiMath::Max(f,0.0f);}
void xiiHairFurComponent::SetStrandCount(xiiUInt32 n){m_uiStrandCount=xiiMath::Max(n,1u);}
void xiiHairFurComponent::SetBaseColor(const xiiColor& c){m_BaseColor=c;}
XII_ADV_EXTRACT(xiiHairFurComponent,xiiHairFurRenderData, pRD->m_fLength=m_fLength; pRD->m_fThickness=m_fThickness; pRD->m_uiStrandCount=m_uiStrandCount; pRD->m_BaseColor=m_BaseColor;)

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Advanced_Implementation_AdvancedComponents);
