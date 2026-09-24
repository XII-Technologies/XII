/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Material/MaterialGpuStorage.h>

/// Frame-local draw/dispatch payload produced by the material system. Renderers use the GPU offset
/// for parameter access and the resource bindings for descriptor or bindless table resolution.
struct XII_GRAPHICSCORE_DLL xiiMaterialRenderData
{
  xiiMaterialGpuHandle                        m_Handle;
  xiiUInt32                                   m_uiGpuOffset = xiiInvalidIndex;
  xiiMaterialRuntimeState                     m_RuntimeState;
  xiiDynamicArray<xiiMaterialResourceBinding> m_ResourceBindings;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialRenderData);

/// High-level owner for material instances and their GPU-visible storage.
///
/// Typical frame flow:
/// 1. BeginFrame() retires slots whose last GPU frame completed.
/// 2. Simulation threads mutate xiiMaterialInstance objects by stable parameter ID.
/// 3. AddUploadPass() captures revisions and declares transfer-to-shader synchronization.
/// 4. ExtractRenderData() produces compact draw/dispatch records for render extraction.
class XII_GRAPHICSCORE_DLL xiiMaterialSystem
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiMaterialSystem);

public:
  xiiMaterialSystem() = default;

  xiiResult Initialize(xiiGALDevice* pDevice, const xiiMaterialGpuStorageDescription& description = {});
  void      Shutdown();

  void BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame);

  [[nodiscard]] xiiMaterialGpuHandle RegisterMaterial(xiiSharedPtr<xiiMaterialInstance> pInstance);
  void                               UnregisterMaterial(xiiMaterialGpuHandle handle);

  [[nodiscard]] xiiRenderGraphBufferHandle AddUploadPass(xiiRenderGraph& graph);
  [[nodiscard]] xiiResult                  ExtractRenderData(xiiMaterialGpuHandle handle, xiiMaterialRenderData& out_renderData) const;

  [[nodiscard]] xiiMaterialGpuStorage&       GetGpuStorage() { return m_GpuStorage; }
  [[nodiscard]] const xiiMaterialGpuStorage& GetGpuStorage() const { return m_GpuStorage; }

  /// Builds a standalone runtime material without going through the resource manager. This is the
  /// preferred path for generated simulation, robotics sensor, and medical visualization materials.
  static xiiResult CreateRuntimeMaterial(const xiiMaterialSchemaDescription& description, const xiiMaterialRuntimeState& runtimeState, xiiSharedPtr<xiiMaterialSchema>& out_pSchema, xiiSharedPtr<xiiMaterialInstance>& out_pInstance, xiiStringBuilder* out_pError = nullptr);

private:
  xiiMaterialGpuStorage m_GpuStorage;
  xiiUInt64             m_uiFrameIndex = 0ULL;
  bool                  m_bInitialized = false;
};
