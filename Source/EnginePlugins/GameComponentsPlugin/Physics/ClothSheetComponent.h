/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/Physics/ClothSheetSimulator.h>
#include <GraphicsCore/Components/RenderComponent.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/Renderer.h>

using xiiMaterialResourceHandle          = xiiTypedResourceHandle<class xiiMaterialResource>;
using xiiDynamicMeshBufferResourceHandle = xiiTypedResourceHandle<class xiiDynamicMeshBufferResource>;

//////////////////////////////////////////////////////////////////////////

class XII_GAMECOMPONENTS_DLL xiiClothSheetComponentManager : public xiiComponentManager<class xiiClothSheetComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiClothSheetComponentManager(xiiWorld* pWorld);
  ~xiiClothSheetComponentManager();

  virtual void Initialize() override;

private:
  void Update(const xiiWorldModule::UpdateContext& context);
  void UpdateBounds(const xiiWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class XII_GAMECOMPONENTS_DLL xiiClothSheetRenderData final : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClothSheetRenderData, xiiRenderData);

public:
  xiiUInt32              m_uiUniqueID = 0;
  xiiArrayPtr<xiiVec3>   m_Positions;
  xiiArrayPtr<xiiUInt16> m_Indices;
  xiiUInt16              m_uiVerticesX;
  xiiUInt16              m_uiVerticesY;
  xiiColor               m_Color;

  xiiMaterialResourceHandle m_hMaterial;
};

class XII_GAMECOMPONENTS_DLL xiiClothSheetRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClothSheetRenderer, xiiRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiClothSheetRenderer);

public:
  xiiClothSheetRenderer();
  ~xiiClothSheetRenderer();

  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;

protected:
  void CreateVertexBuffer();

  xiiDynamicMeshBufferResourceHandle m_hDynamicMeshBuffer;
};

/// \brief Flags for how a piece of cloth should be simulated.
struct XII_GAMECOMPONENTS_DLL xiiClothSheetFlags
{
  using StorageType = xiiUInt16;

  enum Enum
  {
    FixedCornerTopLeft     = XII_BIT(0), ///< This corner can't move.
    FixedCornerTopRight    = XII_BIT(1), ///< This corner can't move.
    FixedCornerBottomRight = XII_BIT(2), ///< This corner can't move.
    FixedCornerBottomLeft  = XII_BIT(3), ///< This corner can't move.
    FixedEdgeTop           = XII_BIT(4), ///< This entire edge can't move.
    FixedEdgeRight         = XII_BIT(5), ///< This entire edge can't move.
    FixedEdgeBottom        = XII_BIT(6), ///< This entire edge can't move.
    FixedEdgeLeft          = XII_BIT(7), ///< This entire edge can't move.

    Default = FixedEdgeTop
  };

  struct Bits
  {
    StorageType FixedCornerTopLeft : 1;
    StorageType FixedCornerTopRight : 1;
    StorageType FixedCornerBottomRight : 1;
    StorageType FixedCornerBottomLeft : 1;
    StorageType FixedEdgeTop : 1;
    StorageType FixedEdgeRight : 1;
    StorageType FixedEdgeBottom : 1;
    StorageType FixedEdgeLeft : 1;
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMECOMPONENTS_DLL, xiiClothSheetFlags);

/// \brief Simulates a rectangular piece of cloth.
///
/// The cloth doesn't interact with the environment and doesn't collide with any geometry.
/// The component samples the wind simulation and applies wind forces to the cloth.
///
/// Cloth sheets can be used as decorative elements like flags that blow in the wind.
class XII_GAMECOMPONENTS_DLL xiiClothSheetComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiClothSheetComponent, xiiRenderComponent, xiiClothSheetComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

private:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // xiiClothSheetComponent

public:
  xiiClothSheetComponent();
  ~xiiClothSheetComponent();

  /// Sets the world-space size of the cloth.
  void    SetSize(xiiVec2 vVal);              // [ property ]
  xiiVec2 GetSize() const { return m_vSize; } // [ property ]

  /// Sets of how many pieces the cloth is made up.
  ///
  /// More pieces cost more performance to simulate the cloth.
  /// A size of 32x32 is already quite performance intensive. USe as few segments as possible.
  /// For many cases 8x8 or 12x12 should already be good enough.
  /// Also the more segments there are, the more the cloth will sag.
  void       SetSegments(xiiVec2U32 vVal);               // [ property ]
  xiiVec2U32 GetSegments() const { return m_vSegments; } // [ property ]

  /// How much sag the cloth should have along each axis.
  void    SetSlack(xiiVec2 vVal);               // [ property ]
  xiiVec2 GetSlack() const { return m_vSlack; } // [ property ]

  /// A factor to tweak how strong the wind can push the cloth.
  float m_fWindInfluence = 0.3f; // [ property ]

  /// Damping slows down cloth movement over time. Higher values make it stop sooner and also improve performance.
  float m_fDamping = 0.5f; // [ property ]

  /// Tint color for the cloth material.
  xiiColor m_Color = xiiColor::White; // [ property ]

  /// Sets where the cloth is attached to the world.
  void                            SetFlags(xiiBitflags<xiiClothSheetFlags> flags); // [ property ]
  xiiBitflags<xiiClothSheetFlags> GetFlags() const { return m_Flags; }             // [ property ]

  xiiMaterialResourceHandle m_hMaterial; // [ property ]

private:
  void Update();
  void SetupCloth();

  xiiVec2                         m_vSize;
  xiiVec2                         m_vSlack;
  xiiVec2U32                      m_vSegments;
  xiiBitflags<xiiClothSheetFlags> m_Flags;

  xiiUInt8          m_uiSleepCounter            = 0;
  mutable xiiUInt8  m_uiVisibleCounter          = 0;
  xiiUInt8          m_uiCheckEquilibriumCounter = 0;
  xiiClothSimulator m_Simulator;

  xiiBoundingBox m_Bbox;
};
