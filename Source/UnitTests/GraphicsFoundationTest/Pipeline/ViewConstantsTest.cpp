/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/World/World.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/ViewConstantsPass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

XII_CREATE_SIMPLE_TEST(PipelineResources, ViewConstantSnapshots)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiWorldDescription description("Camera constant snapshots");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  auto*         module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView*      views[2];
  xiiCamera     cameras[2];
  xiiViewHandle handles[2];
  for (xiiUInt32 index = 0; index < 2; ++index)
  {
    handles[index] = module->CreateView(index ? "Camera B" : "Camera A", views[index]);
    cameras[index].SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.25f, 100);
    cameras[index].LookAt({1.0f + index * 10.0f, 2, 3}, {2.0f + index * 10.0f, 2, 3}, {0, 0, 1});
    views[index]->SetCamera(&cameras[index]);
    views[index]->SetViewport({0, 0, 16, 16});
  }
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin(2)))
    return;
  xiiRenderGraph              graph("Independent camera constants");
  xiiRenderGraphBlackboard    blackboard;
  xiiRenderGraphResourceCache cache;
  cache.Initialize(environment.GetDeviceShared());
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame());
  graph.BeginSetup(frame.GetFrameIndex());
  xiiViewConstantsPass       operations[2];
  xiiRenderGraphBufferHandle cameraInputs[2], globalInputs[2];
  const xiiVariantDictionary parameters;
  for (xiiUInt32 index = 0; index < 2; ++index)
  {
    XII_TEST_BOOL(operations[index].GetExtractionRequirements().IsNoFlagSet());
    XII_TEST_BOOL(operations[index].GetRuntimeFeatures().IsNoFlagSet());
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    operations[index].GetPins(pins);
    xiiDynamicArray<xiiRenderPipelineBinding> bindings;
    bindings.SetCount(pins.GetCount());
    xiiRenderPipelinePassContext context{views[index], graph, blackboard, parameters, pins, bindings, index ? "B" : "A"};
    XII_TEST_RESULT(operations[index].Build(context));
    cameraInputs[index] = bindings[context.FindPin("PerFrameCamera", false)].m_hBuffer;
    globalInputs[index] = bindings[context.FindPin("PerFrameGlobal", false)].m_hBuffer;
  }
  // Execution must consume extracted frame values, even after the live camera changes.
  cameras[0].LookAt({100, 200, 300}, {101, 200, 300}, {0, 0, 1});
  struct CopyData
  {
    xiiRenderGraphBufferHandle m_Source[4], m_Target[4];
  };
  const auto copied = graph.AddPass<CopyData>("Read camera snapshots", xiiGALCommandQueueFlags::Transfer, [&](CopyData& data, xiiRenderGraphBuilder& builder) {
      for (xiiUInt32 index = 0; index < 4; ++index)
      {
        data.m_Source[index] = builder.ReadBuffer(index < 2 ? cameraInputs[index] : globalInputs[index - 2], xiiGALResourceStateFlags::CopySource);
        xiiGALBufferCreationDescription staging;
        staging.m_uiSize = index < 2 ? sizeof(xiiFrameCameraConstants) : sizeof(xiiFrameGlobalConstants);
        staging.m_Usage = xiiGALResourceUsage::Staging;
        staging.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
        xiiStringBuilder name;
        name.SetFormat("Frame constant readback {}", index);
        data.m_Target[index] = builder.WriteBuffer(name, staging, xiiGALResourceStateFlags::CopyDestination);
        builder.ExportBuffer(data.m_Target[index], xiiGALResourceStateFlags::CopyDestination);
      } }, [](const CopyData& data, xiiRenderGraphPassContext& execute) {
      for (xiiUInt32 index = 0; index < 4; ++index)
        execute.GetCommandList().CopyBuffer(execute.GetBuffer(data.m_Source[index]), execute.GetBuffer(data.m_Target[index])); }, true);
  graph.EndSetup();
  if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &blackboard, &cache)))
    return;
  environment.GetDevice()->WaitIdle();
  for (xiiUInt32 index = 0; index < 4; ++index)
  {
    auto commands = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    auto buffer = graph.GetExportedBuffer(copied.first->m_Target[index]);
    if (!XII_TEST_BOOL(buffer != nullptr))
      return;
    commands->Begin();
    void* mapped = nullptr;
    if (XII_TEST_RESULT(commands->MapBuffer(buffer.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, mapped)))
    {
      if (index < 2)
      {
        const auto& camera = *static_cast<const xiiFrameCameraConstants*>(mapped);
        XII_TEST_FLOAT(camera.m_CameraPositionAndNearPlane.x, 1.0f + index * 10, 0.0001f);
        XII_TEST_FLOAT(camera.m_CameraPositionAndNearPlane.w, 0.25f, 0.0001f);
      }
      else
        XII_TEST_INT(static_cast<const xiiFrameGlobalConstants*>(mapped)->m_uiFrameIndex, frame.GetFrameIndex());
      XII_TEST_RESULT(commands->UnmapBuffer(buffer.Borrow(), xiiGALMapType::Read));
    }
    commands->End();
  }
  for (auto handle : handles)
    module->DestroyView(handle);
}
