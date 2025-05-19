#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>

class XII_GRAPHICSCORE_DLL xiiConstantBufferStorageBase
{
protected:
  friend class xiiRenderContext;
  friend class xiiMemoryUtils;

  xiiConstantBufferStorageBase(xiiUInt32 uiSizeInBytes);
  ~xiiConstantBufferStorageBase();

public:
  xiiArrayPtr<xiiUInt8>       GetRawDataForWriting();
  xiiArrayPtr<const xiiUInt8> GetRawDataForReading() const;

  void UploadData(xiiGALCommandList* pCommandList);

  XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetGALBuffer() const { return m_pGALConstantBuffer; }

protected:
  bool                       m_bHasBeenModified = false;
  xiiUInt32                  m_uiLastHash       = 0;
  xiiSharedPtr<xiiGALBuffer> m_pGALConstantBuffer;

  xiiArrayPtr<xiiUInt8> m_Data;
};

template <typename T>
class xiiConstantBufferStorage : public xiiConstantBufferStorageBase
{
public:
  XII_FORCE_INLINE T& GetDataForWriting()
  {
    xiiArrayPtr<xiiUInt8> pRawData = GetRawDataForWriting();
    XII_ASSERT_DEV(pRawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<T*>(pRawData.GetPtr());
  }

  XII_FORCE_INLINE const T& GetDataForReading() const
  {
    xiiArrayPtr<const xiiUInt8> pRawData = GetRawDataForReading();
    XII_ASSERT_DEV(pRawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<const T*>(pRawData.GetPtr());
  }
};

using xiiConstantBufferStorageId = xiiGenericId<24, 8>;

class xiiConstantBufferStorageHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiConstantBufferStorageHandle, xiiConstantBufferStorageId);

  friend class xiiRenderContext;
};
