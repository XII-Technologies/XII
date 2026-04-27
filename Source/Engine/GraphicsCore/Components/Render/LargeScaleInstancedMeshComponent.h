#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/IndirectDrawBatchBuilder.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <Foundation/Math/BoundingBoxSphere.h>

struct xiiMsgExtractRenderData;
class  xiiWorld;

// ============================================================
//  Render Data
// ============================================================

/// \brief Render data for one large-scale instanced draw (e.g. 100k trees, tiles, robots).
class XII_GRAPHICSCORE_DLL xiiLargeScaleInstancedMeshRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLargeScaleInstancedMeshRenderData, xiiRenderData);

public:
  xiiMeshResourceHandle     m_hMesh;
  xiiMeshletResourceHandle  m_hMeshlets;    ///< Optional meshlet path.
  xiiMaterialResourceHandle m_hMaterial;

  xiiGALBufferHandle m_hInstanceBuffer;    ///< GPU structured buffer of xiiGPUInstanceData.
  xiiUInt32          m_uiInstanceCount = 0;
  bool               m_bUseMeshlets    = false;
};

// ============================================================
//  Component
// ============================================================

using xiiLargeScaleInstancedMeshComponentManager =
  xiiComponentManager<class xiiLargeScaleInstancedMeshComponent, xiiBlockStorageType::Compact>;

/// \brief Renders up to several million identical mesh instances per component.
///
/// Unlike xiiInstancedMeshComponent (which is CPU-driven), this component owns a
/// persistent GPU instance buffer that can be populated by:
///   - A CPU `BeginWriteInstances()` / `EndWriteInstances()` call once or rarely.
///   - A compute shader that writes directly (set via SetComputeFilledBuffer).
///   - A simulation system that exports positions into the buffer each frame.
///
/// The GPU culling pass (`xiiGPUDrivenCullingPass`) culls this buffer per-frame.
/// This is the primary component for robotics scenarios with dense agent populations.
class XII_GRAPHICSCORE_DLL xiiLargeScaleInstancedMeshComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiLargeScaleInstancedMeshComponent, xiiRenderComponent, xiiLargeScaleInstancedMeshComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiLargeScaleInstancedMeshComponent();
  ~xiiLargeScaleInstancedMeshComponent();

  // ---- Mesh ----
  void          SetMeshFile(xiiStringView sFile);  // [ property ]
  xiiStringView GetMeshFile() const;               // [ property ]

  // ---- Meshlet ----
  void          SetMeshletFile(xiiStringView sFile); // [ property ]
  xiiStringView GetMeshletFile() const;              // [ property ]

  // ---- Material ----
  void          SetMaterialFile(xiiStringView sFile); // [ property ]
  xiiStringView GetMaterialFile() const;              // [ property ]

  // ---- Instance capacity ----
  void      SetMaxInstances(xiiUInt32 uiMax); // [ property ]
  xiiUInt32 GetMaxInstances() const { return m_uiMaxInstances; }

  // ---- CPU-side instance write ----
  /// \brief Locks the instance buffer for CPU write. Returns a pointer to the first record.
  xiiGPUInstanceData* BeginWriteInstances(xiiUInt32 uiCount);
  /// \brief Commits the CPU-written instances and sets the live count.
  void EndWriteInstances();

  // ---- GPU-compute populated buffer ----
  /// \brief Replaces the internal instance buffer with an externally managed one (compute-filled).
  void SetExternalInstanceBuffer(xiiGALBufferHandle hBuffer, xiiUInt32 uiCount);

  xiiUInt32 GetCurrentInstanceCount() const { return m_uiCurrentInstanceCount; }

  // ---- Bounds ----
  void SetCustomBounds(const xiiBoundingBoxSphere& bounds); // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

private:
  void EnsureBufferCapacity(xiiUInt32 uiRequired);

  xiiMeshResourceHandle     m_hMesh;
  xiiMeshletResourceHandle  m_hMeshlets;
  xiiMaterialResourceHandle m_hMaterial;

  xiiGALBufferHandle m_hInstanceBuffer;
  xiiGALBufferHandle m_hExternalBuffer;   ///< Set by SetExternalInstanceBuffer; takes priority.

  xiiBoundingBoxSphere m_CustomBounds = xiiBoundingBoxSphere::MakeZero();
  bool                 m_bHasCustomBounds = false;

  xiiUInt32 m_uiMaxInstances           = 0;
  xiiUInt32 m_uiCurrentInstanceCount   = 0;
  bool      m_bExternalBuffer          = false;
};
