/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/Input/InputManager.h>
#include <Core/System/Window.h>
#include <Core/World/World.h>
#include <DearImguiPlugin/DearImguiInput.h>
#include <DearImguiPlugin/DearImguiSubsystem.h>
#include <DearImguiPlugin/Pipeline/DearImguiRenderPass.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

/// Two independent tools plus an offscreen mirror in the first tool's world. Uses xiiApplication directly.
class xiiDearImguiApp final : public xiiApplication
{
public:
  xiiDearImguiApp() : xiiApplication("Dear ImGui") {}

  void AfterCoreSystemsStartup() override
  {
    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiFileSystem::AddDataDirectory(">sdk/Output", "ShaderCache", "shadercache", xiiDataDirUsage::AllowWrites).AssertSuccess();
    xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").AssertSuccess();
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      auto&                        tool = m_Tools[i];
      xiiWindowCreationDescription window;
      window.m_Title            = i ? "Dear ImGui - independent tool" : "Dear ImGui - XII native overlay";
      window.m_uiWindowNumber   = xiiWindow::GetNextUnusedWindowNumber();
      window.m_Resolution       = {720, 480};
      window.m_WindowMode       = xiiWindowMode::WindowResizable;
      window.m_bShowMouseCursor = true;
      window.m_bClipMouseCursor = false;
      window.m_Position         = {100 + static_cast<xiiInt32>(i) * 760, 100};
      tool.m_pWindow            = XII_DEFAULT_NEW(xiiWindow);
      tool.m_pWindow->Initialize(window).AssertSuccess();
      tool.m_pWindow->GetWindowEvents().AddEventHandler([this, i](const xiiWindowEvent& event) {
        if (event.m_Type == xiiWindowEvent::Type::CloseButtonClicked)
          m_Tools[i].m_bClose = true;
      });
    }
    xiiStringView shaderModel, compiler;
#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
    const char* defaultRenderer = "Vulkan";
#else
    const char* defaultRenderer = "D3D12";
#endif
    const auto renderer = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, defaultRenderer);
    xiiGALDeviceFactory::GetShaderModelAndCompiler(renderer, shaderModel, compiler);
    xiiGALShaderManager::Configure(shaderModel, true);
    xiiPlugin::LoadPlugin(compiler).AssertSuccess();
    xiiGALDeviceCreationDescription device;
    device.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
    m_pDevice                = xiiGALDeviceFactory::CreateDevice(renderer, xiiFoundation::GetDefaultAllocator(), device);
    XII_ASSERT_ALWAYS(m_pDevice, "Renderer is unavailable.");
    m_pDevice->Initialize().AssertSuccess();
    xiiGALDevice::SetDefaultDevice(m_pDevice);
    xiiStartup::StartupHighLevelSystems();

    xiiRenderPipelineResourceDescriptor definition;
    definition.AddPass("Dear ImGui", xiiGetStaticRTTI<xiiImguiRenderPass>());
    definition.m_Interface.PushBack({"Color", true, definition.m_Passes[0].m_Id, "Color"});
    definition.m_Interface.PushBack({"UI", true, definition.m_Passes[0].m_Id, "UI"});
    definition.m_Interface.PushBack({"Output", false, definition.m_Passes[0].m_Id, "Color"});
    xiiRenderPipeline::Compile(definition, m_pPipeline).AssertSuccess();
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      auto& tool = m_Tools[i];
      xiiImguiSubsystem::CreateContext(tool.m_hContext).AssertSuccess();
      tool.m_TextSubscription = tool.m_pWindow->GetInputDevice()->GetTextInputEvents().AddEventHandler([this, i](xiiUInt32 character) {
        xiiImguiInput::Text(m_Tools[i].m_hContext, character).AssertSuccess();
      });
      tool.m_SlotSubscription = tool.m_pWindow->GetInputDevice()->GetInputSlotEvents().AddEventHandler([this, i](const xiiInputDevice::InputSlotEvent& event) {
        xiiImguiInput::InputSlot(m_Tools[i].m_hContext, *m_Tools[i].m_pWindow->GetInputDevice(), event.m_sSlot, event.m_fValue).AssertSuccess();
      });
      xiiWorldDescription world(i ? "Independent tool world" : "Main tool world");
      tool.m_pWorld = XII_DEFAULT_NEW(xiiWorld, world);
      {
        XII_LOCK(tool.m_pWorld->GetWriteMarker());
        tool.m_pWorld->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("UI destination", tool.m_pView);
      }
      tool.m_Pipeline.Initialize(m_pPipeline).AssertSuccess();
      xiiGALSwapChainCreationDescription swapchain;
      swapchain.m_pWindow           = tool.m_pWindow.Borrow();
      swapchain.m_ColorBufferFormat = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      swapchain.m_UsageFlags        = xiiGALSwapChainUsageFlags::RenderTarget;
      tool.m_pSwapChain             = m_pDevice->CreateSwapChain(swapchain);
      XII_ASSERT_ALWAYS(tool.m_pSwapChain, "Swapchain creation failed.");
      tool.m_hGraph = xiiRenderGraphManager::CreateGraph("Dear ImGui tool", [this, i](xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiView*) {
        return Build(graph, blackboard, m_Tools[i].m_Pipeline, m_Tools[i].m_pView, m_Tools[i].m_pSwapChain->GetBackBufferTexture(), m_Tools[i].m_pFrame, true);
      });
    }
    {
      XII_LOCK(m_Tools[0].m_pWorld->GetWriteMarker());
      m_Tools[0].m_pWorld->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Shared context offscreen mirror", m_pMirrorView);
      m_pMirrorView->SetViewport({16, 12, 720, 480});
    }
    xiiGALTextureCreationDescription mirror;
    mirror.m_Type      = xiiGALResourceDimension::Texture2D;
    mirror.m_Size      = {752, 504};
    mirror.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    mirror.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    m_pMirror          = m_pDevice->CreateTexture(mirror);
    m_MirrorPipeline.Initialize(m_pPipeline).AssertSuccess();
    m_hMirrorGraph = xiiRenderGraphManager::CreateGraph("Dear ImGui mirror", [this](xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiView*) {
      return Build(graph, blackboard, m_MirrorPipeline, m_pMirrorView, m_pMirror, m_Tools[0].m_pFrame, false);
    });
  }

  Execution Run() override
  {
    for (auto& tool : m_Tools)
      if (tool.m_pWindow)
        tool.m_pWindow->ProcessWindowMessages();
    xiiClock::GetGlobalClock()->Update();
    const auto delta = xiiClock::GetGlobalClock()->GetTimeDiff();
    xiiInputManager::PollHardware();
    for (auto& tool : m_Tools)
      if (!tool.m_bClose && tool.m_pWindow)
      {
        const auto size = tool.m_pWindow->GetClientAreaSize();
        xiiImguiInput::UpdateFromDevice(tool.m_hContext, *tool.m_pWindow->GetInputDevice(), {float(size.width), float(size.height)}).AssertSuccess();
      }
    xiiInputManager::Update(delta);

    bool anyOpen = false;
    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      auto& tool = m_Tools[i];
      if (tool.m_bClose)
      {
        CloseTool(tool);
        continue;
      }
      anyOpen         = true;
      const auto size = tool.m_pWindow->GetClientAreaSize();
      if (!size.HasNonZeroArea())
        continue;
      if (size != tool.m_pSwapChain->GetCurrentSize())
        tool.m_pSwapChain->Resize(size).AssertSuccess();
      tool.m_pView->SetViewport({0, 0, float(size.width), float(size.height)});
      xiiImguiScopedContext    scope(tool.m_hContext);
      xiiImguiFrameDescription frame;
      frame.m_vDisplaySize = {float(size.width), float(size.height)};
      frame.m_DeltaTime    = delta;
      scope.BeginFrame(frame).AssertSuccess();
      if (tool.m_bShowUI)
      {
        ImGui::SetNextWindowPos({30, 30}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({440, 330}, ImGuiCond_FirstUseEver);
        ImGui::Begin(i ? "Independent tool" : "XII Dear ImGui");
        ImGui::TextUnformatted("Rendered by xiiImguiRenderPass through XII GAL");
        ImGui::SliderFloat("Value", &tool.m_fValue, 0, 1);
        ImGui::InputText("Text", tool.m_Text, XII_ARRAY_SIZE(tool.m_Text));
        if (ImGui::Button("Increment"))
          ++tool.m_uiClicks;
        ImGui::SameLine();
        ImGui::Text("Count: %u", tool.m_uiClicks);
        ImGui::Text("Mouse capture: %s", scope.GetInputCapture().m_bMouse ? "yes" : "no");
        ImGui::TextUnformatted("Resize either window; each tool owns its input and UI state.");
        if (ImGui::Button("Submit empty frames"))
          tool.m_bShowUI = false;
        ImGui::End();
      }
      scope.EndFrame(tool.m_pFrame).AssertSuccess();
    }
    if (!anyOpen || WasQuitRequested())
      return Execution::Quit;
    m_pDevice->BeginFrame();
    {
      xiiRenderGraphFrame frame;
      frame.Begin().AssertSuccess();
      for (auto& tool : m_Tools)
        if (tool.m_pWindow && tool.m_pWindow->IsVisible() && tool.m_pSwapChain->GetCurrentSize().HasNonZeroArea())
        {
          xiiStringBuilder error;
          if (xiiRenderGraphManager::ExecuteGraph(frame, tool.m_hGraph, tool.m_pView, {}, &error).Failed())
          {
            xiiLog::Error("Dear ImGui tool rendering failed: {0}", error);
            SetReturnCode(1);
            RequestQuit();
          }
          tool.m_pSwapChain->Present();
        }
      if (m_Tools[0].m_pFrame)
      {
        xiiStringBuilder error;
        if (xiiRenderGraphManager::ExecuteGraph(frame, m_hMirrorGraph, m_pMirrorView, {}, &error).Failed())
        {
          xiiLog::Error("Dear ImGui offscreen rendering failed: {0}", error);
          SetReturnCode(1);
          RequestQuit();
        }
      }
    }
    m_pDevice->EndFrame();
    xiiResourceManager::PerFrameUpdate();
    xiiTaskSystem::FinishFrameTasks();
    const auto limit = xiiCommandLineUtils::GetGlobalInstance()->GetIntOption("-frames", 0);
    return limit > 0 && ++m_uiFrames >= static_cast<xiiUInt32>(limit) ? Execution::Quit : Execution::Continue;
  }

  void BeforeHighLevelSystemsShutdown() override
  {
    m_pDevice->WaitIdle();
    xiiRenderGraphManager::DestroyGraph(m_hMirrorGraph).AssertSuccess();
    m_hMirrorGraph.Invalidate();
    m_MirrorPipeline.Clear();
    m_pMirror.Clear();
    for (auto& tool : m_Tools)
      CloseTool(tool);
    for (auto& tool : m_Tools)
      tool.m_pWorld.Clear();
    m_pMirrorView = nullptr;
    m_pPipeline.Clear();
    xiiStartup::ShutdownHighLevelSystems();
    xiiGALDevice::SetDefaultDevice(nullptr);
    m_pDevice.Clear();
  }

