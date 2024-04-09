#pragma once

#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

class xiiGraphicsExplorerWindow;
class xiiGALDevice;

// A simple application that creates a window.
class xiiGraphicsExplorerWindowApp final : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiGraphicsExplorerWindowApp();

  virtual Execution Run() override;

  virtual void AfterCoreSystemsStartup() override;

  virtual void BeforeHighLevelSystemsShutdown() override;

  virtual void BeforeCoreSystemsShutdown() override;

  void UpdateSwapChain();

private:
  xiiGraphicsExplorerWindow* m_pWindow = nullptr;

  xiiGALDevice* m_pDevice = nullptr;

  xiiGALSwapChainHandle m_hSwapChain;
  xiiGALTextureHandle   m_hDepthStencilTexture;

  xiiGALRenderPassHandle  m_hRenderPass;
  xiiGALFramebufferHandle m_hFrameBuffer;
};
