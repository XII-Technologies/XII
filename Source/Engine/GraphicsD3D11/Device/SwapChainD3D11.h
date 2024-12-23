#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Device/SwapChain.h>

struct IDXGISwapChain4;

struct XII_GRAPHICSD3D11_DLL xiiGALSwapChainD3D11EventType
{
  using StorageType = xiiInt8;

  enum Enum : StorageType
  {
    Unknown = -1,        ///< Unknown swapchain event.
    BeforeBufferRelease, ///< Before swapchain buffer release.

    ENUM_COUNT,

    Default = Unknown
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSD3D11_DLL, xiiGALSwapChainD3D11EventType);

/// \brief This describes the graphics abstraction layer device events.
struct XII_GRAPHICSD3D11_DLL xiiGALSwapChainD3D11Event : public xiiHashableStruct<xiiGALSwapChainD3D11Event>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiGALSwapChainD3D11EventType> m_Type            = xiiGALSwapChainD3D11EventType::Unknown;
  xiiGALSwapChainD3D11*                  m_pSwapChainD3D11 = nullptr;
};

class XII_GRAPHICSD3D11_DLL xiiGALSwapChainD3D11 final : public xiiGALSwapChain
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALSwapChainD3D11, xiiGALSwapChain);

public:
  /// \brief Registers event handlers.
  static xiiEvent<const xiiGALSwapChainD3D11Event&, xiiNoMutex, xiiStaticAllocatorWrapper> s_Events;

  virtual void Present() override final;

  virtual xiiResult Resize(xiiSizeU32 newSize, xiiEnum<xiiGALSurfaceTransform> newTransform = xiiGALSurfaceTransform::Optimal) override final;

  virtual void SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode) override final;

  virtual void SetWindowedMode() override final;

  virtual void SetMaximumFrameLatency(xiiUInt32 uiMaxLatency) override final;

  void WaitForFrame();

  XII_ALWAYS_INLINE IDXGISwapChain4* GetSwapChain() const { return m_pSwapChain; };

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALSwapChainD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALSwapChainCreationDescription& creationDescription);

  virtual ~xiiGALSwapChainD3D11();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

  xiiResult CreateDXGISwapChain();

  xiiResult UpdateSwapChain(bool bCreateNew);

  xiiResult CreateBackBufferInternal(xiiGALDeviceD3D11* pDeviceD3D11);

  void DestroyBackBufferInternal(xiiGALDeviceD3D11* pDeviceD3D11);

protected:
  IDXGISwapChain4* m_pSwapChain = nullptr;

  xiiGALTextureHandle m_hActualBackBufferTexture;

  xiiGALFullScreenModeDescription m_FullScreenMode;

  HANDLE m_FrameLatencyWaitableObject = NULL;

  xiiUInt32 m_uiMaximumFrameLatency = 0U;
};
