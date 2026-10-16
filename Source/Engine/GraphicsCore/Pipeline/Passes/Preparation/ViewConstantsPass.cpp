/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/FrameConstants.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/Passes/Preparation/ViewConstantsPass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiViewConstantsPass, 1, xiiRTTIDefaultAllocator<xiiViewConstantsPass>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Preparation")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiViewConstantsPass::GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const
{
  xiiRenderPassUtils::ResourcePins(pins);
  pins.PushBack({"PerFrameCamera", xiiRenderPipelinePin::Buffer, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UniformBuffer;
  pins.PushBack({"PerFrameGlobal", xiiRenderPipelinePin::Buffer, false});
  pins.PeekBack().m_Constraints.m_RequiredBindFlags = xiiGALBindFlags::UniformBuffer;
}
xiiResult xiiViewConstantsPass::Build(xiiRenderPipelinePassContext& context)
{
  if (!context.m_pView || !context.m_pView->GetCamera())
    return XII_FAILURE;
  xiiRenderPassResourceScope scope(context);
  struct Data
  {
    xiiFrameCameraConstants    m_Camera;
    xiiFrameGlobalConstants    m_Global;
    xiiRenderGraphBufferHandle m_CameraBuffer, m_GlobalBuffer;
  };
  const auto emitted = context.m_Graph.AddPass<Data>(context.m_sPassName, xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_Camera = xiiSnapshotCameraConstants(*context.m_pView);
      data.m_Global = xiiSnapshotGlobalConstants(*context.m_pView, static_cast<xiiUInt32>(context.m_Graph.GetFrameIndex()));
      data.m_CameraBuffer = xiiRenderPassUtils::Constants(builder, "PerFrameCamera", sizeof(data.m_Camera));
      data.m_GlobalBuffer = xiiRenderPassUtils::Constants(builder, "PerFrameGlobal", sizeof(data.m_Global)); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands = execute.GetCommandList();
      {
        xiiGALMapHelper<xiiFrameCameraConstants> mapped(commands, execute.GetBuffer(data.m_CameraBuffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *mapped = data.m_Camera;
      }
      {
        xiiGALMapHelper<xiiFrameGlobalConstants> mapped(commands, execute.GetBuffer(data.m_GlobalBuffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *mapped = data.m_Global;
      }
      xiiGALStateTransitionDescription camera, global;
      camera.m_pResource = execute.GetBuffer(data.m_CameraBuffer);
      camera.m_NewState = xiiGALResourceStateFlags::ConstantBuffer;
      camera.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
      global = camera;
      global.m_pResource = execute.GetBuffer(data.m_GlobalBuffer);
      commands.TransitionResourceStates(xiiMakeArrayPtr(&camera, 1));
      commands.TransitionResourceStates(xiiMakeArrayPtr(&global, 1)); });
  XII_SUCCEED_OR_RETURN(context.SetOutput("PerFrameCamera", emitted.first->m_CameraBuffer));
  return context.SetOutput("PerFrameGlobal", emitted.first->m_GlobalBuffer);
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Preparation_ViewConstantsPass);
