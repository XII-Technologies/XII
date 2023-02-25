#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ResourceView.h>


xiiGALResourceView::xiiGALResourceView(xiiGALResourceBase* pResource, const xiiGALResourceViewCreationDescription& description) :
  xiiGALObject(description), m_pResource(pResource)
{
  XII_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

xiiGALResourceView::~xiiGALResourceView() {}


XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_ResourceView);
