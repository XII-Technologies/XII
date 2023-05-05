#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class XII_RENDERERCORE_DLL xiiConstantBufferStorageBase
{
protected:
  friend class xiiRenderContext;
  friend class xiiMemoryUtils;

  xiiConstantBufferStorageBase(xiiUInt32 uiSizeInBytes, const char* szName);
  ~xiiConstantBufferStorageBase();

public:
  xiiArrayPtr<xiiUInt8>       GetRawDataForWriting();
  xiiArrayPtr<const xiiUInt8> GetRawDataForReading() const;

  void UploadData(xiiGALCommandEncoder* pCommandEncoder);

  XII_ALWAYS_INLINE xiiGALBufferHandle GetGALBufferHandle() const { return m_hGALConstantBuffer; }

protected:
  bool               m_bHasBeenModified;
  xiiUInt32          m_uiLastHash;
  xiiGALBufferHandle m_hGALConstantBuffer;

  xiiArrayPtr<xiiUInt8> m_Data;
};

template <typename T>
class xiiConstantBufferStorage : public xiiConstantBufferStorageBase
{
public:
  XII_FORCE_INLINE T& GetDataForWriting()
  {
    xiiArrayPtr<xiiUInt8> rawData = GetRawDataForWriting();
    XII_ASSERT_DEV(rawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<T*>(rawData.GetPtr());
  }

  XII_FORCE_INLINE const T& GetDataForReading() const
  {
    xiiArrayPtr<const xiiUInt8> rawData = GetRawDataForReading();
    XII_ASSERT_DEV(rawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<const T*>(rawData.GetPtr());
  }
};

using xiiConstantBufferStorageId = xiiGenericId<24, 8>;

class xiiConstantBufferStorageHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiConstantBufferStorageHandle, xiiConstantBufferStorageId);

  friend class xiiRenderContext;
};
