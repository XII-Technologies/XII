#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Materials/ShaderAndMaterialComponents.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSSSProfile, 1)
  XII_ENUM_CONSTANTS(xiiSSSProfile::Skin, xiiSSSProfile::Wax, xiiSSSProfile::Marble, xiiSSSProfile::Foliage, xiiSSSProfile::Custom)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSSSRenderData, 1, xiiRTTIDefaultAllocator<xiiSSSRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiSubsurfaceScatteringComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("ScatterRadius", GetScatterRadius, SetScatterRadius)->AddAttributes(new xiiDefaultValueAttribute(0.1f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("ScatterColor",  GetScatterColor,  SetScatterColor),
    XII_ENUM_ACCESSOR_PROPERTY("Profile",  xiiSSSProfile, GetProfile, SetProfile),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Materials"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalMaterialRenderData, 1, xiiRTTIDefaultAllocator<xiiDecalMaterialRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiDecalMaterialComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("DecalMaterial", GetDecalMaterialFile, SetDecalMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Materials"); } XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderPermutationRenderData, 1, xiiRTTIDefaultAllocator<xiiShaderPermutationRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiShaderPermutationComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_ATTRIBUTES { new xiiCategoryAttribute("Rendering/Materials"); } XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_COMPONENT_TYPE;
// clang-format on

// ===== xiiSubsurfaceScatteringComponent =====

xiiSubsurfaceScatteringComponent::xiiSubsurfaceScatteringComponent()  = default;
xiiSubsurfaceScatteringComponent::~xiiSubsurfaceScatteringComponent() = default;

void xiiSubsurfaceScatteringComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_fScatterRadius << m_ScatterColor << m_Profile.GetValue();
}

void xiiSubsurfaceScatteringComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  xiiUInt8 p = 0;
  s.GetStream() >> m_fScatterRadius >> m_ScatterColor >> p;
  m_Profile = static_cast<xiiSSSProfile::Enum>(p);
}

xiiResult xiiSubsurfaceScatteringComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m)
{
  XII_IGNORE_UNUSED(b);
  XII_IGNORE_UNUSED(bAV);
  XII_IGNORE_UNUSED(m);
  return XII_FAILURE;
}

void xiiSubsurfaceScatteringComponent::SetScatterRadius(float f)
{
  m_fScatterRadius = xiiMath::Max(f, 0.0f);
  InvalidateCachedRenderData();
}
void xiiSubsurfaceScatteringComponent::SetScatterColor(const xiiColor& c)
{
  m_ScatterColor = c;
  InvalidateCachedRenderData();
}
void xiiSubsurfaceScatteringComponent::SetProfile(xiiEnum<xiiSSSProfile> p)
{
  m_Profile = p;
  InvalidateCachedRenderData();
}

void xiiSubsurfaceScatteringComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (!ref_msg.m_pView || !ref_msg.m_pExtractedRenderData) return;
  auto pWM = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (!pWM) return;
  auto* p              = pWM->CreateRenderDataForThisFrame<xiiSSSRenderData>(this);
  p->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  p->m_GlobalBounds    = GetOwner()->GetGlobalBounds();
  p->m_hOwnerObject    = GetOwner()->GetHandle();
  p->m_hOwnerComponent = GetHandle();
  p->m_ScatterColor    = m_ScatterColor;
  p->m_fScatterRadius  = m_fScatterRadius;
  p->m_Profile         = m_Profile;
  p->m_uiSortingKey    = GetUniqueIdForRendering();
  ref_msg.AddRenderData(p, xiiRenderData::Caching::IfStatic);
}

// ===== xiiDecalMaterialComponent =====

xiiDecalMaterialComponent::xiiDecalMaterialComponent()  = default;
xiiDecalMaterialComponent::~xiiDecalMaterialComponent() = default;

void xiiDecalMaterialComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_hDecalMaterial << m_uiDecalTypeFlags;
}
void xiiDecalMaterialComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_hDecalMaterial >> m_uiDecalTypeFlags;
}
xiiResult xiiDecalMaterialComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m)
{
  XII_IGNORE_UNUSED(b);
  XII_IGNORE_UNUSED(bAV);
  XII_IGNORE_UNUSED(m);
  return XII_FAILURE;
}

