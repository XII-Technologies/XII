#pragma once

#include <Core/World/World.h>
#include <GraphicsCore/Meshes/MeshComponentBase.h>

using xiiLodMeshComponentManager = xiiComponentManager<class xiiLodMeshComponent, xiiBlockStorageType::Compact>;

struct xiiLodMeshLod
{
  xiiMeshResourceHandle m_hMesh;      // [ property ]
  float                 m_fThreshold; // [ property ]
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLodMeshLod);

/// \brief Renders one of several level-of-detail meshes depending on the distance to the camera.
///
/// This component is very similar to the xiiLodComponent, please read it's description for details.
/// The difference is, that this component doesn't switch child object on and off, but rather only selects between different render-meshes.
/// As such there is less performance impact for switching between meshes and also the memory overhead for storing LOD information is smaller.
/// If it is only desired to switch between meshes, it is also more convenient to work with just a single component.
///
/// The component does not allow to place the LOD meshes differently, they all need to have the same origin.
/// Compared with the regular xiiMeshComponent there is also no way to override the used materials, since each LOD mesh may use different materials.
class XII_GRAPHICSCORE_DLL xiiLodMeshComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiLodMeshComponent, xiiRenderComponent, xiiLodMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLodMeshComponent

public:
  xiiLodMeshComponent();
  ~xiiLodMeshComponent();

  /// \brief An additional tint color passed to the renderer to modify the mesh.
  void            SetColor(const xiiColor& color); // [ property ]
  const xiiColor& GetColor() const;                // [ property ]

  /// \brief The sorting depth offset allows to tweak the order in which this mesh is rendered relative to other meshes.
  ///
  /// This is mainly useful for transparent objects to render them before or after other meshes.
  void  SetSortingDepthOffset(float fOffset); // [ property ]
  float GetSortingDepthOffset() const;        // [ property ]

  /// \brief Enables text output to show the current coverage value and selected LOD.
  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  /// \brief Disabling the LOD range overlap functionality can make it easier to determine the desired coverage thresholds.
  void SetOverlapRanges(bool bOverlap); // [ property ]
  bool GetOverlapRanges() const;        // [ property ]

  void OnMsgSetColor(xiiMsgSetColor& ref_msg); // [ msg handler ]

protected:
  virtual xiiMeshRenderData* CreateRenderData() const;

  void UpdateSelectedLod(const xiiView& view) const;
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  mutable xiiInt32               m_iCurLod = 0;
  xiiDynamicArray<xiiLodMeshLod> m_Meshes;
  xiiColor                       m_Color               = xiiColor::White;
  float                          m_fSortingDepthOffset = 0.0f;
  xiiVec3                        m_vBoundsOffset       = xiiVec3::MakeZero();
  float                          m_fBoundsRadius       = 1.0f;
};
