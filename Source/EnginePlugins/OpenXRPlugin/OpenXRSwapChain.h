#pragma once

#include <GameEngine/XR/XRSwapChain.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

class xiiOpenXR;

XII_DEFINE_AS_POD_TYPE(XrSwapchainImageD3D11KHR);

class XII_OPENXRPLUGIN_DLL xiiGALOpenXRSwapChain : public xiiGALXRSwapChain
{
public:
  xiiSizeU32  GetRenderTargetSize() const { return m_CurrentSize; }
  XrSwapchain GetColorSwapchain() const { return m_colorSwapchain.handle; }
  XrSwapchain GetDepthSwapchain() const { return m_depthSwapchain.handle; }

  virtual void AcquireNextRenderTarget(xiiGALDevice* pDevice) override;
  virtual void PresentRenderTarget(xiiGALDevice* pDevice) override;
  void         PresentRenderTarget() const;

protected:
  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;
  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

private:
  friend class xiiOpenXR;
  struct Swapchain
  {
    XrSwapchain                 handle     = 0;
    int64_t                     format     = 0;
    xiiUInt32                   imageCount = 0;
    XrSwapchainImageBaseHeader* images     = nullptr;
    uint32_t                    imageIndex = 0;
  };
  enum class SwapchainType
  {
    Color,
    Depth,
  };

private:
  xiiGALOpenXRSwapChain(xiiOpenXR* pXrInterface, xiiGALMSAASampleCount::Enum msaaCount);
  XrResult SelectSwapchainFormat(int64_t& colorFormat, int64_t& depthFormat);
  XrResult CreateSwapchainImages(Swapchain& swapchain, SwapchainType type);
  XrResult InitSwapChain(xiiGALMSAASampleCount::Enum msaaCount);
  void     DeinitSwapChain();

private:
  XrInstance                     m_instance = XR_NULL_HANDLE;
  uint64_t                       m_systemId = XR_NULL_SYSTEM_ID;
  XrSession                      m_session  = XR_NULL_HANDLE;
  xiiEnum<xiiGALMSAASampleCount> m_msaaCount;

  // Swapchain
  XrViewConfigurationView m_primaryConfigView;
  Swapchain               m_colorSwapchain;
  Swapchain               m_depthSwapchain;

  xiiHybridArray<XrSwapchainImageD3D11KHR, 3> m_colorSwapChainImagesD3D11;
  xiiHybridArray<XrSwapchainImageD3D11KHR, 3> m_depthSwapChainImagesD3D11;
  xiiHybridArray<xiiGALTextureHandle, 3>      m_hColorRTs;
  xiiHybridArray<xiiGALTextureHandle, 3>      m_hDepthRTs;

  bool                m_bImageAcquired = false;
  xiiGALTextureHandle m_hColorRT;
  xiiGALTextureHandle m_hDepthRT;
};
