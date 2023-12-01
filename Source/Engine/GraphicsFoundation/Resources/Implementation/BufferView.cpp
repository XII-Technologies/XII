#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/BufferView.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALBufferView, xiiNoBase, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;

// clang-format on

xiiGALBufferView::xiiGALBufferView(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription) :
  xiiGALResource<xiiGALBufferViewCreationDescription>(creationDescription), m_pBuffer(pBuffer)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif

  XII_ASSERT_DEV(m_pBuffer != nullptr, "The given buffer must not be nullptr.");
}

xiiGALBufferView::~xiiGALBufferView() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_BufferView);
