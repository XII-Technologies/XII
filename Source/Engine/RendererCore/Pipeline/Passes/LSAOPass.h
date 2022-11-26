#pragma once

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/LSAOConstants.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief Defines the depth compare function to be used to decide sample weights.
struct XII_RENDERERCORE_DLL xiiLSAODepthCompareFunction
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Depth,                   ///< A hard cutoff function between the linear depth values. Samples with an absolute distance greater than
                             ///< xiiLSAOPass::SetDepthCutoffDistance are ignored.
    Normal,                  ///< Samples that are on the same plane as constructed by the center position and normal will be weighted higher than those samples that
                             ///< are above or below the plane.
    NormalAndSampleDistance, ///< Same as Normal, but if two samples are tested, their distance to the center position is is inversely multiplied as
                             ///< well, giving closer matches a higher weight.
    Default = NormalAndSampleDistance
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERCORE_DLL, xiiLSAODepthCompareFunction);

/// Screen space ambient occlusion using "line sweep ambient occlusion" by Ville Timonen
///
/// Resources:
/// Use in Quantum Break: http://wili.cc/research/quantum_break/SIGGRAPH_2015_Remedy_Notes.pdf
/// Presentation slides EGSR: http://wili.cc/research/lsao/EGSR13_LSAO.pdf
/// Paper: http://wili.cc/research/lsao/lsao.pdf
///
/// There are a few adjustments and own ideas worked into this implementation.
/// The biggest change probably is that pixels in the gather pass compute their target linesample arithmetically instead of relying on lookups.
class XII_RENDERERCORE_DLL xiiLSAOPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLSAOPass, xiiRenderPipelinePass);

public:
  xiiLSAOPass();
  ~xiiLSAOPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  virtual void Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  xiiUInt32 GetLineToLinePixelOffset() const { return m_iLineToLinePixelOffset; }
  void      SetLineToLinePixelOffset(xiiUInt32 uiPixelOffset);
  xiiUInt32 GetLineSamplePixelOffset() const { return m_iLineSamplePixelOffsetFactor; }
  void      SetLineSamplePixelOffset(xiiUInt32 uiPixelOffset);

  // Factor used for depth cutoffs (determines when a depth difference is too large to be considered)
  float GetDepthCutoffDistance() const;
  void  SetDepthCutoffDistance(float fDepthCutoffDistance);

  // Determines how quickly the occlusion falls of.
  float GetOcclusionFalloff() const;
  void  SetOcclusionFalloff(float fFalloff);


protected:
  /// Destroys all GPU data that might have been created in in SetupLineSweepData
  void DestroyLineSweepData();
  void SetupLineSweepData(const xiiVec3I32& imageResolution);


  void AddLinesForDirection(const xiiVec3I32& imageResolution, const xiiVec2I32& sampleDir, xiiUInt32 lineIndex, xiiDynamicArray<LineInstruction>& outinLineInstructions, xiiUInt32& outinTotalNumberOfSamples);

  xiiRenderPipelineNodeInputPin  m_PinDepthInput;
  xiiRenderPipelineNodeOutputPin m_PinOutput;

  xiiConstantBufferStorageHandle m_hLineSweepCB;

  bool m_bSweepDataDirty;

  /// Output of the line sweep pass.
  xiiGALBufferHandle              m_hLineSweepOutputBuffer;
  xiiGALUnorderedAccessViewHandle m_hLineSweepOutputUAV;
  xiiGALResourceViewHandle        m_hLineSweepOutputSRV;

  /// Structured buffer containing instructions for every single line to trace.
  xiiGALBufferHandle       m_hLineInfoBuffer;
  xiiGALResourceViewHandle m_hLineSweepInfoSRV;

  /// Total number of lines to be traced.
  xiiUInt32 m_uiNumSweepLines;

  xiiInt32                             m_iLineToLinePixelOffset;
  xiiInt32                             m_iLineSamplePixelOffsetFactor;
  xiiEnum<xiiLSAODepthCompareFunction> m_DepthCompareFunction;
  bool                                 m_bDistributedGathering;

  xiiShaderResourceHandle m_hShaderLineSweep;
  xiiShaderResourceHandle m_hShaderGather;
  xiiShaderResourceHandle m_hShaderAverage;
};
