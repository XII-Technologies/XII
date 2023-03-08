#pragma once

#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>

#include <Foundation/Application/Application.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

class HelloDiligentWindow;
class xiiCamera;
class xiiGALDevice;
class xiiDirectoryWatcher;

class HelloDiligent : public xiiApplication
{
public:
  typedef xiiApplication SUPER;

  HelloDiligent();

  virtual Execution Run() override;

  virtual void AfterCoreSystemsStartup() override;

  virtual void BeforeHighLevelSystemsShutdown() override;

  virtual void BeforeCoreSystemsShutdown() override;

private:
  void UpdateSwapChain();
  void CreateScreenQuad();
  void OnFileChanged(const char* filename, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type);

  HelloDiligentWindow* m_pWindow = nullptr;

  xiiGALDevice* m_pDevice = nullptr;

  xiiGALSwapChainHandle m_hSwapChain;
  xiiGALTextureHandle   m_hDepthStencilTexture;

  xiiMaterialResourceHandle   m_hMaterial;
  xiiMeshBufferResourceHandle m_hQuadMeshBuffer;

  xiiUniquePtr<xiiCamera>           m_pCamera;
  xiiUniquePtr<xiiDirectoryWatcher> m_pDirectoryWatcher;

  bool m_bDirectoryModified = false;
};
