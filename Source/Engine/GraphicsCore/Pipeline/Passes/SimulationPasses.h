#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

// ============================================================
//  Segmentation Pass — entity/semantic ID render target
// ============================================================

/// \brief Outputs a R32_UINT render target where each pixel stores the entity ID of the
///        frontmost visible component, enabling pixel-perfect semantic segmentation ground truth.
///
/// Entity IDs are sourced from xiiRenderData::m_hOwnerComponent packed to 32 bits.
/// A class-label LUT maps component handles to semantic class IDs.
class XII_GRAPHICSCORE_DLL xiiSegmentationPass : public xiiRenderGraphPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSegmentationPass, xiiRenderGraphPass);
public:
  xiiSegmentationPass();
  ~xiiSegmentationPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view,
    const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs,
    xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void Execute(const xiiRenderViewContext& ctx,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const xiiRenderViewContext& ctx,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  static constexpr const char* k_sInputDepth      = "Depth";
  static constexpr const char* k_sOutputSegment   = "SegmentationID";

private:
  xiiGALPipelineStateHandle m_hSegmentPSO;
  bool m_bInitialised = false;
};

// ============================================================
//  Depth Ground Truth Pass — metric-scale depth image
// ============================================================

/// \brief Produces a R32_FLOAT render target with linearised camera-space depth in metres.
///
/// Used to simulate depth cameras (RGB-D, structured light, stereo) for robotics ML.
/// Supports configurable noise models: none, Gaussian, shot noise, structured-light dropouts.
struct XII_GRAPHICSCORE_DLL xiiDepthNoiseModel
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType { None=0, Gaussian, ShotNoise, StructuredLightDropout, Default=None };
};

class XII_GRAPHICSCORE_DLL xiiDepthGroundTruthPass : public xiiRenderGraphPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDepthGroundTruthPass, xiiRenderGraphPass);
public:
  xiiDepthGroundTruthPass();
  ~xiiDepthGroundTruthPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view,
    const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs,
    xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void Execute(const xiiRenderViewContext& ctx,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const xiiRenderViewContext& ctx,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  void SetNoiseModel(xiiDepthNoiseModel::Enum e) { m_NoiseModel = e; }
  void SetGaussianSigma(float f)                 { m_fGaussianSigma = f; }
  void SetMaxDepth(float f)                      { m_fMaxDepth = f; }

  static constexpr const char* k_sInputDepth    = "Depth";
  static constexpr const char* k_sOutputLinear  = "LinearDepth";

private:
  xiiGALPipelineStateHandle          m_hLinearisePSO;
  xiiEnum<xiiDepthNoiseModel>        m_NoiseModel    = xiiDepthNoiseModel::None;
  float                              m_fGaussianSigma= 0.005f;
  float                              m_fMaxDepth     = 100.0f;
  bool                               m_bInitialised  = false;
};

// ============================================================
//  Optical Flow Pass
// ============================================================

/// \brief Converts the screen-space velocity buffer to per-pixel optical flow in pixel/frame units.
///
/// Output format: R16G16_FLOAT.  Values match the OpenCV / ROS optical flow convention
/// (positive X = rightward, positive Y = downward motion in screen space).
class XII_GRAPHICSCORE_DLL xiiOpticalFlowPass : public xiiRenderGraphPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOpticalFlowPass, xiiRenderGraphPass);
public:
  xiiOpticalFlowPass();
  ~xiiOpticalFlowPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view,
    const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs,
    xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void Execute(const xiiRenderViewContext& ctx,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const xiiRenderViewContext& ctx,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  static constexpr const char* k_sInputVelocity   = "Velocity";
  static constexpr const char* k_sOutputFlow       = "OpticalFlow";

private:
  xiiGALPipelineStateHandle m_hFlowPSO;
  bool m_bInitialised = false;
};

// ============================================================
//  Surface Normal Ground Truth Pass
// ============================================================

/// \brief Outputs world-space and camera-space normal maps for synthetic training data.
///
/// Two-channel output: RT0 = world-space normals (RGB16F), RT1 = camera-space normals (RGB16F).
/// Reconstructed from the GBuffer normal RT; does not require a separate geometry pass.
class XII_GRAPHICSCORE_DLL xiiNormalGroundTruthPass : public xiiRenderGraphPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNormalGroundTruthPass, xiiRenderGraphPass);
public:
  xiiNormalGroundTruthPass();
  ~xiiNormalGroundTruthPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view,
    const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs,
    xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void Execute(const xiiRenderViewContext& ctx,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const xiiRenderViewContext& ctx,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs,
    const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  static constexpr const char* k_sInputNormalSM        = "NormalSM";
  static constexpr const char* k_sOutputWorldNormals    = "WorldNormals";
  static constexpr const char* k_sOutputCameraNormals   = "CameraNormals";

private:
  xiiGALPipelineStateHandle m_hNormalPSO;
  bool m_bInitialised = false;
};
