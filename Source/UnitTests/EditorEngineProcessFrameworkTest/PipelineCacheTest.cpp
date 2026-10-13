/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

namespace
{
  class PipelineRequest : public xiiThread
  {
  public:
    PipelineRequest() : xiiThread("Concurrent pipeline request", 256 * 1024) {}

    xiiThreadSignal*                              m_pStart = nullptr;
    xiiGALComputePipelineStateCreationDescription m_Description;
    xiiSharedPtr<xiiGALComputePipelineState>      m_Result;

  private:
    xiiUInt32 Run() override
    {
      m_pStart->WaitForSignal();
      m_Result = xiiGALPipelineCache::GetPipeline(m_Description);
      return 0;
    }
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, PipelineCacheConcurrency)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  XII_TEST_BOOL(!xiiShaderPermutationUtilities::PreloadSinglePermutation({}, {}, false).IsValid());
  XII_TEST_BOOL(!xiiShaderPermutationUtilities::PreloadSinglePermutation({}, {}, true).IsValid());
  const auto shader      = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Pipeline/ColorGrading.xiiShader");
  const auto permutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(shader, {}, false);
  if (!XII_TEST_BOOL(permutation.IsValid()))
    return;
  xiiResourceLock<xiiShaderPermutationResource> program(permutation, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(program.IsValid() && program->IsShaderValid()))
    return;
  xiiGALComputePipelineStateCreationDescription description;
  description.m_pComputeShader             = program->GetGALShader(xiiGALShaderType::Compute);
  description.m_pPipelineResourceSignature = program->GetPipelineResourceSignature();

  if (environment.GetDevice()->GetFeatures().m_MultithreadedResourceCreation == xiiGALDeviceFeatureState::Enabled)
  {
    xiiThreadSignal start(xiiThreadSignal::Mode::ManualReset);
    PipelineRequest requests[8];
    for (auto& request : requests)
    {
      request.m_pStart      = &start;
      request.m_Description = description;
      request.Start();
    }
    start.RaiseSignal();
    for (auto& request : requests)
      request.Join();
    for (const auto& request : requests)
    {
      XII_TEST_BOOL(request.m_Result != nullptr);
      XII_TEST_BOOL(request.m_Result == requests[0].m_Result);
    }
    XII_TEST_BOOL(xiiGALPipelineCache::GetPipeline(description) == requests[0].m_Result);
  }
  else
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Concurrent pipeline creation requires multithreaded resource creation support.");
    const auto pipeline = xiiGALPipelineCache::GetPipeline(description);
    XII_TEST_BOOL(pipeline != nullptr);
    XII_TEST_BOOL(xiiGALPipelineCache::GetPipeline(description) == pipeline);
  }
}
