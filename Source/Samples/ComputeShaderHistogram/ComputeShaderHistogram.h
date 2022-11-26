#pragma once

#include <Foundation/Basics.h>

#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

class xiiWindow;
class xiiDirectoryWatcher;

/// Uses shader reloading mechanism of the ShaderExplorer sample for quick prototyping.
class xiiComputeShaderHistogramApp : public xiiGameApplication
{
public:
  typedef xiiGameApplication SUPER;

  xiiComputeShaderHistogramApp();
  ~xiiComputeShaderHistogramApp();

  virtual xiiApplication::Execution Run() override;

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeHighLevelSystemsShutdown() override;

private:
  void CreateHistogramQuad();
  void OnFileChanged(const char* filename, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type);

  xiiGALTextureHandle          m_hScreenTexture;
  xiiGALRenderTargetViewHandle m_hScreenRTV;
  xiiGALResourceViewHandle     m_hScreenSRV;

  // Could use buffer, but access and organisation with texture is more straight forward.
  xiiGALTextureHandle             m_hHistogramTexture;
  xiiGALUnorderedAccessViewHandle m_hHistogramUAV;
  xiiGALResourceViewHandle        m_hHistogramSRV;

  xiiGALSwapChainHandle        m_hSwapChain;
  xiiGALRenderTargetViewHandle m_hBackbufferRTV;

  xiiShaderResourceHandle m_hScreenShader;
  xiiShaderResourceHandle m_hHistogramDisplayShader;
  xiiShaderResourceHandle m_hHistogramComputeShader;

  xiiMeshBufferResourceHandle m_hHistogramQuadMeshBuffer;

  xiiUniquePtr<xiiDirectoryWatcher> m_pDirectoryWatcher;

  bool m_bStuffChanged;
};
