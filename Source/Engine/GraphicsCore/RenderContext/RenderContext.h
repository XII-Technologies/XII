#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/Map.h>

#include <GraphicsCore/Shader/ConstantBufferStorage.h>

//////////////////////////////////////////////////////////////////////////
// xiiRenderContext
//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiRenderContext
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderContext);

  // Static Functions
public:
  // Constant buffer storage handling
  template <typename T>
  XII_ALWAYS_INLINE static xiiConstantBufferStorageHandle CreateConstantBufferStorage()
  {
    return CreateConstantBufferStorage(sizeof(T));
  }

  template <typename T>
  XII_FORCE_INLINE static xiiConstantBufferStorageHandle CreateConstantBufferStorage(xiiConstantBufferStorage<T>*& out_pStorage)
  {
    xiiConstantBufferStorageBase*  pStorage;
    xiiConstantBufferStorageHandle hStorage = CreateConstantBufferStorage(sizeof(T), pStorage);
    out_pStorage                            = static_cast<xiiConstantBufferStorage<T>*>(pStorage);
    return hStorage;
  }

  XII_FORCE_INLINE static xiiConstantBufferStorageHandle CreateConstantBufferStorage(xiiUInt32 uiSizeInBytes)
  {
    xiiConstantBufferStorageBase* pStorage;
    return CreateConstantBufferStorage(uiSizeInBytes, pStorage);
  }

  static xiiConstantBufferStorageHandle CreateConstantBufferStorage(xiiUInt32 uiSizeInBytes, xiiConstantBufferStorageBase*& out_pStorage);
  static void                           DeleteConstantBufferStorage(xiiConstantBufferStorageHandle hStorage);

  template <typename T>
  XII_FORCE_INLINE static bool TryGetConstantBufferStorage(xiiConstantBufferStorageHandle hStorage, xiiConstantBufferStorage<T>*& out_pStorage)
  {
    xiiConstantBufferStorageBase* pStorage = nullptr;
    bool                          bResult  = TryGetConstantBufferStorage(hStorage, pStorage);
    out_pStorage                           = static_cast<xiiConstantBufferStorage<T>*>(pStorage);
    return bResult;
  }

  static bool TryGetConstantBufferStorage(xiiConstantBufferStorageHandle hStorage, xiiConstantBufferStorageBase*& out_pStorage);

  template <typename T>
  XII_FORCE_INLINE static T* GetConstantBufferData(xiiConstantBufferStorageHandle hStorage)
  {
    xiiConstantBufferStorage<T>* pStorage = nullptr;
    if (TryGetConstantBufferStorage(hStorage, pStorage))
    {
      return &(pStorage->GetDataForWriting());
    }

    return nullptr;
  }

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RendererContext);

  static void OnEngineShutdown();

private:
  static xiiMutex                                                              s_ConstantBufferStorageMutex;
  static xiiIdTable<xiiConstantBufferStorageId, xiiConstantBufferStorageBase*> s_ConstantBufferStorageTable;
  static xiiMap<xiiUInt32, xiiDynamicArray<xiiConstantBufferStorageBase*>>     s_FreeConstantBufferStorage;
};
