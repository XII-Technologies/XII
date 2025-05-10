#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>

/// \brief This describes the dynamic buffer creation description.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALDynamicBufferCreationDescription : public xiiHashableStruct<xiiGALDynamicBufferCreationDescription>
{
  xiiGALBufferCreationDescription m_BufferDescription;                ///< The buffer description.
  xiiUInt32                       m_uiMemoryPageSize = 64U << 10U;    ///< For a sparse buffer, the size of the memory page, ignored otherwise.
  xiiUInt64                       m_uiVirtualSize    = 1ULL << 30ULL; ///< For a sparse buffer, the virtual size, ignored otherwise.
};

class XII_GRAPHICSFOUNDATION_DLL xiiGALDynamicBuffer : xiiGALObject
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALDynamicBuffer);

  XII_ADD_DYNAMIC_REFLECTION(xiiGALDynamicBuffer, xiiGALObject);

public:
  xiiGALDynamicBuffer(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALDynamicBufferCreationDescription& description);
  ~xiiGALDynamicBuffer();

  [[nodiscard]] XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> GetBuffer() const;


  xiiSharedPtr<xiiGALBuffer> Resize(xiiSharedPtr<xiiGALCommandList> pCommandList, xiiUInt64 uiNewSize, bool bDiscardContent = false);
  xiiSharedPtr<xiiGALBuffer> Update(xiiSharedPtr<xiiGALCommandList> pCommandList);

  private:
  xiiGALBufferCreationDescription m_BufferDescription;
};
