#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/BufferView.h>

xiiGALBufferView::xiiGALBufferView(xiiGALResourceBase* pResource, const xiiGALBufferViewCreationDescription& creationDescription) :
  xiiGALResource<xiiGALBufferViewCreationDescription>(creationDescription), m_pResource(pResource)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif

  XII_ASSERT_DEV(m_pResource != nullptr, "The given resource must not be nullptr.");
}

xiiGALBufferView::~xiiGALBufferView() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_BufferView);
