#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// \brief Render data submitted per-frame by a static mesh component.
class XII_GRAPHICSCORE_DLL xiiStaticMeshRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStaticMeshRenderData, xiiRenderData);

public:
  xiiMeshResourceHandle                      m_hMesh;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;
  xiiUInt8                                   m_uiActiveLOD    = 0;
  bool                                       m_bCastShadows   = true;
  bool                                       m_bCastDynShadow = true;
};

using xiiStaticMeshComponentManager = xiiComponentManager<class xiiStaticMeshComponent, xiiBlockStorageType::Compact>;

/// \brief Renders a non-animated triangle mesh asset at a fixed world transform.
///
/// The mesh is sourced from a xiiMeshResource. Multiple material slots can be overridden
/// at the component level. Shadow casting can be toggled independently for static and
/// dynamic shadows. An LOD bias nudges the engine LOD selection up or down.
class XII_GRAPHICSCORE_DLL xiiStaticMeshComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiStaticMeshComponent, xiiRenderComponent, xiiStaticMeshComponentManager);

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
  // xiiStaticMeshComponent

public:
  xiiStaticMeshComponent();
  ~xiiStaticMeshComponent();

  void          SetMeshFile(xiiStringView sFile); // [ property ]
  xiiStringView GetMeshFile() const;              // [ property ]

  void                         SetMesh(const xiiMeshResourceHandle& hMesh);
  const xiiMeshResourceHandle& GetMesh() const { return m_hMesh; }

  /// \brief Number of material override slots (one per mesh sub-mesh).
  xiiUInt32 GetMaterialCount() const;

  void                      SetMaterial(xiiUInt32 uiIndex, const xiiMaterialResourceHandle& hMaterial);
  xiiMaterialResourceHandle GetMaterial(xiiUInt32 uiIndex) const;

  void          SetMaterialFile(xiiUInt32 uiIndex, xiiStringView sFile); // [ property ]
  xiiStringView GetMaterialFile(xiiUInt32 uiIndex) const;                // [ property ]

  void    SetLODBias(xiiInt8 iBias);                // [ property ]
  xiiInt8 GetLODBias() const { return m_iLODBias; } // [ property ]

  void SetCastShadows(bool bCast);                       // [ property ]
  bool GetCastShadows() const { return m_bCastShadows; } // [ property ]

  void SetCastDynamicShadows(bool bCast);                          // [ property ]
  bool GetCastDynamicShadows() const { return m_bCastDynShadows; } // [ property ]

private:
  void          SetMaterialFile0Prop(xiiStringView s);
  xiiStringView GetMaterialFile0Prop() const;

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiMeshResourceHandle                      m_hMesh;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;
  xiiInt8                                    m_iLODBias        = 0;
  bool                                       m_bCastShadows    = true;
  bool                                       m_bCastDynShadows = true;
};
