#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBufferView, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALBufferView::xiiGALBufferView(xiiSharedPtr<xiiGALDevice> pDevice, xiiSharedPtr<xiiGALBuffer> pBuffer, const xiiGALBufferViewCreationDescription& creationDescription) :
  xiiGALResourceView(pDevice), m_pBuffer(pBuffer), m_Description(creationDescription)
{
  XII_ASSERT_DEV(m_pBuffer != nullptr, "The given buffer must not be nullptr.");
}

xiiGALBufferView::~xiiGALBufferView() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_BufferView);
