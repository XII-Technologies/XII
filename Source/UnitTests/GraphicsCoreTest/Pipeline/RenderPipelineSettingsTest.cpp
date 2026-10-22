/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <TestFramework/Utilities/TestLogInterface.h>

class xiiSettingsPipelinePass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSettingsPipelinePass, xiiRenderPipelinePass);

public:
  static xiiUInt32 s_uiBuilds;
  void             GetPins(xiiDynamicArray<xiiRenderPipelinePin>&) const override {}
  xiiResult        Build(xiiRenderPipelinePassContext&) override
  {
    ++s_uiBuilds;
    return XII_SUCCESS;
  }
};
xiiUInt32 xiiSettingsPipelinePass::s_uiBuilds = 0;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSettingsPipelinePass, 1, xiiRTTIDefaultAllocator<xiiSettingsPipelinePass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(Pipeline, AssetSettings)
{
  xiiRenderPipelineSettings settings;
  xiiStringBuilder          error;
  XII_TEST_RESULT(settings.Validate());
  XII_TEST_RESULT(settings.ValidateDevice(nullptr));
  XII_TEST_RESULT(settings.ValidateOutput(nullptr));
  settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute | xiiRenderPipelineDeviceFeature::RayTracing;
  xiiGALDeviceFeatures features;
  features.m_ComputeShaders = xiiGALDeviceFeatureState::Enabled;
  XII_TEST_BOOL(settings.ValidateDevice(&features, &error).Failed());
  XII_TEST_BOOL(error.FindSubString("RayTracing") != nullptr);
  features.m_RayTracing = xiiGALDeviceFeatureState::Optional;
  XII_TEST_BOOL(settings.ValidateDevice(&features).Failed());
  features.m_RayTracing = xiiGALDeviceFeatureState::Enabled;
  XII_TEST_RESULT(settings.ValidateDevice(&features));
  settings.m_OutputFormat    = xiiGALResourceFormat::RGBA8UNormalized;
  settings.m_uiOutputSamples = 1;
  xiiGALTextureCreationDescription output;
  output.m_Type          = xiiGALResourceDimension::Texture2D;
  output.m_Format        = xiiGALResourceFormat::RGBA8UNormalized;
  output.m_uiSampleCount = 1;
  XII_TEST_RESULT(settings.ValidateOutput(&output));
  XII_TEST_BOOL(settings.ValidateOutput(nullptr, &error).Failed());
  output.m_uiSampleCount = 4;
  XII_TEST_BOOL(settings.ValidateOutput(&output, &error).Failed());
  XII_TEST_BOOL(error.FindSubString("OutputSamples") != nullptr);
  output.m_uiSampleCount = 1;
  output.m_Format        = xiiGALResourceFormat::RGBA16Float;
  XII_TEST_BOOL(settings.ValidateOutput(&output, &error).Failed());
  XII_TEST_BOOL(error.FindSubString("OutputFormat") != nullptr);

  xiiRenderPipelineResourceDescriptor descriptor;
  descriptor.AddPass("Operation", xiiSettingsPipelinePass().GetDynamicRTTI());
  descriptor.m_Settings = settings;
  xiiSharedPtr<const xiiRenderPipeline> compiled;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled));
  xiiContiguousMemoryStreamStorage bytes, repeated;
  xiiMemoryStreamWriter            writer(&bytes), repeatWriter(&repeated);
  XII_TEST_RESULT(descriptor.Serialize(writer));
  XII_TEST_RESULT(descriptor.Serialize(repeatWriter));
  XII_TEST_BOOL(bytes.GetStorageSize32() == repeated.GetStorageSize32() && xiiMemoryUtils::IsEqual(bytes.GetData(), repeated.GetData(), bytes.GetStorageSize32()));
  xiiMemoryStreamReader               reader(&bytes);
  xiiRenderPipelineResourceDescriptor restored;
  XII_TEST_RESULT(restored.Deserialize(reader));
  XII_TEST_BOOL(restored.m_Settings.m_RequiredDeviceFeatures == settings.m_RequiredDeviceFeatures);
  XII_TEST_BOOL(restored.m_Settings.m_OutputFormat == settings.m_OutputFormat);
  XII_TEST_INT(restored.m_Settings.m_uiOutputSamples, 1);
  restored.m_Settings.m_uiOutputSamples = 3;
  XII_TEST_BOOL(xiiRenderPipeline::Compile(restored, compiled, &error).Failed());
  XII_TEST_BOOL(error.FindSubString("OutputSamples") != nullptr);
  restored.m_Settings.m_uiOutputSamples = 1;
  restored.m_Settings.m_RequiredDeviceFeatures.SetValue(128);
  XII_TEST_BOOL(xiiRenderPipeline::Compile(restored, compiled, &error).Failed());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Rejected pipeline output never constructs algorithm work")
  {
    // No view/output is selected. A contract failure precedes every pass Build.
    descriptor.m_Settings.m_RequiredDeviceFeatures = {};
    XII_TEST_RESULT(xiiRenderPipeline::Compile(descriptor, compiled));
    xiiRenderPipelineInstance instance;
    XII_TEST_RESULT(instance.Initialize(compiled));
    xiiRenderGraph           graph;
    xiiRenderGraphBlackboard blackboard;
    const auto               builds = xiiSettingsPipelinePass::s_uiBuilds;
    xiiTestLogInterface      log;
    xiiLogSystemScope        scope(&log);
    log.ExpectMessage("Pipeline output contract requires a selected output texture", xiiLogMsgType::ErrorMsg);
    XII_TEST_BOOL(instance.Build(nullptr, graph, blackboard).Failed());
    XII_TEST_INT(xiiSettingsPipelinePass::s_uiBuilds, builds);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Referenced definition device requirements constrain the containing resource")
  {
    xiiRenderPipelineResourceDescriptor child;
    child.AddPass("Child operation", xiiSettingsPipelinePass().GetDynamicRTTI());
    child.m_Settings.m_RequiredDeviceFeatures    = xiiRenderPipelineDeviceFeature::MeshShaders;
    const auto                          id       = child.m_Id;
    const auto                          resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("PipelineSettings/SharedDefinition", std::move(child));
    xiiRenderPipelineResourceDescriptor root;
    root.m_Settings.m_RequiredDeviceFeatures = xiiRenderPipelineDeviceFeature::Compute;
    root.AddSubgraph("First", id);
    root.AddSubgraph("Second", id);
    root.m_References.PushBack({id, resource.GetResourceID()});
    XII_TEST_RESULT(xiiRenderPipeline::Compile(root, compiled));
    const auto& requirements = compiled->GetDescriptor().m_Settings;
    XII_TEST_BOOL(requirements.m_RequiredDeviceFeatures.IsSet(xiiRenderPipelineDeviceFeature::Compute));
    XII_TEST_BOOL(requirements.m_RequiredDeviceFeatures.IsSet(xiiRenderPipelineDeviceFeature::MeshShaders));
    XII_TEST_BOOL(requirements.ValidateDevice(&features, &error).Failed());
    XII_TEST_BOOL(error.FindSubString("MeshShaders") != nullptr);
    XII_TEST_BOOL(!root.m_Settings.m_RequiredDeviceFeatures.IsSet(xiiRenderPipelineDeviceFeature::MeshShaders));
  }
}
