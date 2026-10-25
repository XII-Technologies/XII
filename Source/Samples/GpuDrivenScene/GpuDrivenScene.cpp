/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include "GpuDrivenSceneWorld.h"

#include <Foundation/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/CommandLineOptions.h>

#include <Core/Graphics/Camera.h>
#include <Core/Input/InputManager.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/System/Window.h>
#include <Foundation/Memory/FrameAllocator.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Device/SwapChain.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/View.h>


#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER)
#  define USE_DIRECTORY_WATCHER XII_ON
#else
#  define USE_DIRECTORY_WATCHER XII_OFF
#endif

xiiCommandLineOptionInt    opt_GpuDrivenMonitor("GpuDrivenScene", "-monitor", "Monitor used by the scene sample.", 0U);
xiiCommandLineOptionInt    opt_GpuDrivenFrameCount("GpuDrivenScene", "-frames", "Quit normally after this many rendered frames (zero runs until closed).", 0U);
xiiCommandLineOptionInt    opt_GpuDrivenGridWidth("GpuDrivenScene", "-grid-width", "Number of scene instances along the X axis.", 24, 1, 512);
xiiCommandLineOptionInt    opt_GpuDrivenGridHeight("GpuDrivenScene", "-grid-height", "Number of scene instances along the Y axis.", 16, 1, 512);
xiiCommandLineOptionString opt_GpuDrivenPipeline("GpuDrivenScene", "-pipeline", "Render pipeline resource used by the view.", "RenderPipelines/Scene.xiiBinRenderPipeline");
namespace
{
  bool g_bWindowResized = false;
}

class xiiGpuDrivenSceneApp final : public xiiApplication
{
public:
  using SUPER = xiiApplication;
  xiiGpuDrivenSceneApp() : xiiApplication("GPU Driven Scene") {}

