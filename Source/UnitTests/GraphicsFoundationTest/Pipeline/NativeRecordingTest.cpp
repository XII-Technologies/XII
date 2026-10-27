/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>

XII_CREATE_SIMPLE_TEST_GROUP(NativeRecording);
XII_CREATE_SIMPLE_TEST(NativeRecording, FailureStopsDependentOutputAndRecovers)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(context.Initialize("Native recording failure")))
    return;
  for (xiiUInt32 phase = 0; phase < 3; ++phase)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
    graph.BeginSetup(frame.GetFrameIndex());
    struct Data
    {
      xiiRenderGraphTextureHandle m_Color;
      xiiUInt32                   m_uiPhase;
    };
    bool       dependentRecorded = false;
    const auto source            = graph.AddPass<Data>("Native raster", xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_uiPhase=phase;
      xiiGALTextureCreationDescription description; description.m_Type=xiiGALResourceDimension::Texture2D; description.m_Size={8,8};
      description.m_Format=xiiGALResourceFormat::RGBA8UNormalized; description.m_BindFlags=xiiGALBindFlags::RenderTarget|xiiGALBindFlags::ShaderResource;
      data.m_Color=builder.WriteTexture("Native output",description,xiiGALResourceStateFlags::RenderTarget);
      builder.SetPassRenderPassManaged();
      builder.SetPassPreparation([&data](xiiRenderGraphPassContext& execute) {
        if(data.m_uiPhase==0) execute.ReportFailure("Injected preparation failure.");
      }); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands=execute.GetCommandList();
      auto* texture=execute.GetTexture(data.m_Color);
      const auto state=texture->GetResourceState();
      commands.ClearRenderTargetView(texture->GetDefaultView(xiiGALTextureViewType::RenderTarget),xiiColor(0,1,0,1));
      if(texture->GetResourceState()!=state)
      {
        xiiGALStateTransitionDescription transition;
        transition.m_pResource=texture; transition.m_NewState=state; transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState;
        commands.TransitionResourceStates(xiiMakeArrayPtr(&transition,1));
      }
      if(data.m_uiPhase==1) execute.ReportFailure("Injected native raster pipeline failure."); });
    graph.AddPass<xiiRenderGraphTextureHandle>("Dependent display output", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
      data=builder.ReadTexture(source.first->m_Color,xiiGALResourceStateFlags::ShaderResource);
      builder.ExportTexture(data,xiiGALResourceStateFlags::ShaderResource); }, [&](const auto&, xiiRenderGraphPassContext&) { dependentRecorded = true; }, true);
    graph.EndSetup();
    xiiStringBuilder error;
    if (!XII_TEST_RESULT(graph.Compile({}, &error)))
      return;
    const auto executed = graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error);
    environment.GetDevice()->WaitIdle();
    if (phase < 2)
    {
      XII_TEST_BOOL(executed.Failed() && !dependentRecorded);
      XII_TEST_BOOL(error.FindSubString(phase == 0 ? "preparation failure" : "raster pipeline failure") != nullptr);
    }
    else
    {
      XII_TEST_RESULT(executed);
      XII_TEST_BOOL(dependentRecorded);
      xiiUInt32 pixel = 0;
      if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), graph.GetExportedTexture(source.first->m_Color).Borrow(), 4, 4, pixel))
        return;
      XII_TEST_INT(pixel, 0xFF00FF00U);
    }
  }
}
