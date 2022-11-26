#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/RenderTargetView.h>


xiiGALRenderTargetView::xiiGALRenderTargetView(xiiGALTexture* pTexture, const xiiGALRenderTargetViewCreationDescription& description) :
  xiiGALObject(description), m_pTexture(pTexture)
{
  XII_ASSERT_DEV(m_pTexture != nullptr, "Texture must not be null");
}

xiiGALRenderTargetView::~xiiGALRenderTargetView() {}



XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_RenderTargetView);