  Execution Run() override
  {
    const auto frameLimit = opt_GpuDrivenFrameCount.GetOptionValue(xiiCommandLineOption::LogMode::Never);
    if (frameLimit > 0 && m_uiFrameIndex >= static_cast<xiiUInt64>(frameLimit))
    {
      xiiLog::Info("Completed {} Scene renderer frames.", m_uiFrameIndex);
      return Execution::Quit;
    }
    m_pWindow->ProcessWindowMessages();
    if (WasQuitRequested() || xiiInputManager::GetInputActionState("Main", "CloseApp") == xiiKeyState::Pressed)
      return Execution::Quit;
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
    xiiClock::GetGlobalClock()->Update();
    xiiInputManager::Update(xiiClock::GetGlobalClock()->GetTimeDiff());
#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    m_bFileModified = false;
    m_pDirectoryWatcher->EnumerateChanges(xiiMakeDelegate(&xiiGpuDrivenSceneApp::OnFileChanged, this));
    if (m_bFileModified)
      xiiResourceManager::ReloadAllResources(false);
#endif
    m_pDevice->BeginFrame();
    if (m_pSwapChain && m_pSwapChain->GetCurrentSize().HasNonZeroArea() && m_pSwapChain->GetBackBufferTexture())
    {
      auto*      view = m_World.GetRenderView();
      const auto size = m_pSwapChain->GetCurrentSize();
      view->SetViewport({0, 0, float(size.width), float(size.height)});
      m_World.Update(xiiClock::GetGlobalClock()->GetTimeDiff());
      if (view->GetLastRenderResult().Failed() || view->GetRenderPipelineInstance().GetResourceAcquireResult() != xiiResourceAcquireResult::Final)
      {
        xiiLog::Error("The selected Scene pipeline failed to render frame {}.", m_uiFrameIndex);
        SetReturnCode(1);
        m_pDevice->EndFrame();
        return Execution::Quit;
      }
      ++m_uiFrameIndex;
      if (m_uiFrameIndex == 1)
      {
        const auto& statistics = view->GetRenderGraph()->GetStatistics();
        xiiLog::Info("Scene resource: {} operations, {} submissions, {} barriers.", statistics.m_uiCompiledPassCount, statistics.m_uiQueueSubmissionCount, statistics.m_uiTotalBarrierCount);
      }
      m_pSwapChain->Present();
    }
    m_pDevice->EndFrame();
    xiiTelemetry::PerFrameUpdate();
    xiiResourceManager::PerFrameUpdate();
    xiiTaskSystem::FinishFrameTasks();
    xiiFrameAllocator::Swap();
    return Execution::Continue;
  }
  void AfterCoreSystemsStartup() override
  {
    xiiStringBuilder sProjectDirectory = ">sdk/Data/Samples/GpuDrivenScene";

    xiiStringBuilder sResolvedProjectDirectory;
    xiiFileSystem::ResolveSpecialDirectory(sProjectDirectory, sResolvedProjectDirectory).AssertSuccess();
    xiiFileSystem::SetSpecialDirectory("project", sResolvedProjectDirectory);

#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    m_pDirectoryWatcher = XII_DEFAULT_NEW(xiiDirectoryWatcher);
    m_pDirectoryWatcher->OpenDirectory(sResolvedProjectDirectory, xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).AssertSuccess("Failed to watch project directory");
#endif

    xiiFileSystem::AddDataDirectory(">sdk/Output/", "ShaderCache", "shadercache", xiiDataDirUsage::AllowWrites).AssertSuccess();
    xiiFileSystem::AddDataDirectory(">sdk/Data/Base", "Base", "base").AssertSuccess();
    // Compiled pipelines come from the normal generated asset cache, never source control.
    xiiFileSystem::AddDataDirectory(">sdk/Data/Base/AssetCache/Common", "PipelineCache", "pipelinecache").AssertSuccess();
    xiiFileSystem::AddDataDirectory(">project/", "Project", "project").AssertSuccess();

    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    xiiTelemetry::SetServerName("GPU Driven Scene");

    // Activate xiiTelemetry such that the inspector plugin can use the network connection.
    xiiTelemetry::CreateServer();

    // Load the inspector plugin.
    // The plugin contains automatic configuration code (through the xiiStartup system), so it will configure itself properly when the engine is initialized by calling xiiStartup::StartupCore().
    // When you are using xiiApplication, this is done automatically.
    xiiPlugin::LoadPlugin("xiiInspectorPlugin", xiiPluginLoadFlags::PluginIsOptional).IgnoreResult();
#endif

    {
      xiiInputActionConfig closeAction   = xiiInputManager::GetInputActionConfig("Main", "CloseApp");
      closeAction.m_sInputSlotTrigger[0] = xiiInputSlot_KeyEscape;
      xiiInputManager::SetInputActionConfig("Main", "CloseApp", closeAction, true);
    }

    {
      xiiWindowCreationDescription windowDescription;
      windowDescription.m_Resolution       = xiiSizeU32(1440U, 810U);
      windowDescription.m_Title            = GetApplicationName();
      windowDescription.m_bShowMouseCursor = true;
      windowDescription.m_bClipMouseCursor = false;
      windowDescription.m_WindowMode       = xiiWindowMode::WindowResizable;
      windowDescription.m_iMonitor         = opt_GpuDrivenMonitor.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
      windowDescription.AdjustWindowSizeAndPosition().IgnoreResult();

      m_pWindow = XII_DEFAULT_NEW(xiiWindow);
      m_pWindow->Initialize(windowDescription).AssertSuccess();
      m_pWindow->GetWindowEvents().AddEventHandler([this](const xiiWindowEvent& event) -> void {
        if (event.m_Type == xiiWindowEvent::Type::CloseButtonClicked)
        {
          RequestQuit();
        }
        else if (event.m_Type == xiiWindowEvent::Type::SizeChanged)
        {
          g_bWindowResized = true;
        }
      });
    }

    {
      xiiGALDeviceCreationDescription deviceDescription;
      deviceDescription.m_DeviceFeatures.m_ComputeShaders                = xiiGALDeviceFeatureState::Enabled;
      deviceDescription.m_DeviceFeatures.m_MeshShaders                   = xiiGALDeviceFeatureState::Disabled;
      deviceDescription.m_DeviceFeatures.m_BindlessResources             = xiiGALDeviceFeatureState::Enabled;
      deviceDescription.m_DeviceFeatures.m_ShaderResourceRuntimeArray    = xiiGALDeviceFeatureState::Enabled;
      deviceDescription.m_DeviceFeatures.m_TimestampQueries              = xiiGALDeviceFeatureState::Optional;
      deviceDescription.m_DeviceFeatures.m_DurationQueries               = xiiGALDeviceFeatureState::Optional;
      deviceDescription.m_DeviceFeatures.m_TransferQueueTimestampQueries = xiiGALDeviceFeatureState::Optional;
      deviceDescription.m_DeviceFeatures.m_RayTracing                    = xiiGALDeviceFeatureState::Disabled;
      deviceDescription.m_DeviceFeatures.m_TextureUAVExtendedFormats     = xiiGALDeviceFeatureState::Optional;
      deviceDescription.m_DeviceFeatures.m_NativeFence                   = xiiGALDeviceFeatureState::Optional; //< Timeline fences are required for GPU-side synchronization between the graphics, asynchronous-compute, and transfer queues.

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      deviceDescription.m_ValidationLevel = xiiGALDeviceValidationLevel::Standard;
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

      m_pDevice = xiiGALDeviceFactory::CreateDevice(sGraphicsAPIName, xiiFoundation::GetDefaultAllocator(), deviceDescription);
      XII_ASSERT_DEV(m_pDevice != nullptr, "Device implementation for '{}' not found", sGraphicsAPIName);
      XII_VERIFY(m_pDevice->Initialize() == XII_SUCCESS, "Device initialization failed!");

      m_pDevice->SetDebugName("Master Graphics Device");

      xiiGALDevice::SetDefaultDevice(m_pDevice);
    }

    UpdateSwapChain();

    m_Configuration                = {};
    m_Configuration.m_uiGridWidth  = static_cast<xiiUInt32>(opt_GpuDrivenGridWidth.GetOptionValue(xiiCommandLineOption::LogMode::Always));
    m_Configuration.m_uiGridHeight = static_cast<xiiUInt32>(opt_GpuDrivenGridHeight.GetOptionValue(xiiCommandLineOption::LogMode::Always));
    xiiGpuDrivenSceneWorld::ConfigureSubsystems(m_Configuration).AssertSuccess();

    xiiStartup::StartupHighLevelSystems();

    m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 250.0f);
    m_Camera.LookAt(xiiVec3(-12.0f, -2.0f, 12.0f), xiiVec3(25.0f, 0.0f, 0.0f), xiiVec3(0.0f, 0.0f, 1.0f));

