#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <Foundation/Math/BoundingBoxSphere.h>

struct xiiMsgExtractRenderData;

// ============================================================
//  Flags
// ============================================================

/// \brief Controls which optional meshlet rendering features are active.
struct XII_GRAPHICSCORE_DLL xiiMeshletRenderFlags
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    None                 = 0,
    CastShadows          = XII_BIT(0),
    CastDynamicShadows   = XII_BIT(1),
    ReceiveDecals        = XII_BIT(2),
    UseBackfaceCull      = XII_BIT(3), ///< Enable per-meshlet backface-cone culling in the task shader.
    UseOcclusionCulling  = XII_BIT(4), ///< Enable HZB-based per-meshlet occlusion culling.

    Default = CastShadows | CastDynamicShadows | ReceiveDecals | UseBackfaceCull | UseOcclusionCulling,
  };
};
XII_DECLARE_FLAGS_OPERATORS(xiiMeshletRenderFlags);

// ============================================================
//  Render Data
// ============================================================

/// \brief Render data submitted by xiiMeshletComponent each frame.
///
/// A single record covers all LODs within the meshlet resource; the GPU
/// task shader selects the appropriate LOD per-meshlet based on screen coverage.
class XII_GRAPHICSCORE_DLL xiiMeshletRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshletRenderData, xiiRenderData);

public:
  xiiMeshletResourceHandle                   m_hMeshlets;       ///< GPU meshlet resource.
  xiiMeshResourceHandle                      m_hFallbackMesh;   ///< Triangle-list fallback when mesh shaders unavailable.
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;       ///< Per-submesh material overrides.

  xiiEnum<xiiMeshletRenderFlags> m_RenderFlags = xiiMeshletRenderFlags::Default;
  xiiUInt32                      m_uiMeshletCount    = 0;  ///< Total meshlets across all LODs.
  xiiUInt32                      m_uiFirstLODOffset  = 0;  ///< Ring-buffer byte offset for this draw (GPU-driven path).
  float                          m_fLODBias          = 0.0f;
};

// ============================================================
//  Component
// ============================================================

using xiiMeshletComponentManager = xiiComponentManager<class xiiMeshletComponent, xiiBlockStorageType::Compact>;

/// \brief Renders geometry using the GPU mesh-shader / task-shader pipeline.
///
/// Requires a xiiMeshletResource (produced offline by the meshlet cooking tool).
/// Falls back transparently to xiiStaticMeshComponent-style indirect drawing when
/// the active GAL device does not expose mesh shader support.
///
/// Typical use:
///   - Author mesh in DCC tool
///   - Cook with XII MeshletCooker → generates .xmeshlet asset
///   - Assign xiiMeshletResource handle here
///   - The task shader performs per-meshlet frustum + backface + HZB occlusion cull
///   - The mesh shader emits primitives directly from the packed vertex buffer
class XII_GRAPHICSCORE_DLL xiiMeshletComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMeshletComponent, xiiRenderComponent, xiiMeshletComponentManager);

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
  // xiiMeshletComponent

public:
  xiiMeshletComponent();
  ~xiiMeshletComponent();

  // ---- Meshlet resource ----
  void                          SetMeshletFile(xiiStringView sFile);  // [ property ]
  xiiStringView                 GetMeshletFile() const;               // [ property ]
  void                          SetMeshlet(const xiiMeshletResourceHandle& h);
  const xiiMeshletResourceHandle& GetMeshlet() const { return m_hMeshlets; }

  // ---- Fallback triangle mesh ----
  void          SetFallbackMeshFile(xiiStringView sFile); // [ property ]
  xiiStringView GetFallbackMeshFile() const;              // [ property ]

  // ---- Materials ----
  xiiUInt32                 GetMaterialCount() const;
  void                      SetMaterial(xiiUInt32 uiIndex, const xiiMaterialResourceHandle& hMaterial);
  xiiMaterialResourceHandle GetMaterial(xiiUInt32 uiIndex) const;
  void                      SetMaterialFile(xiiUInt32 uiIndex, xiiStringView sFile); // [ property ]
  xiiStringView             GetMaterialFile(xiiUInt32 uiIndex) const;                // [ property ]

  // ---- Per-meshlet culling flags ----
  void SetCastShadows(bool b);         bool GetCastShadows()         const;
  void SetCastDynamicShadows(bool b);  bool GetCastDynamicShadows()  const;
  void SetBackfaceCull(bool b);        bool GetBackfaceCull()         const;
  void SetOcclusionCulling(bool b);    bool GetOcclusionCulling()     const;
  void SetReceiveDecals(bool b);       bool GetReceiveDecals()        const;

  // ---- LOD bias ----
  void  SetLODBias(float f);         // [ property ]
  float GetLODBias() const { return m_fLODBias; }

private:
  void          SetMeshletFile0Prop(xiiStringView s);
  xiiStringView GetMeshletFile0Prop() const;

  void          SetMat0Prop(xiiStringView s);
  xiiStringView GetMat0Prop() const;

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiMeshletResourceHandle                   m_hMeshlets;
  xiiMeshResourceHandle                      m_hFallbackMesh;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;
  xiiEnum<xiiMeshletRenderFlags>             m_RenderFlags = xiiMeshletRenderFlags::Default;
  float                                      m_fLODBias    = 0.0f;
};
