#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/UnorderedAccesView.h>

xiiGALUnorderedAccessView::xiiGALUnorderedAccessView(xiiGALResourceBase* pResource, const xiiGALUnorderedAccessViewCreationDescription& description) :
  xiiGALObject(description), m_pResource(pResource)
{
  XII_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

xiiGALUnorderedAccessView::~xiiGALUnorderedAccessView() = default;

XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_UnorderedAccessView);
