#pragma once

#include <GraphicsCore/../../../Data/Base/Shaders/Pipeline/LSAOConstants.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Shader/ConstantBufferStorage.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/GraphicsFoundationDLL.h>

/// \brief Defines the depth compare function to be used to decide sample weights.
struct XII_GRAPHICSCORE_DLL xiiLSAODepthCompareFunction
{
  using StorageType = xiiUInt8;

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

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLSAODepthCompareFunction);

/// Screen space ambient occlusion using "line sweep ambient occlusion" by Ville Timonen
///
/// Resources:
/// Use in Quantum Break: http://wili.cc/research/quantum_break/SIGGRAPH_2015_Remedy_Notes.pdf
/// Presentation slides EGSR: http://wili.cc/research/lsao/EGSR13_LSAO.pdf
/// Paper: http://wili.cc/research/lsao/lsao.pdf
///
/// There are a few adjustments and own ideas worked into this implementation.
/// The biggest change probably is that pixels in the gather pass compute their target linesample arithmetically instead of relying on lookups.
class XII_GRAPHICSCORE_DLL xiiLSAOPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLSAOPass, xiiRenderPipelinePass);

public:
  xiiLSAOPass();
  ~xiiLSAOPass();

  virtual bool GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;

  virtual void      Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual void      ExecuteInactive(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;

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

  bool m_bSweepDataDirty = true;
  bool m_bConstantsDirty = true;

  /// Output of the line sweep pass.
  xiiGALBufferHandle              m_hLineSweepOutputBuffer;
  xiiGALUnorderedAccessViewHandle m_hLineSweepOutputUAV;
  xiiGALResourceViewHandle        m_hLineSweepOutputSRV;

  /// Structured buffer containing instructions for every single line to trace.
  xiiGALBufferHandle       m_hLineInfoBuffer;
  xiiGALResourceViewHandle m_hLineSweepInfoSRV;

  /// Total number of lines to be traced.
  xiiUInt32 m_uiNumSweepLines = 0;

  xiiInt32 m_iLineToLinePixelOffset       = 2;
  xiiInt32 m_iLineSamplePixelOffsetFactor = 1;
  float    m_fOcclusionFalloff            = 0.2f;
  float    m_fDepthCutoffDistance         = 4.0f;

  xiiEnum<xiiLSAODepthCompareFunction> m_DepthCompareFunction;
  bool                                 m_bDistributedGathering = true;

  xiiShaderResourceHandle m_hShaderLineSweep;
  xiiShaderResourceHandle m_hShaderGather;
  xiiShaderResourceHandle m_hShaderAverage;
};