void xiiDecalMaterialComponent::SetDecalMaterialFile(xiiStringView sFile)
{
  m_hDecalMaterial = sFile.IsEmpty() ? xiiMaterialResourceHandle{} : xiiResourceManager::LoadResource<xiiMaterialResource>(sFile);
  InvalidateCachedRenderData();
}
xiiStringView xiiDecalMaterialComponent::GetDecalMaterialFile() const
{
  return m_hDecalMaterial.IsValid() ? xiiResourceManager::GetResourceIDOrDescription(m_hDecalMaterial) : xiiStringView{};
}
void xiiDecalMaterialComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (!ref_msg.m_pView || !ref_msg.m_pExtractedRenderData) return;
  auto pWM = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (!pWM) return;
  auto* p               = pWM->CreateRenderDataForThisFrame<xiiDecalMaterialRenderData>(this);
  p->m_GlobalTransform  = GetOwner()->GetGlobalTransform();
  p->m_GlobalBounds     = GetOwner()->GetGlobalBounds();
  p->m_hOwnerObject     = GetOwner()->GetHandle();
  p->m_hOwnerComponent  = GetHandle();
  p->m_hDecalMaterial   = m_hDecalMaterial;
  p->m_uiDecalTypeFlags = m_uiDecalTypeFlags;
  p->m_uiSortingKey     = GetUniqueIdForRendering();
  ref_msg.AddRenderData(p, xiiRenderData::Caching::IfStatic);
}

// ===== xiiShaderPermutationComponent =====

xiiShaderPermutationComponent::xiiShaderPermutationComponent()  = default;
xiiShaderPermutationComponent::~xiiShaderPermutationComponent() = default;

void xiiShaderPermutationComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << static_cast<xiiUInt32>(m_Permutations.GetCount());
  for (const auto& perm : m_Permutations)
  {
    s << perm.m_sName.GetString();
    s << perm.m_sValue.GetString();
  }
}

void xiiShaderPermutationComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto&     s = inout_stream.GetStream();
  xiiUInt32 n = 0;
  s >> n;
  m_Permutations.SetCount(n);
  for (auto& perm : m_Permutations)
  {
    xiiString sN, sV;
    s >> sN >> sV;
    perm.m_sName.Assign(sN);
    perm.m_sValue.Assign(sV);
  }
}

xiiResult xiiShaderPermutationComponent::GetLocalBounds(xiiBoundingBoxSphere& b, bool& bAV, xiiMsgUpdateLocalBounds& m)
{
  XII_IGNORE_UNUSED(b);
  XII_IGNORE_UNUSED(bAV);
  XII_IGNORE_UNUSED(m);
  return XII_FAILURE;
}

void xiiShaderPermutationComponent::SetPermutation(xiiStringView sKey, xiiStringView sValue)
{
  xiiTempHashedString key(sKey);
  for (auto& p : m_Permutations)
    if (p.m_sName == key)
    {
      p.m_sValue.Assign(sValue);
      InvalidateCachedRenderData();
      return;
    }
  auto& entry = m_Permutations.ExpandAndGetRef();
  entry.m_sName.Assign(sKey);
  entry.m_sValue.Assign(sValue);
  InvalidateCachedRenderData();
}

void xiiShaderPermutationComponent::ClearPermutations()
{
  m_Permutations.Clear();
  InvalidateCachedRenderData();
}

void xiiShaderPermutationComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (m_Permutations.IsEmpty() || !ref_msg.m_pView || !ref_msg.m_pExtractedRenderData) return;
  auto pWM = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (!pWM) return;
  auto* p                   = pWM->CreateRenderDataForThisFrame<xiiShaderPermutationRenderData>(this);
  p->m_GlobalTransform      = GetOwner()->GetGlobalTransform();
  p->m_GlobalBounds         = GetOwner()->GetGlobalBounds();
  p->m_hOwnerObject         = GetOwner()->GetHandle();
  p->m_hOwnerComponent      = GetHandle();
  p->m_PermutationOverrides = m_Permutations;
  p->m_uiSortingKey         = GetUniqueIdForRendering();
  ref_msg.AddRenderData(p, xiiRenderData::Caching::IfStatic);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Materials_Implementation_ShaderAndMaterialComponents);
