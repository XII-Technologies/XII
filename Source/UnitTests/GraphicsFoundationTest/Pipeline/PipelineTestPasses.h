/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Color.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <TestFramework/Framework/TestFramework.h>

class xiiIndirectTestExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiIndirectTestExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Input{xiiGALResourceFormat::RGBA32Float, false, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::RGBA32Float, false, {}, xiiGALResourceDimension::Texture2D, 1, xiiRenderPipelinePinAccess::Read};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override;
};

class xiiOpaqueTestExport : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOpaqueTestExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineTextureInputPin  m_Input{xiiGALResourceFormat::Unknown, true};
  xiiRenderPipelineTextureOutputPin m_Output{xiiGALResourceFormat::Unknown, true, {}, xiiGALResourceDimension::Undefined, 0, xiiRenderPipelinePinAccess::Read};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override;
};

class xiiScreenEffectTestSource : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScreenEffectTestSource, xiiRenderPipelinePass);

public:
  float                             m_fDepth     = 0;
  xiiColor                          m_Attributes = xiiColor::Black;
  xiiRenderPipelineTextureOutputPin m_ColorOutput{xiiGALResourceFormat::RGBA16Float};
  xiiRenderPipelineTextureOutputPin m_AttributesOutput{xiiGALResourceFormat::RGBA16Float};
  xiiRenderPipelineTextureOutputPin m_DepthOutput{xiiGALResourceFormat::R32Float};
  xiiResult                         Build(xiiRenderPipelinePassContext& context) override;
};

class xiiExtractionDemandPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiExtractionDemandPass, xiiRenderPipelinePass);

public:
  xiiUInt32                                m_uiCategories = 0;
  xiiBitflags<xiiRenderExtractionCategory> GetExtractionRequirements() const override { return static_cast<xiiRenderExtractionCategory::Enum>(m_uiCategories); }
  void                                     GetPins(xiiDynamicArray<xiiRenderPipelinePin>&) const override {}
  xiiResult                                Build(xiiRenderPipelinePassContext&) override;
};

/// Optional diagnostic capture for producers which legitimately omit a buffer.
class xiiOptionalBufferTestExport final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOptionalBufferTestExport, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferInputPin  m_Input{true, xiiGALBindFlags::ShaderResource};
  xiiRenderPipelineBufferOutputPin m_Output{true, xiiGALBindFlags::ShaderResource, xiiRenderPipelinePinAccess::Read};
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};

/// Verify the production graph allocates metering resources only in automatic mode.
inline void xiiTestExposureResources(const xiiRenderGraph& graph, bool automatic)
{
  const xiiStringView names[] = {"LuminanceHistogram", "Exposure Histogram Constants", "Exposure Adaptation Constants", "Manual Exposure"};
  for (xiiUInt32 index = 0; index < XII_ARRAY_SIZE(names); ++index)
  {
    xiiUInt32 count = 0;
    for (const auto& resource : graph.GetResourceDescriptions())
      count += !resource.m_bIsTexture && resource.m_sName.GetView().EndsWith(names[index]);
    XII_TEST_INT(count, index == 3 ? !automatic : automatic);
  }
}
