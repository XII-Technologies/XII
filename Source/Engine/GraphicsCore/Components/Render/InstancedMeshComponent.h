#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// \brief Per-instance transform and data packed into the GPU instancing buffer.
struct XII_GRAPHICSCORE_DLL xiiMeshInstanceData
{
  xiiTransform m_Transform = xiiTransform::MakeIdentity();
  xiiColor     m_Color     = xiiColor::White;
};

/// \brief Render data submitted per-frame by an instanced mesh component.
class XII_GRAPHICSCORE_DLL xiiInstancedMeshRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInstancedMeshRenderData, xiiRenderData);

public:
  xiiMeshResourceHandle     m_hMesh;
  xiiMaterialResourceHandle m_hMaterial;

  /// \brief Points into a per-frame instance data arena; valid only during rendering.
  xiiArrayPtr<const xiiMeshInstanceData> m_Instances;

  xiiUInt32 m_uiInstanceCount = 0;
  bool      m_bCastShadows    = true;
};

using xiiInstancedMeshComponentManager = xiiComponentManager<class xiiInstancedMeshComponent, xiiBlockStorageType::Compact>;

/// \brief Renders many copies of the same mesh in a single GPU instanced draw call.
///
/// Instance transforms are stored CPU-side and uploaded each frame as part of render data extraction.
/// For very large instance counts (> ~100k) prefer the GPUDrivenComponent pipeline.
class XII_GRAPHICSCORE_DLL xiiInstancedMeshComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiInstancedMeshComponent, xiiRenderComponent, xiiInstancedMeshComponentManager);

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
  // xiiInstancedMeshComponent

public:
  xiiInstancedMeshComponent();
  ~xiiInstancedMeshComponent();

  void          SetMeshFile(xiiStringView sFile); // [ property ]
  xiiStringView GetMeshFile() const;              // [ property ]

  void                         SetMesh(const xiiMeshResourceHandle& hMesh);
  const xiiMeshResourceHandle& GetMesh() const { return m_hMesh; }

  void          SetMaterialFile(xiiStringView sFile); // [ property ]
  xiiStringView GetMaterialFile() const;              // [ property ]

  void                             SetMaterial(const xiiMaterialResourceHandle& hMaterial);
  const xiiMaterialResourceHandle& GetMaterial() const { return m_hMaterial; }

  void SetCastShadows(bool bCast);                       // [ property ]
  bool GetCastShadows() const { return m_bCastShadows; } // [ property ]

  xiiDynamicArray<xiiMeshInstanceData>&       GetInstances() { return m_Instances; }
  const xiiDynamicArray<xiiMeshInstanceData>& GetInstances() const { return m_Instances; }

  /// \brief Replaces the whole instance list and rebuilds the local bounds.
  void SetInstances(xiiArrayPtr<const xiiMeshInstanceData> instances);

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;
  void RecomputeBounds();

  xiiMeshResourceHandle                m_hMesh;
  xiiMaterialResourceHandle            m_hMaterial;
  xiiDynamicArray<xiiMeshInstanceData> m_Instances;
  xiiBoundingBoxSphere                 m_LocalBounds  = xiiBoundingBoxSphere::MakeInvalid();
  bool                                 m_bCastShadows = true;
};