    m_World.Initialize(m_pDevice.Borrow(), m_Configuration).AssertSuccess();
    auto* view = m_World.GetRenderView();
    view->SetCamera(&m_Camera);
    view->SetSwapChain(m_pSwapChain.Borrow());
    const auto                                 pipeline = xiiResourceManager::LoadResource<xiiRenderPipelineResource>(opt_GpuDrivenPipeline.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
    xiiResourceLock<xiiRenderPipelineResource> loaded(pipeline, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (!loaded || !loaded->GetPipeline())
    {
      xiiLog::Error("The selected pipeline resource is unavailable or invalid.");
      SetReturnCode(1);
      RequestQuit();
      return;
    }
    view->SetRenderPipelineResource(pipeline);
  }

  void BeforeHighLevelSystemsShutdown() override
  {
#if XII_ENABLED(USE_DIRECTORY_WATCHER)
    m_pDirectoryWatcher.Clear();
#endif
    if (m_pDevice)
      m_pDevice->WaitIdle();
    xiiFrameAllocator::Reset();
    m_World.Shutdown();
    m_pSwapChain.Clear();
    xiiStartup::ShutdownHighLevelSystems();
    xiiGALDevice::SetDefaultDevice(nullptr);
    m_pDevice.Clear();
    if (m_pWindow)
      m_pWindow->Destroy().IgnoreResult();
    m_pWindow.Clear();
  }
  void BeforeCoreSystemsShutdown() override
  {
    xiiPlugin::UnloadAllPlugins();
    SUPER::BeforeCoreSystemsShutdown();
  }

private:
  void UpdateSwapChain()
  {
    if (!m_pSwapChain)
    {
      xiiGALSwapChainCreationDescription description;
      description.m_pWindow           = m_pWindow.Borrow();
      description.m_ColorBufferFormat = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      description.m_UsageFlags        = xiiGALSwapChainUsageFlags::RenderTarget;
      description.m_uiBufferCount     = 3U;
      m_pSwapChain                    = m_pDevice->CreateSwapChain(description);
      m_pSwapChain->SetPresentMode(xiiGALPresentMode::VSync);
    }
    else if (m_pSwapChain->GetCurrentSize() != m_pWindow->GetClientAreaSize())
    {
      // Resize destroys the old swapchain. Waiting here guarantees presentation has released all
      // acquired images and satisfies VUID-vkDestroySwapchainKHR-swapchain-01282.
      m_pDevice->WaitIdle();
      m_pSwapChain->Resize(m_pWindow->GetClientAreaSize()).AssertSuccess();
      if (auto* view = m_World.GetRenderView())
        view->InvalidateTemporalHistory();
    }
  }

#if XII_ENABLED(USE_DIRECTORY_WATCHER)
  void OnFileChanged(xiiStringView sFileName, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type)
  {
    if (action == xiiDirectoryWatcherAction::Modified && type == xiiDirectoryWatcherType::File)
    {
      xiiLog::Info("File modified: '{0}'.", sFileName);

      m_bFileModified = true;
    }
  }
#endif

  xiiSharedPtr<xiiGALDevice>     m_pDevice;
  xiiSharedPtr<xiiGALSwapChain>  m_pSwapChain;
  xiiUniquePtr<xiiWindow>        m_pWindow;
  xiiGpuDrivenSceneConfiguration m_Configuration;
  xiiGpuDrivenSceneWorld         m_World;
  xiiCamera                      m_Camera;
  xiiUInt64                      m_uiFrameIndex = 0;
#if XII_ENABLED(USE_DIRECTORY_WATCHER)
  xiiUniquePtr<xiiDirectoryWatcher> m_pDirectoryWatcher;
  bool                              m_bFileModified = false;
#endif
};
XII_CONSOLEAPP_ENTRY_POINT(xiiGpuDrivenSceneApp);
