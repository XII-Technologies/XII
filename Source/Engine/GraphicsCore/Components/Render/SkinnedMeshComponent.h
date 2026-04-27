#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// \brief Render data submitted per-frame by a skinned mesh component.
class XII_GRAPHICSCORE_DLL xiiSkinnedMeshRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkinnedMeshRenderData, xiiRenderData);

public:
  xiiMeshResourceHandle                      m_hMesh;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;

  /// \brief Byte offset into the per-frame GPU bone palette buffer for this draw call.
  xiiUInt32 m_uiBonePaletteOffset = 0;

  /// \brief Number of joints / bone transforms uploaded for this mesh instance.
  xiiUInt16 m_uiBoneCount = 0;

  bool m_bCastShadows = true;
};

using xiiSkinnedMeshComponentManager = xiiComponentManager<class xiiSkinnedMeshComponent, xiiBlockStorageType::Compact>;

/// \brief Renders a GPU-skinned skeletal mesh driven by an external animation controller.
///
/// The component does not own the skeleton or animation graph; it only receives the final
/// bone palette (world-space joint matrices) each frame and packages it into render data for
/// the skinned mesh renderer to consume. Material slots mirror the mesh sub-mesh layout.
class XII_GRAPHICSCORE_DLL xiiSkinnedMeshComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkinnedMeshComponent, xiiRenderComponent, xiiSkinnedMeshComponentManager);

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
  // xiiSkinnedMeshComponent

public:
  xiiSkinnedMeshComponent();
  ~xiiSkinnedMeshComponent();

  void          SetMeshFile(xiiStringView sFile); // [ property ]
  xiiStringView GetMeshFile() const;              // [ property ]

  void                         SetMesh(const xiiMeshResourceHandle& hMesh);
  const xiiMeshResourceHandle& GetMesh() const { return m_hMesh; }

  xiiUInt32                 GetMaterialCount() const;
  void                      SetMaterial(xiiUInt32 uiIndex, const xiiMaterialResourceHandle& hMaterial);
  xiiMaterialResourceHandle GetMaterial(xiiUInt32 uiIndex) const;

  void          SetMaterialFile(xiiUInt32 uiIndex, xiiStringView sFile); // [ property ]
  xiiStringView GetMaterialFile(xiiUInt32 uiIndex) const;                // [ property ]

  void SetCastShadows(bool bCast);                       // [ property ]
  bool GetCastShadows() const { return m_bCastShadows; } // [ property ]

  /// \brief Called by the animation system each frame to supply the updated joint matrices.
  void SetBonePalette(xiiArrayPtr<const xiiMat4> palette);

  const xiiDynamicArray<xiiMat4>& GetBonePalette() const { return m_BonePalette; }

private:
  void          SetMaterialFile0Prop(xiiStringView s);
  xiiStringView GetMaterialFile0Prop() const;

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiMeshResourceHandle                      m_hMesh;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;
  xiiDynamicArray<xiiMat4>                   m_BonePalette;
  bool                                       m_bCastShadows = true;
};