private:
  struct Tool
  {
    xiiUniquePtr<xiiWindow>           m_pWindow;
    xiiUniquePtr<xiiWorld>            m_pWorld;
    xiiView*                          m_pView = nullptr;
    xiiSharedPtr<xiiGALSwapChain>     m_pSwapChain;
    xiiImguiContextHandle             m_hContext;
    xiiSharedPtr<const xiiImguiFrame> m_pFrame;
    xiiRenderGraphHandle              m_hGraph;
    xiiRenderPipelineInstance         m_Pipeline;
    float                             m_fValue           = 0.5f;
    char                              m_Text[128]        = "Type here";
    xiiUInt32                         m_uiClicks         = 0;
    xiiEventSubscriptionID            m_TextSubscription = 0;
    xiiEventSubscriptionID            m_SlotSubscription = 0;
    bool                              m_bClose           = false;
    bool                              m_bShowUI          = true;
  };

  void CloseTool(Tool& tool)
  {
    if (!tool.m_pWindow)
      return;
    xiiRenderGraphManager::DestroyGraph(tool.m_hGraph).AssertSuccess();
    tool.m_hGraph.Invalidate();
    tool.m_Pipeline.Clear();
    tool.m_pFrame.Clear();
    tool.m_pWindow->GetInputDevice()->GetTextInputEvents().RemoveEventHandler(tool.m_TextSubscription);
    tool.m_pWindow->GetInputDevice()->GetInputSlotEvents().RemoveEventHandler(tool.m_SlotSubscription);
    xiiImguiSubsystem::DestroyContext(tool.m_hContext).AssertSuccess();
    tool.m_hContext.Invalidate();
    tool.m_pSwapChain.Clear();
    // The main world's mirror view remains live until its graph is destroyed at shutdown.
    if (&tool != &m_Tools[0] || m_hMirrorGraph.IsInvalidated())
      tool.m_pWorld.Clear();
    tool.m_pWindow->Destroy().AssertSuccess();
    tool.m_pWindow.Clear();
  }

  xiiResult Build(xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, xiiRenderPipelineInstance& pipeline, xiiView* view,
                  xiiSharedPtr<xiiGALTexture> texture, xiiSharedPtr<const xiiImguiFrame> frame, bool present)
  {
    xiiRenderGraphTextureHandle color;
    graph.AddPass<xiiRenderGraphTextureHandle>("Clear UI background", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
        color = data = builder.WriteTexture(builder.ImportTexture("UI target", texture, texture->GetResourceState()), xiiGALResourceStateFlags::RenderTarget);
        builder.SetPassRenderPassManaged(); }, [](const auto& data, xiiRenderGraphPassContext& execution) { execution.GetCommandList().ClearRenderTargetView(execution.GetTexture(data)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.025f, 0.04f, 0.065f)); });
    xiiRenderPipelineBinding binding;
    binding.m_pFrameData                         = XII_DEFAULT_NEW(xiiImguiFrameData, graph.GetSetupSerial(), std::move(frame));
    const xiiRenderPipelineInputBinding inputs[] = {
      {graph, m_pPipeline->FindInterface("Color", true), color},
      {graph, m_pPipeline->FindInterface("UI", true), binding}};
    XII_SUCCEED_OR_RETURN(pipeline.Build(view, graph, blackboard, {}, inputs));
    const auto output = pipeline.GetInterfaceBinding(m_pPipeline->FindInterface("Output", false))->m_hTexture;
    graph.AddPass<xiiRenderGraphTextureHandle>("UI output", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
        data = builder.ReadTexture(output, xiiGALResourceStateFlags::RenderTarget);
        builder.ExportTexture(data, present ? xiiGALResourceStateFlags::Present : xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    return XII_SUCCESS;
  }

  Tool                                  m_Tools[2];
  xiiSharedPtr<xiiGALDevice>            m_pDevice;
  xiiSharedPtr<const xiiRenderPipeline> m_pPipeline;
  xiiView*                              m_pMirrorView = nullptr;
  xiiSharedPtr<xiiGALTexture>           m_pMirror;
  xiiRenderPipelineInstance             m_MirrorPipeline;
  xiiRenderGraphHandle                  m_hMirrorGraph;
  xiiUInt32                             m_uiFrames = 0;
};

XII_CONSOLEAPP_ENTRY_POINT(xiiDearImguiApp);
