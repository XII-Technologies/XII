#pragma once

#include <Foundation/Application/Application.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class xiiShaderExplorerWindow;
class xiiCamera;
class xiiGALDevice;
class xiiDirectoryWatcher;

// A simple application that renders a full screen quad with a single shader and does live reloading of that shader
// Can be used for singed distance rendering experiments or other single shader experiments.
class xiiShaderExplorerApp : public xiiApplication
{
public:
  typedef xiiApplication SUPER;

  xiiShaderExplorerApp();

  virtual Execution Run() override;

  virtual void AfterCoreSystemsStartup() override;

  virtual void BeforeHighLevelSystemsShutdown() override;

private:
  void UpdateSwapChain();
  void CreateScreenQuad();
  void OnFileChanged(const char* filename, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type);

  xiiShaderExplorerWindow* m_pWindow = nullptr;
  xiiGALDevice*            m_pDevice = nullptr;

  xiiGALSwapChainHandle m_hSwapChain;
  xiiGALTextureHandle   m_hDepthStencilTexture;

  xiiMaterialResourceHandle   m_hMaterial;
  xiiMeshBufferResourceHandle m_hQuadMeshBuffer;

  xiiUniquePtr<xiiCamera>           m_pCamera;
  xiiUniquePtr<xiiDirectoryWatcher> m_pDirectoryWatcher;

  bool m_bStuffChanged;
};
