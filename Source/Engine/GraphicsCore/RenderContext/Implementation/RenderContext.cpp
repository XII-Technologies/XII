#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>

#include <GraphicsFoundation/ShaderCompiler/ShaderStageBinary.h>

#include <GraphicsCore/RenderContext/RenderContext.h>

xiiMutex                                                              xiiRenderContext::s_ConstantBufferStorageMutex;
xiiIdTable<xiiConstantBufferStorageId, xiiConstantBufferStorageBase*> xiiRenderContext::s_ConstantBufferStorageTable;
xiiMap<xiiUInt32, xiiDynamicArray<xiiConstantBufferStorageBase*>>     xiiRenderContext::s_FreeConstantBufferStorage;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, RendererContext)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiRenderContext::OnEngineShutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// static
xiiConstantBufferStorageHandle xiiRenderContext::CreateConstantBufferStorage(xiiUInt32 uiSizeInBytes, xiiConstantBufferStorageBase*& out_pStorage)
{
  XII_ASSERT_DEV(xiiMemoryUtils::IsSizeAligned(uiSizeInBytes, 16U), "Storage struct for constant buffer is not aligned to 16 bytes.");

  XII_LOCK(s_ConstantBufferStorageMutex);

  xiiConstantBufferStorageBase* pStorage = nullptr;

  auto it = s_FreeConstantBufferStorage.Find(uiSizeInBytes);
  if (it.IsValid())
  {
    xiiDynamicArray<xiiConstantBufferStorageBase*>& storageForSize = it.Value();
    if (!storageForSize.IsEmpty())
    {
      pStorage = storageForSize[0];
      storageForSize.RemoveAtAndSwap(0);
    }
  }

  if (pStorage == nullptr)
  {
    pStorage = XII_DEFAULT_NEW(xiiConstantBufferStorageBase, uiSizeInBytes);
  }

  out_pStorage = pStorage;
  return xiiConstantBufferStorageHandle(s_ConstantBufferStorageTable.Insert(pStorage));
}

// static
void xiiRenderContext::DeleteConstantBufferStorage(xiiConstantBufferStorageHandle hStorage)
{
  XII_LOCK(s_ConstantBufferStorageMutex);

  xiiConstantBufferStorageBase* pStorage = nullptr;
  if (!s_ConstantBufferStorageTable.Remove(hStorage.m_InternalId, &pStorage))
  {
    // already deleted
    return;
  }

  xiiUInt32 uiSizeInBytes = pStorage->m_Data.GetCount();

  auto it = s_FreeConstantBufferStorage.Find(uiSizeInBytes);
  if (!it.IsValid())
  {
    it = s_FreeConstantBufferStorage.Insert(uiSizeInBytes, xiiDynamicArray<xiiConstantBufferStorageBase*>());
  }

  it.Value().PushBack(pStorage);
}

// static
bool xiiRenderContext::TryGetConstantBufferStorage(xiiConstantBufferStorageHandle hStorage, xiiConstantBufferStorageBase*& out_pStorage)
{
  XII_LOCK(s_ConstantBufferStorageMutex);
  return s_ConstantBufferStorageTable.TryGetValue(hStorage.m_InternalId, out_pStorage);
}

// private functions
//////////////////////////////////////////////////////////////////////////

// static
void xiiRenderContext::OnEngineShutdown()
{
  xiiGALShaderStageBinary::OnEngineShutdown();

  // Cleanup constant buffer storage
  {
    for (auto it = s_ConstantBufferStorageTable.GetIterator(); it.IsValid(); ++it)
    {
      xiiConstantBufferStorageBase* pStorage = it.Value();
      XII_DEFAULT_DELETE(pStorage);
    }

    s_ConstantBufferStorageTable.Clear();

    for (auto it = s_FreeConstantBufferStorage.GetIterator(); it.IsValid(); ++it)
    {
      xiiDynamicArray<xiiConstantBufferStorageBase*>& storageForSize = it.Value();
      for (auto& pStorage : storageForSize)
      {
        XII_DEFAULT_DELETE(pStorage);
      }
    }

    s_FreeConstantBufferStorage.Clear();
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_RenderContext_Implementation_RenderContext);
