#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

// ============================================================
//  GBuffer Layout (slots must match GBufferResolve.hlsl)
// ============================================================

/// \brief Canonical GBuffer render target slot assignments.
///
/// All passes that read from the GBuffer must use these indices.
/// Packing is chosen to minimise bandwidth while supporting full PBR + velocity.
///
/// | Slot | Format               | R          | G          | B          | A              |
/// |------|----------------------|------------|------------|------------|----------------|
/// | RT0  | R8G8B8A8_UNORM       | Albedo.R   | Albedo.G   | Albedo.B   | Baked AO       |
/// | RT1  | R10G10B10A2_UNORM    | Normal.X   | Normal.Y   | Normal.Z   | Shading model  |
/// | RT2  | R8G8B8A8_UNORM       | Roughness  | Metallic   | Specular   | Custom data    |
/// | RT3  | R16G16_FLOAT         | Velocity.X | Velocity.Y | —          | —              |
/// | DS   | D32_FLOAT            | Depth      | —          | —          | —              |
struct xiiGBufferLayout
{
  static constexpr xiiUInt8 Albedo_AO   = 0;
  static constexpr xiiUInt8 Normal_ShadingModel = 1;
  static constexpr xiiUInt8 ORM_Custom  = 2;  // Occlusion/Roughness/Metallic
  static constexpr xiiUInt8 Velocity    = 3;
  static constexpr xiiUInt8 RenderTargetCount = 4;
};

// ============================================================
//  Blackboard keys written by the GBuffer pass
// ============================================================
// (declared in PipelineBlackboardKeys.h — mirrored here for documentation)
//   "GBuffer_AlbedoAO"       — xiiGALTextureViewHandle (SRV)
//   "GBuffer_NormalSM"       — xiiGALTextureViewHandle (SRV)
//   "GBuffer_ORM"            — xiiGALTextureViewHandle (SRV)
//   "GBuffer_Velocity"       — xiiGALTextureViewHandle (SRV)
//   "GBuffer_Depth"          — xiiGALTextureViewHandle (DSV + SRV)

// ============================================================
//  Pass
// ============================================================

/// \brief Renders all opaque geometry into the four GBuffer render targets.
///
/// Execution order in the render graph:
///   FrameKickoff → GPUDrivenCull (Phase1) → GBufferPass → HZBBuild → GPUDrivenCull (Phase2) → DeferredLighting
///
/// The pass dispatches:
///   1. DispatchMesh for meshlet batches (task + mesh shader path).
///   2. DrawIndexedInstanced for non-meshlet indirect batches.
///
/// All draws share a single large bindless material table (updated once per frame).
class XII_GRAPHICSCORE_DLL xiiGBufferPass : public xiiRenderGraphPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGBufferPass, xiiRenderGraphPass);

public:
  xiiGBufferPass();
  ~xiiGBufferPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view,
                                           const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs,
                                           xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;

  virtual void InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
                                      const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext,
                       const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
                       const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  virtual void ExecuteInactive(const xiiRenderViewContext& renderViewContext,
                               const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
                               const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  // ---- Settings ----
  void SetWireframeOverlay(bool b)  { m_bWireframe     = b; }
  void SetMeshShaderEnabled(bool b) { m_bUseMeshShader = b; }

  bool GetWireframeOverlay()  const { return m_bWireframe; }
  bool GetMeshShaderEnabled() const { return m_bUseMeshShader; }

  // ---- Render graph pin names ----
  static constexpr const char* k_sOutputAlbedoAO  = "AlbedoAO";
  static constexpr const char* k_sOutputNormalSM   = "NormalSM";
  static constexpr const char* k_sOutputORM        = "ORM";
  static constexpr const char* k_sOutputVelocity   = "Velocity";
  static constexpr const char* k_sOutputDepth      = "Depth";

private:
  void DrawMeshletBatches(const xiiRenderViewContext& ctx, xiiGALCommandList& cmd);
  void DrawIndirectBatches(const xiiRenderViewContext& ctx, xiiGALCommandList& cmd);
  void UploadPerFrameData(const xiiRenderViewContext& ctx, xiiGALCommandList& cmd);

  // Pipeline state objects
  xiiGALPipelineStateHandle m_hGBufferMeshletPSO;    ///< Task + mesh shader GBuffer fill
  xiiGALPipelineStateHandle m_hGBufferStaticPSO;     ///< Vertex + pixel shader GBuffer fill
  xiiGALPipelineStateHandle m_hGBufferSkinnedPSO;    ///< Vertex (bone palette) + pixel shader
  xiiGALPipelineStateHandle m_hGBufferWireframePSO;  ///< Wireframe overlay (debug)

  // Per-frame material parameter table
  xiiGALBufferHandle m_hMaterialTableBuffer;

  bool m_bWireframe     = false;
  bool m_bUseMeshShader = true;
  bool m_bInitialised   = false;
};
