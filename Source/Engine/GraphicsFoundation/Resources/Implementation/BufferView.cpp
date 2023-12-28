#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/BufferView.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBufferView, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALBufferView::xiiGALBufferView(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription) :
  xiiGALResourceView(), m_Description(creationDescription), m_pBuffer(pBuffer)
{
  XII_ASSERT_DEV(m_pBuffer != nullptr, "The given buffer must not be nullptr.");
}

xiiGALBufferView::~xiiGALBufferView() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_BufferView);
