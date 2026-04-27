#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Components/Terrain/TerrainComponents.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// ============================================================
// xiiTerrainComponent
// ============================================================

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiTerrainComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Heightmap", GetHeightmapFile, SetHeightmapFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_Heightmap")),
    XII_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(1024.0f))),
    XII_ACCESSOR_PROPERTY("HeightRange", GetHeightRange, SetHeightRange)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2(0.0f, 256.0f))),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Terrain"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiTerrainComponent::xiiTerrainComponent() = default;
xiiTerrainComponent::~xiiTerrainComponent() = default;

void xiiTerrainComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_sHeightmapFile << m_vSize << m_vHeightRange;
}

void xiiTerrainComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_sHeightmapFile >> m_vSize >> m_vHeightRange;
}

xiiResult xiiTerrainComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);
  ref_bAlwaysVisible = false;
  // Terrain bounds derived from size and height range
  xiiVec3 extents(m_vSize.x * 0.5f, m_vSize.y * 0.5f, (m_vHeightRange.y - m_vHeightRange.x) * 0.5f);
  xiiVec3 center(0.0f, 0.0f, m_vHeightRange.x + extents.z);
  ref_bounds = xiiBoundingBoxSphere::MakeFromBox(xiiBoundingBox::MakeFromCenterAndHalfExtents(center, extents));
  return XII_SUCCESS;
}

void xiiTerrainComponent::SetHeightmapFile(xiiStringView sFile)
{
  m_sHeightmapFile = sFile;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiStringView xiiTerrainComponent::GetHeightmapFile() const { return m_sHeightmapFile; }

void xiiTerrainComponent::SetSize(const xiiVec2& vSize)
{
  m_vSize = vSize;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiTerrainComponent::SetHeightRange(const xiiVec2& vRange)
{
  m_vHeightRange = vRange;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

void xiiTerrainComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  XII_IGNORE_UNUSED(ref_msg);
  // Extract terrain render data (clipmap nodes)
}

// ============================================================
// xiiTerrainMaterialLayerComponent
// ============================================================

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiTerrainMaterialLayerComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material")),
    XII_ACCESSOR_PROPERTY("LayerMask", GetLayerMaskFile, SetLayerMaskFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Terrain"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiTerrainMaterialLayerComponent::xiiTerrainMaterialLayerComponent() = default;
xiiTerrainMaterialLayerComponent::~xiiTerrainMaterialLayerComponent() = default;

void xiiTerrainMaterialLayerComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_sMaterialFile << m_sLayerMaskFile;
}

void xiiTerrainMaterialLayerComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_sMaterialFile >> m_sLayerMaskFile;
}

void xiiTerrainMaterialLayerComponent::SetMaterialFile(xiiStringView sFile) { m_sMaterialFile = sFile; }
xiiStringView xiiTerrainMaterialLayerComponent::GetMaterialFile() const { return m_sMaterialFile; }
void xiiTerrainMaterialLayerComponent::SetLayerMaskFile(xiiStringView sFile) { m_sLayerMaskFile = sFile; }
xiiStringView xiiTerrainMaterialLayerComponent::GetLayerMaskFile() const { return m_sLayerMaskFile; }

// ============================================================
// xiiVirtualHeightfieldComponent
// ============================================================

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiVirtualHeightfieldComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("HeightData", GetHeightDataFile, SetHeightDataFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_VirtualHeightfield")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Terrain"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiVirtualHeightfieldComponent::xiiVirtualHeightfieldComponent() = default;
xiiVirtualHeightfieldComponent::~xiiVirtualHeightfieldComponent() = default;

void xiiVirtualHeightfieldComponent::SerializeComponent(xiiWorldWriter& s) const
{
  SUPER::SerializeComponent(s);
  s.GetStream() << m_sHeightDataFile;
}

void xiiVirtualHeightfieldComponent::DeserializeComponent(xiiWorldReader& s)
{
  SUPER::DeserializeComponent(s);
  s.GetStream() >> m_sHeightDataFile;
}

xiiResult xiiVirtualHeightfieldComponent::GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg)
{
  XII_IGNORE_UNUSED(ref_msg);
  ref_bAlwaysVisible = false;
  ref_bounds = xiiBoundingBoxSphere::MakeFromSphere(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), 100.0f));
  return XII_SUCCESS;
}

void xiiVirtualHeightfieldComponent::SetHeightDataFile(xiiStringView sFile)
{
  m_sHeightDataFile = sFile;
  TriggerLocalBoundsUpdate();
  InvalidateCachedRenderData();
}

xiiStringView xiiVirtualHeightfieldComponent::GetHeightDataFile() const { return m_sHeightDataFile; }

void xiiVirtualHeightfieldComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  XII_IGNORE_UNUSED(ref_msg);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Terrain_Implementation_TerrainComponents);
