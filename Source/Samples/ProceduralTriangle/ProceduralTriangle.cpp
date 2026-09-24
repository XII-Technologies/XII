/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/System/Screen.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/CommandLineOptions.h>

#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderPassCache.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>

#include <Shaders/ProceduralTriangleConstants.h>

xiiCommandLineOptionInt opt_MonitorId("ProceduralTriangle", "-monitor", "The monitor to launch the application winodw.", 0U);

static bool g_bWindowResized = false;

class xiiProceduralTriangleApp : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiProceduralTriangleApp() :
    xiiApplication("Procedural Triangle")
  {
  }

  virtual Execution Run() override
  {
    {
      xiiStringBuilder sCmdHelp;
      if (xiiCommandLineOption::LogAvailableOptionsToBuffer(sCmdHelp, xiiCommandLineOption::LogAvailableModes::IfHelpRequested, "ProceduralTriangle"))
      {
        xiiLog::Print(sCmdHelp);

        return xiiApplication::Execution::Quit;
      }
    }

    m_pWindow->ProcessWindowMessages();

    if (!m_pWindow->IsVisible())
    {
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(16));
      return Execution::Continue;
    }

    if (g_bWindowResized)
    {
      g_bWindowResized = false;

      UpdateSwapChain();
    }

    if (WasQuitRequested() || xiiInputManager::GetInputActionState("Main", "CloseApp") == xiiKeyState::Pressed)
      return Execution::Quit;

    // Make sure time goes on
    xiiClock::GetGlobalClock()->Update();

    // Update all input state
    xiiInputManager::Update(xiiClock::GetGlobalClock()->GetTimeDiff());

    // Engage mouse look
    if (xiiInputManager::GetInputActionState("Main", "Look") == xiiKeyState::Down)
    {
      m_pWindow->GetInputDevice()->SetShowMouseCursor(false);
      m_pWindow->GetInputDevice()->SetClipMouseCursor(xiiMouseCursorClipMode::ClipToPosition);
    }
    else
    {
      m_pWindow->GetInputDevice()->SetShowMouseCursor(true);
      m_pWindow->GetInputDevice()->SetClipMouseCursor(xiiMouseCursorClipMode::NoClip);
    }

    // Perform rendering.
    {
      // Before starting to render in a frame call this function.
      m_pDevice->BeginFrame();

      // If swap chain or its back buffer (or depth/rederpass) are not available we must skip rendering.
      bool bCanRender = (m_pSwapChain != nullptr) && m_pSwapChain->GetCurrentSize().HasNonZeroArea() && (m_pSwapChain->GetBackBufferTexture() != nullptr);

      if (bCanRender)
      {
        // Build a minimal render graph that clears the depth and backbuffer.
        ++m_uiFrameIndex;

        m_pRenderGraph->BeginSetup(m_uiFrameIndex);
        {
          m_pRenderGraph->AddPass<OffscreenPassData>("OffscreenPass", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiProceduralTriangleApp::SetupOffscreenPass, this), xiiMakeDelegate(&xiiProceduralTriangleApp::ExecuteOffscreenPass, this));
          m_pRenderGraph->AddPass<ProceduralTrianglePassData>("ProceduralTrianglePass", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiProceduralTriangleApp::SetupProceduralTrianglePass, this), xiiMakeDelegate(&xiiProceduralTriangleApp::ExecuteProceduralTrianglePass, this));
          m_pRenderGraph->AddPass<BlitPassData>("BlitPass", xiiGALCommandQueueFlags::Graphics, xiiMakeDelegate(&xiiProceduralTriangleApp::SetupBlitPass, this), xiiMakeDelegate(&xiiProceduralTriangleApp::ExecuteBlitPass, this), /*bHasSideEffects=*/true);
        }
        m_pRenderGraph->EndSetup();

        m_pRenderGraphResourceCache->BeginFrame(m_uiFrameIndex);
        {
          xiiStringBuilder              sError;
          xiiRenderGraphCompileSettings settings;
          settings.m_bEnablePassCulling  = true;
          settings.m_bEnableCompileCache = true;
          settings.m_bEnableAsyncQueues  = true;
          settings.m_bEnableGPUProfiling = true;

          if (m_pRenderGraph->Compile(settings, &sError).Succeeded())
          {
            m_pRenderGraph->Execute(m_pDevice.Borrow(), /*pView=*/nullptr, m_pRenderGraphBlackboard.Borrow(), m_pRenderGraphResourceCache.Borrow(), m_pRenderGraphProfiler.Borrow()).AssertSuccess("RenderGraph execution failed.");
          }
          else
          {
            xiiLog::Error("RenderGraph compile failed: {0}", sError);
          }
        }
        m_pRenderGraphResourceCache->EndFrame();

        m_pSwapChain->Present();
      }
      else
      {
        // Ensure the swap chain can perform any internal throttling (e.g. when minimized)
        if (m_pSwapChain)
        {
          m_pSwapChain->Present();
        }
      }

      m_pDevice->EndFrame();
    }

    // Needs to be called once per frame
    xiiResourceManager::PerFrameUpdate();

    // Tell the task system to finish its work for this frame
    // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
    // uploading GPU data etc.
    xiiTaskSystem::FinishFrameTasks();

    return Execution::Continue;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    xiiStringBuilder sProjectDir = ">sdk/Data/Samples/ProceduralTriangle";
    xiiStringBuilder sProjectDirResolved;
    xiiFileSystem::ResolveSpecialDirectory(sProjectDir, sProjectDirResolved).IgnoreResult();
    xiiFileSystem::SetSpecialDirectory("project", sProjectDirResolved);

    xiiFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", xiiDataDirUsage::AllowWrites).AssertSuccess();
    xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").AssertSuccess();
    xiiFileSystem::AddDataDirectory(">project/", "Project", "project").AssertSuccess();

    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

    // Register Input
    {
      xiiInputActionConfig cfg;

      cfg                        = xiiInputManager::GetInputActionConfig("Main", "CloseApp");
      cfg.m_sInputSlotTrigger[0] = xiiInputSlot_KeyEscape;
      xiiInputManager::SetInputActionConfig("Main", "CloseApp", cfg, true);

      cfg                        = xiiInputManager::GetInputActionConfig("Main", "Look");
      cfg.m_sInputSlotTrigger[0] = xiiInputSlot_MouseButton0;
      cfg.m_bApplyTimeScaling    = false;
      xiiInputManager::SetInputActionConfig("Main", "Look", cfg, true);
    }

    // Create a window for rendering
    {
      xiiWindowCreationDescription WindowCreationDescription;
      WindowCreationDescription.m_Resolution.width  = 960;
      WindowCreationDescription.m_Resolution.height = 540;
      WindowCreationDescription.m_Title             = GetApplicationName();
      WindowCreationDescription.m_bShowMouseCursor  = true;
      WindowCreationDescription.m_bClipMouseCursor  = false;
      WindowCreationDescription.m_WindowMode        = xiiWindowMode::WindowResizable;
      WindowCreationDescription.m_iMonitor          = opt_MonitorId.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
      WindowCreationDescription.AdjustWindowSizeAndPosition().IgnoreResult();

      m_pWindow = XII_DEFAULT_NEW(xiiWindow);
      m_pWindow->Initialize(WindowCreationDescription).AssertSuccess();

      m_pWindow->GetWindowEvents().AddEventHandler([this](const xiiWindowEvent& e) -> void {
        if (e.m_Type == xiiWindowEvent::Type::CloseButtonClicked)
        {
          this->RequestQuit();
        }

        if (e.m_Type == xiiWindowEvent::Type::SizeChanged)
        {
          g_bWindowResized = true;
        }
      });
    }

    {
      xiiGALDeviceCreationDescription deviceCreationDescription;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
#else
      deviceCreationDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Disabled;
#endif

#if BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
      constexpr const char* szDefaultGraphicsAPI = "Vulkan";
#elif BUILDSYSTEM_ENABLE_D3D12_SUPPORT
      constexpr const char* szDefaultGraphicsAPI = "D3D12";
#else
      constexpr const char* szDefaultGraphicsAPI = "";
#endif

      xiiStringView sGraphicsAPIName = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultGraphicsAPI);
      xiiStringView sShaderModel     = {};
      xiiStringView sShaderCompiler  = {};
      xiiGALDeviceFactory::GetShaderModelAndCompiler(sGraphicsAPIName, sShaderModel, sShaderCompiler);

      xiiGALShaderManager::Configure(sShaderModel, true);
      XII_VERIFY(xiiPlugin::LoadPlugin(sShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found.", sShaderCompiler);

      m_pDevice = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), deviceCreationDescription);
      XII_ASSERT_DEV(m_pDevice != nullptr, "Device implementation for '{}' not found", sGraphicsAPIName);
      XII_VERIFY(m_pDevice->Initialize() == XII_SUCCESS, "Device initialization failed!");

      m_pDevice->SetDebugName("Master Graphics Device");

      xiiGALDevice::SetDefaultDevice(m_pDevice);
    }

    UpdateSwapChain();

    // Now that we have a window and device, tell the engine to initialize the rendering infrastructure
    xiiStartup::StartupHighLevelSystems();

    m_pRenderGraph              = XII_DEFAULT_NEW(xiiRenderGraph);
    m_pRenderGraphBlackboard    = XII_DEFAULT_NEW(xiiRenderGraphBlackboard);
    m_pRenderGraphResourceCache = XII_DEFAULT_NEW(xiiRenderGraphResourceCache);
    m_pRenderGraphProfiler      = XII_DEFAULT_NEW(xiiRenderGraphTimestampProfiler);

    m_pRenderGraphResourceCache->Initialize(m_pDevice);
    m_pRenderGraphProfiler->Initialize(m_pDevice);
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    xiiPlugin::UnloadAllPlugins();

    SUPER::BeforeCoreSystemsShutdown();
  }

  virtual void BeforeHighLevelSystemsShutdown() override
  {
    m_pRenderGraphProfiler.Clear();
    m_pRenderGraphResourceCache.Clear();
    m_pRenderGraphBlackboard.Clear();
    m_pRenderGraph.Clear();

    m_pSwapChain.Clear();

    // Tell the engine that we are about to destroy window and graphics device,
    // and that it therefore needs to cleanup anything that depends on that
    xiiStartup::ShutdownHighLevelSystems();

    if (xiiGALDevice::GetDefaultDevice() == m_pDevice)
    {
      xiiGALDevice::SetDefaultDevice(nullptr);
    }
    m_pDevice.Clear();

    // Finally destroy the window
    m_pWindow->Destroy().IgnoreResult();
    m_pWindow.Clear();
  }

  void UpdateSwapChain()
  {
    // Create a Swap Chain
    if (!m_pSwapChain)
    {
      xiiGALSwapChainCreationDescription swapChainDescription;
      swapChainDescription.m_pWindow               = m_pWindow.Borrow();
      swapChainDescription.m_ColorBufferFormat     = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      swapChainDescription.m_UsageFlags            = xiiGALSwapChainUsageFlags::RenderTarget;
      swapChainDescription.m_PreTransform          = xiiGALSurfaceTransform::Optimal;
      swapChainDescription.m_uiBufferCount         = 2U;
      swapChainDescription.m_fDefaultDepthValue    = 1.0f;
      swapChainDescription.m_uiDefaultStencilValue = 0U;

      m_pSwapChain = m_pDevice->CreateSwapChain(swapChainDescription);

      m_pSwapChain->SetPresentMode(xiiGALPresentMode::VSync);
    }
    else
    {
      auto currentSize = m_pWindow->GetClientAreaSize();

      if (m_pSwapChain->GetCurrentSize() != currentSize)
      {
        m_pSwapChain->Resize(currentSize).IgnoreResult();
      }
    }
  }

