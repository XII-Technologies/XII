#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>

struct xiiMsgExtractRenderData;

// ============================================================
//  Terrain Component
// ============================================================

using xiiTerrainComponentManager = xiiComponentManager<class xiiTerrainComponent, xiiBlockStorageType::Compact>;

/// \brief Renders a clipmap-based terrain suitable for massive open worlds or planetary scales.
///
/// Integrates with Virtual Texturing for material blending across many layers and Virtual Heightfields
/// for sub-meter geometric detail.
class XII_GRAPHICSCORE_DLL xiiTerrainComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiTerrainComponent, xiiRenderComponent, xiiTerrainComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiTerrainComponent();
  ~xiiTerrainComponent();

  // ---- Geometry / Heightmap ----
  void SetHeightmapFile(xiiStringView sFile); // [ property ]
  xiiStringView GetHeightmapFile() const;
  
  void SetSize(const xiiVec2& vSize); // [ property ]
  const xiiVec2& GetSize() const { return m_vSize; }
  
  void SetHeightRange(const xiiVec2& vRange); // [ property ]
  const xiiVec2& GetHeightRange() const { return m_vHeightRange; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

private:
  xiiString m_sHeightmapFile;
  xiiVec2 m_vSize = xiiVec2(1024.0f, 1024.0f);
  xiiVec2 m_vHeightRange = xiiVec2(0.0f, 256.0f);
};

// ============================================================
//  Terrain Material Layer Component
// ============================================================

using xiiTerrainMaterialLayerComponentManager = xiiComponentManager<class xiiTerrainMaterialLayerComponent, xiiBlockStorageType::Compact>;

/// \brief Defines a biome or material layer applied to the terrain, utilizing virtual texturing.
class XII_GRAPHICSCORE_DLL xiiTerrainMaterialLayerComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiTerrainMaterialLayerComponent, xiiComponent, xiiTerrainMaterialLayerComponentManager);

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  xiiTerrainMaterialLayerComponent();
  ~xiiTerrainMaterialLayerComponent();

  void SetMaterialFile(xiiStringView sFile); // [ property ]
  xiiStringView GetMaterialFile() const;

  void SetLayerMaskFile(xiiStringView sFile); // [ property ]
  xiiStringView GetLayerMaskFile() const;

private:
  xiiString m_sMaterialFile;
  xiiString m_sLayerMaskFile;
};

// ============================================================
//  Virtual Heightfield Component
// ============================================================

using xiiVirtualHeightfieldComponentManager = xiiComponentManager<class xiiVirtualHeightfieldComponent, xiiBlockStorageType::Compact>;

/// \brief Represents a sparse, virtualized heightfield layer that adds fine geometric detail to the terrain or standalone planes.
class XII_GRAPHICSCORE_DLL xiiVirtualHeightfieldComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiVirtualHeightfieldComponent, xiiRenderComponent, xiiVirtualHeightfieldComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiVirtualHeightfieldComponent();
  ~xiiVirtualHeightfieldComponent();

  void SetHeightDataFile(xiiStringView sFile); // [ property ]
  xiiStringView GetHeightDataFile() const;

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

private:
  xiiString m_sHeightDataFile;
};
