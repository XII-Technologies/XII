/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <DearImguiPlugin/DearImguiPluginPCH.h>

#include <DearImguiPlugin/Implementation/DearImguiContextData.h>
#include <GraphicsFoundation/Device/Device.h>

xiiImguiFrame::xiiImguiFrame()  = default;
xiiImguiFrame::~xiiImguiFrame() = default;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiImguiFrameData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiImguiFrameData::xiiImguiFrameData(xiiUInt64 uiSetupSerial, xiiSharedPtr<const xiiImguiFrame> pFrame) :
  xiiRenderPipelineFrameData(uiSetupSerial), m_pFrame(std::move(pFrame))
{
}

xiiSharedPtr<xiiGALTexture> xiiImguiTexture::GetTexture()
{
  XII_LOCK(m_Mutex);
  if (m_pTexture)
    return m_pTexture;
  if (m_Pixels.IsEmpty())
    return {};
  xiiGALTextureCreationDescription description;
  description.m_Type      = xiiGALResourceDimension::Texture2D;
  description.m_Size      = m_Size;
  description.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  description.m_Usage     = xiiGALResourceUsage::Immutable;
  description.m_BindFlags = xiiGALBindFlags::ShaderResource;
  xiiGALTextureSubResourceData pixels;
  pixels.m_pData         = xiiMakeByteBlobPtr(m_Pixels.GetData(), m_Pixels.GetCount());
  pixels.m_uiStride      = m_Size.width * 4;
  pixels.m_uiDepthStride = m_Pixels.GetCount();
  const xiiGALTextureData initial(xiiMakeArrayPtr(&pixels, 1));
  m_pTexture = xiiGALDevice::GetDefaultDevice()->CreateTexture(description, &initial);
  return m_pTexture;
}

XII_STATICLINK_FILE(DearImguiPlugin, DearImguiPlugin_Implementation_DearImguiRenderer);