private:
  struct OffscreenPassData
  {
    xiiRenderGraphTextureHandle m_hOffScreenTexture;
    float                       m_fGlobalTime = 0.0f;
  };

  void SetupOffscreenPass(OffscreenPassData& data, xiiRenderGraphBuilder& builder)
  {
    xiiGALTextureCreationDescription textureDescription;
    textureDescription.m_Type      = xiiGALResourceDimension::Texture2D;
    textureDescription.m_Size      = m_pWindow->GetClientAreaSize();
    textureDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    textureDescription.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget;

    // This declares a new texture resource for the render graph and registers that we will write to it in this pass.
    // The returned handle references the texture at its new version, so store and use this handle for all future reads/writes.
    data.m_hOffScreenTexture = builder.WriteTexture("OffScreenTexture", textureDescription, xiiGALResourceStateFlags::RenderTarget);

    data.m_fGlobalTime = (float)xiiMath::Mod(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 360.0);
  }

  void ExecuteOffscreenPass(const OffscreenPassData& data, xiiRenderGraphPassContext& context)
  {
    xiiGALCommandList& cmd = context.GetCommandList();

    cmd.BeginDebugGroup("Offscreen Clear");
    {
      cmd.ClearRenderTargetView(context.GetTexture(data.m_hOffScreenTexture)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black);
    }
    cmd.EndDebugGroup();
  }

  struct ProceduralTrianglePassData
  {
    xiiRenderGraphTextureHandle        m_hOffScreenTexture;
    xiiRenderGraphBufferHandle         m_hTriangleConstantBuffer;
    xiiShaderResourceHandle            m_hShader;
    xiiShaderPermutationResourceHandle m_hShaderPermutation;
    xiiSharedPtr<xiiGALRenderPass>     m_pRenderPass;
  };

  void SetupProceduralTrianglePass(ProceduralTrianglePassData& data, xiiRenderGraphBuilder& builder)
  {
    data.m_hOffScreenTexture  = builder.ReadTexture("OffScreenTexture", xiiGALResourceStateFlags::RenderTarget);        // Declare that we will read from the offscreen texture in this pass, which will create a dependency on the previous pass that writes to it and ensure proper synchronization. The returned handle references the texture at its current version, so store and use this handle for all future reads/writes.
    data.m_hOffScreenTexture  = builder.WriteTexture(data.m_hOffScreenTexture, xiiGALResourceStateFlags::RenderTarget); // Declare that we will write to the offscreen texture again in this pass, which will bump its version and ensure proper synchronization with the previous pass.
    data.m_hShader            = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/ProceduralTriangle.xiiShader");
    data.m_hShaderPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(data.m_hShader, {}, false);

    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

    xiiGALRenderPassCreationDescription renderPassDescription;
    {
      xiiGALRenderPassAttachmentDescription& colorAttachmentDescription = renderPassDescription.m_Attachments.ExpandAndGetRef();
      colorAttachmentDescription.m_Format                               = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      colorAttachmentDescription.m_uiSampleCount                        = 1U;
      colorAttachmentDescription.m_InitialStateFlags                    = xiiGALResourceStateFlags::RenderTarget;
      colorAttachmentDescription.m_FinalStateFlags                      = xiiGALResourceStateFlags::RenderTarget;
      colorAttachmentDescription.m_LoadOperation                        = xiiGALAttachmentLoadOperation::Load;
      colorAttachmentDescription.m_StoreOperation                       = xiiGALAttachmentStoreOperation::Store;

      xiiGALSubPassDescription& subpassDescription = renderPassDescription.m_SubPasses.ExpandAndGetRef();
      {
        xiiGALAttachmentReferenceDescription& colorAttachmentReferenceDescription = subpassDescription.m_RenderTargetAttachments.ExpandAndGetRef();
        colorAttachmentReferenceDescription.m_ResourceStateFlags                  = xiiGALResourceStateFlags::RenderTarget;
        colorAttachmentReferenceDescription.m_uiAttachmentIndex                   = 0U;
      }

      xiiGALSubPassDependencyDescription& dependencyDesc = renderPassDescription.m_Dependencies.ExpandAndGetRef();
      dependencyDesc.m_uiSourceSubPass                   = XII_GAL_SUBPASS_EXTERNAL;
      dependencyDesc.m_uiDestinationSubPass              = 0U;
      dependencyDesc.m_SourceStageFlags                  = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;
      dependencyDesc.m_DestinationStageFlags             = xiiGALPipelineStageFlags::RenderTarget | xiiGALPipelineStageFlags::EarlyFragmentTests;
      dependencyDesc.m_DestinationAccessFlags            = xiiGALAccessFlags::RenderTargetWrite;
    }
    data.m_pRenderPass = xiiGALRenderPassCache::GetRenderPass(renderPassDescription);

    xiiGALBufferCreationDescription triangleConstantBufferDescription;
    triangleConstantBufferDescription.m_BindFlags           = xiiGALBindFlags::UniformBuffer;
    triangleConstantBufferDescription.m_uiElementByteStride = 0U;
    triangleConstantBufferDescription.m_uiSize              = sizeof(xiiProceduralTriangleConstants);
    triangleConstantBufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
    triangleConstantBufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
    data.m_hTriangleConstantBuffer                          = builder.WriteBuffer("TriangleConstantBuffer", triangleConstantBufferDescription, xiiGALResourceStateFlags::ConstantBuffer);
  }

  void ExecuteProceduralTrianglePass(const ProceduralTrianglePassData& data, xiiRenderGraphPassContext& context)
  {
    xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

    xiiSharedPtr<xiiGALGraphicsPipelineState> pPipelineState;
    {
      xiiResourceLock<xiiShaderPermutationResource> pShaderPermutation(data.m_hShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);

      xiiGALGraphicsPipelineStateCreationDescription graphicsPipelineStateDescription;
      graphicsPipelineStateDescription.m_pPipelineResourceSignature            = pShaderPermutation->GetPipelineResourceSignature();
      graphicsPipelineStateDescription.m_pVertexShader                         = pShaderPermutation->GetGALShader(xiiGALShaderType::Vertex);
      graphicsPipelineStateDescription.m_pPixelShader                          = pShaderPermutation->GetGALShader(xiiGALShaderType::Pixel);
      graphicsPipelineStateDescription.m_GraphicsPipeline.m_pBlendState        = pShaderPermutation->GetBlendState();
      graphicsPipelineStateDescription.m_GraphicsPipeline.m_pRasterizerState   = pShaderPermutation->GetRasterizerState();
      graphicsPipelineStateDescription.m_GraphicsPipeline.m_pDepthStencilState = pShaderPermutation->GetDepthStencilState();
      graphicsPipelineStateDescription.m_GraphicsPipeline.m_pRenderPass        = data.m_pRenderPass;
      graphicsPipelineStateDescription.m_GraphicsPipeline.m_PrimitiveTopology  = xiiGALPrimitiveTopology::TriangleList;

      pPipelineState = xiiGALPipelineCache::GetPipeline(graphicsPipelineStateDescription);
    }

    xiiSharedPtr<xiiGALFramebuffer> pFramebuffer;
    xiiSizeU32                      framebufferSize;
    {
      xiiGALFramebufferCreationDescription framebufferDescription;
      framebufferDescription.m_pRenderPass       = data.m_pRenderPass;
      framebufferDescription.m_FramebufferSize   = m_pWindow->GetClientAreaSize();
      framebufferDescription.m_uiArraySliceCount = 1U;
      framebufferDescription.m_Attachments.PushBack(context.GetTexture(data.m_hOffScreenTexture)->GetDefaultView(xiiGALTextureViewType::RenderTarget));

      pFramebuffer    = pDevice->CreateFramebuffer(framebufferDescription);
      framebufferSize = framebufferDescription.m_FramebufferSize;
    }

    xiiGALCommandList& cmd = context.GetCommandList();

    cmd.BeginDebugGroup("Procedural Triangle");
    {
      {
        // Map the constant buffer and write the data for this frame. The render graph will ensure proper synchronization so that the GPU is not still reading from it when we write to it.
        xiiGALMapHelper<xiiProceduralTriangleConstants> pConstants(cmd, context.GetBuffer(data.m_hTriangleConstantBuffer), xiiGALMapType::Write, xiiGALMapFlags::Discard);

        pConstants->fTime       = (float)xiiMath::Mod(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 1000.0);
        pConstants->vResolution = xiiVec2::Make((float)framebufferSize.width, (float)framebufferSize.height);
        pConstants->fWireWidth  = 1.0f;
      }
      cmd.BeginRenderPass({data.m_pRenderPass.Borrow(), pFramebuffer}); // Begin a render pass on the offscreen framebuffer we created, which will also perform the necessary resource transitions for the offscreen texture and depth buffer.
      {
        cmd.SetViewport(xiiRectFloat(0.0f, 0.0f, (float)framebufferSize.width, (float)framebufferSize.height));                               // Set the viewport to cover the entire render target.
        cmd.SetPipelineState(pPipelineState);                                                                                                 // Set the pipeline state we created in the setup function. This will also bind the shaders and their resources (none in this case).
        cmd.ResolveAndSetConstantBuffer(XII_PP_STRINGIFY(xiiProceduralTriangleConstants), context.GetBuffer(data.m_hTriangleConstantBuffer)); // This will bind the constant buffer to the correct slot based on the shader reflection data, and also ensure proper resource state transitions.
        cmd.CommitShaderResources(xiiGALStateTransitionMode::Verify).IgnoreResult();                                                          // This will bind the offscreen texture as render target, as well as any other resources used by the shader (none in this case).
        cmd.Draw({3});                                                                                                                        // We will draw a single triangle with 3 vertices, generated procedurally in the vertex shader.
      }
      cmd.EndRenderPass(); // End the render pass, which will also perform necessary resource transitions to make the offscreen texture available for reading in the next pass.
    }
    cmd.EndDebugGroup();
  }

  struct BlitPassData
  {
    xiiRenderGraphTextureHandle m_hBackBufferTexture;
    xiiRenderGraphTextureHandle m_hOffScreenTexture;
  };

  void SetupBlitPass(BlitPassData& data, xiiRenderGraphBuilder& builder)
  {
    // Declare that we will read from the offscreen texture created in the previous pass.
    // This registers a read dependency on that pass, so it will be scheduled after it and the texture will be transitioned to the correct state before we read from it.
    data.m_hOffScreenTexture = builder.ReadTexture("OffScreenTexture", xiiGALResourceStateFlags::CopySource);

    // We also need to get the back buffer texture from the swap chain as a render target.
    data.m_hBackBufferTexture = builder.ImportTexture("BackBuffer", m_pSwapChain->GetBackBufferTexture(), xiiGALResourceStateFlags::CopyDestination);

    // This pass writes to the back buffer, so we need to declare that it has side effects to prevent it from being culled.
    builder.SetPassSideEffects(true);
  }

  void ExecuteBlitPass(const BlitPassData& data, xiiRenderGraphPassContext& context)
  {
    xiiGALCommandList& cmd = context.GetCommandList();

    cmd.BeginDebugGroup("Blit to Back Buffer");
    {
      // Just blit the offscreen texture to the back buffer. The render graph will take care of all necessary resource transitions.
      cmd.CopyTexture(context.GetTexture(data.m_hOffScreenTexture), context.GetTexture(data.m_hBackBufferTexture));
    }
    cmd.EndDebugGroup();
  }

private:
  xiiSharedPtr<xiiGALDevice>    m_pDevice;
  xiiSharedPtr<xiiGALSwapChain> m_pSwapChain;

  xiiUniquePtr<xiiRenderGraph>                  m_pRenderGraph;
  xiiUniquePtr<xiiRenderGraphBlackboard>        m_pRenderGraphBlackboard;
  xiiUniquePtr<xiiRenderGraphResourceCache>     m_pRenderGraphResourceCache;
  xiiUniquePtr<xiiRenderGraphTimestampProfiler> m_pRenderGraphProfiler;
  xiiUInt64                                     m_uiFrameIndex = 0ULL;
  xiiUniquePtr<xiiWindow>                       m_pWindow;
};

XII_CONSOLEAPP_ENTRY_POINT(xiiProceduralTriangleApp);
