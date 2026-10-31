/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <Shaders/Pipeline/Passes/Exposure/ExposureAdaptationConstants.h>

XII_CREATE_SIMPLE_TEST(EditorViews, MaterialExposure)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  const auto device = environment.GetDeviceShared();
  xiiGALDevice::SetDefaultDevice(device);
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(device->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                    resource    = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/ExposureAdaptation.xiiShader");
  const auto                                    permutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(resource, {}, false);
  xiiResourceLock<xiiShaderPermutationResource> shader(permutation, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(shader.IsValid() && shader->IsShaderValid()))
    return;
  xiiGALComputePipelineStateCreationDescription pipelineDescription;
  pipelineDescription.m_pComputeShader             = shader->GetGALShader(xiiGALShaderType::Compute);
  pipelineDescription.m_pPipelineResourceSignature = shader->GetPipelineResourceSignature();
  auto pipeline                                    = device->CreateComputePipelineState(pipelineDescription);
  if (!XII_TEST_BOOL(pipeline != nullptr))
    return;

  xiiExposureAdaptationConstants constants{};
  constants.LogLuminanceRange = xiiVec4(-12, 20, 32, 0);
  constants.Metering          = xiiVec4(0.005f, 0.98f, 3, 1);
  constants.Exposure          = xiiVec4(0.1f, -8, 16, 0);
  constants.ManualExposure    = 0.25f;
  constants.AutomaticExposure = 1;
  xiiGALBufferCreationDescription uniformDescription;
  uniformDescription.m_uiSize    = sizeof(constants);
  uniformDescription.m_BindFlags = xiiGALBindFlags::UniformBuffer;
  xiiGALBufferData uniformData(&constants, sizeof(constants));
  auto             uniform = device->CreateBuffer(uniformDescription, &uniformData);

  float measured[2] = {};
  for (xiiUInt32 background = 0; background < 2; ++background)
  {
    xiiUInt32 bins[256] = {};
    bins[128]           = 20;
    bins[0]             = background ? 100000 : 0;
    xiiGALBufferCreationDescription description;
    description.m_uiSize              = sizeof(bins);
    description.m_uiElementByteStride = sizeof(xiiUInt32);
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    xiiGALBufferData histogramData(bins, sizeof(bins));
    auto             histogram        = device->CreateBuffer(description, &histogramData);
    description.m_uiSize              = sizeof(xiiVec2);
    description.m_uiElementByteStride = sizeof(float);
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    xiiVec2          initial          = xiiVec2::MakeZero();
    xiiGALBufferData exposureData(&initial, sizeof(initial));
    auto             exposure = device->CreateBuffer(description, &exposureData);
    auto             commands = device->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    if (!XII_TEST_BOOL(uniform && histogram && exposure && commands))
      return;
    commands->Begin();
    commands->SetPipelineState(pipeline.Borrow());
    commands->ResolveAndSetConstantBuffer("xiiExposureAdaptationConstants", uniform.Borrow(), xiiGALShaderType::Compute);
    commands->ResolveAndSetShaderResourceBufferView("g_Histogram", histogram->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    commands->ResolveAndSetUnorderedAccessBufferView("g_Exposure", exposure->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    XII_TEST_RESULT(commands->CommitShaderResources(xiiGALStateTransitionMode::Transition));
    commands->DispatchCompute({1, 1, 1});
    commands->End();
    device->GetCommandQueue()->Submit(commands);
    device->WaitIdle();
    xiiVec2 result;
    if (!xiiGpuTestReadback::ReadBufferRecord(*device, exposure.Borrow(), 0, result))
      return;
    XII_TEST_BOOL(result.IsValid() && result.x > 0 && result.y > 0);
    measured[background] = result.x;
  }
  XII_TEST_FLOAT(measured[0], measured[1], 1e-7f);
}
