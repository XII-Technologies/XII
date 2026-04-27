#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Tiled deferred PBR lighting pass — reads all four GBuffer RTs and produces an HDR lit scene.
///
/// ## Algorithm
/// 1. Classify screen tiles (16×16 px) by light count using a compute pass that tests each light
///    against the tile frustum. Result: per-tile light index lists in a structured buffer.
/// 2. Full-screen evaluation pixel shader:
///    - Reconstructs world position from depth.
///    - Reads albedo, normal, ORM, velocity from the GBuffer SRVs.
///    - Iterates per-tile light list, evaluates Cook-Torrance PBR for each:
///      * Punctual: point, spot, directional.
///      * Area: rect lights via Linearly Transformed Cosines (LTC).
///      * IES profiles: modulate angular distribution by a 2D LUT.
///    - Adds IBL (irradiance + specular) from the dominant reflection probe.
///    - Applies baked/RT ambient occlusion.
/// 3. Composites emissive contribution from the RT2 custom channel.
///
/// ## Outputs
/// - **LitHDR** (`R16G16B16A16_FLOAT`): fully lit, pre-tonemap HDR colour.
///
/// ## Inputs (GBuffer pins)
/// - AlbedoAO, NormalSM, ORM, Velocity, Depth
class XII_GRAPHICSCORE_DLL xiiDeferredLightingPass : public xiiRenderGraphPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDeferredLightingPass, xiiRenderGraphPass);

public:
  xiiDeferredLightingPass();
  ~xiiDeferredLightingPass();

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
  void SetTileSize(xiiUInt32 uiSize)         { m_uiTileSize          = uiSize; }
  void SetMaxLightsPerTile(xiiUInt32 uiMax)  { m_uiMaxLightsPerTile  = uiMax; }
  void SetIBLEnabled(bool b)                 { m_bIBLEnabled         = b; }
  void SetAreaLightsEnabled(bool b)          { m_bAreaLightsEnabled  = b; }
  void SetShadowsEnabled(bool b)             { m_bShadowsEnabled     = b; }

  xiiUInt32 GetTileSize()          const { return m_uiTileSize; }
  xiiUInt32 GetMaxLightsPerTile()  const { return m_uiMaxLightsPerTile; }
  bool      GetIBLEnabled()        const { return m_bIBLEnabled; }
  bool      GetAreaLightsEnabled() const { return m_bAreaLightsEnabled; }
  bool      GetShadowsEnabled()    const { return m_bShadowsEnabled; }

  // ---- Pin names ----
  static constexpr const char* k_sInputAlbedoAO  = "AlbedoAO";
  static constexpr const char* k_sInputNormalSM   = "NormalSM";
  static constexpr const char* k_sInputORM        = "ORM";
  static constexpr const char* k_sInputVelocity   = "Velocity";
  static constexpr const char* k_sInputDepth      = "Depth";
  static constexpr const char* k_sOutputLitHDR    = "LitHDR";

private:
  void ClassifyTiles(const xiiRenderViewContext& ctx, xiiGALCommandList& cmd);
  void UploadLightData(const xiiRenderViewContext& ctx, xiiGALCommandList& cmd);
  void EvaluateLighting(const xiiRenderViewContext& ctx, xiiGALCommandList& cmd,
                        const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs);

  // Pipeline state objects
  xiiGALPipelineStateHandle m_hTileClassifyPSO;   ///< Compute: frustum-test lights per tile
  xiiGALPipelineStateHandle m_hLightingPSO;       ///< Full-screen PBR evaluation
  xiiGALPipelineStateHandle m_hLTCMatrixLUT;      ///< Area-light LTC precomputed matrix LUT (texture)

  // Per-frame GPU data
  xiiGALBufferHandle  m_hLightDataBuffer;          ///< Packed point/spot/directional light params
  xiiGALBufferHandle  m_hTileLightListBuffer;      ///< Per-tile: [count, lightIdx0, lightIdx1, …]
  xiiGALTextureHandle m_hLTCTexture;               ///< 64×64 LTC matrix + magnitude LUT

  // Settings
  xiiUInt32 m_uiTileSize         = 16;
  xiiUInt32 m_uiMaxLightsPerTile = 256;
  bool      m_bIBLEnabled        = true;
  bool      m_bAreaLightsEnabled = true;
  bool      m_bShadowsEnabled    = true;
  bool      m_bInitialised       = false;
};

// ============================================================
//  Translucency Pass
// ============================================================

/// \brief Forward+ translucent geometry pass.
///
/// Uses the same per-tile light list built by xiiDeferredLightingPass.
/// Translucent geometry is drawn in a single back-to-front sorted pass using
/// the per-tile light list for efficient lighting without OIT overhead.
/// For scientific visualisation (molecular surfaces) an optional per-pixel
/// linked-list OIT fallback is available via SetOITEnabled(true).
class XII_GRAPHICSCORE_DLL xiiTranslucencyPass : public xiiRenderGraphPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTranslucencyPass, xiiRenderGraphPass);

public:
  xiiTranslucencyPass();
  ~xiiTranslucencyPass();

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

  void SetOITEnabled(bool b) { m_bOITEnabled = b; }
  bool GetOITEnabled() const { return m_bOITEnabled; }

  static constexpr const char* k_sInputLitHDR   = "LitHDR";
  static constexpr const char* k_sInputDepth     = "Depth";
  static constexpr const char* k_sOutputLitHDR   = "LitHDR";

private:
  xiiGALPipelineStateHandle m_hTranslucentPSO;
  xiiGALPipelineStateHandle m_hOITResolvePSO;

  // OIT per-pixel linked list
  xiiGALBufferHandle  m_hOITFragmentBuffer;
  xiiGALBufferHandle  m_hOITCounterBuffer;
  xiiGALTextureHandle m_hOITHeadPointerTexture;

  bool m_bOITEnabled   = false;
  bool m_bInitialised  = false;
};
