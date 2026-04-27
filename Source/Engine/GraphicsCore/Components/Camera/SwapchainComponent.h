#pragma once

#include <Core/World/World.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// \brief Present mode for the swapchain.
struct XII_GRAPHICSCORE_DLL xiiPresentMode
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Immediate = 0, ///< No VSync, may tear.
    Fifo,          ///< Standard VSync.
    FifoRelaxed,   ///< VSync unless late — allows tearing.
    Mailbox,       ///< Triple-buffer / "fast" VSync.

    ENUM_COUNT,
    Default = Fifo
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiPresentMode);

/// \brief Render data submitted per-frame by a swapchain component.
class XII_GRAPHICSCORE_DLL xiiSwapchainRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSwapchainRenderData, xiiRenderData);

public:
  xiiEnum<xiiPresentMode> m_PresentMode;
  xiiUInt8                m_uiBufferCount = 2;
  bool                    m_bHDR          = false;
};

using xiiSwapchainComponentManager = xiiComponentManager<class xiiSwapchainComponent, xiiBlockStorageType::Compact>;

/// \brief Controls the platform swapchain present parameters for a camera/window.
class XII_GRAPHICSCORE_DLL xiiSwapchainComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSwapchainComponent, xiiComponent, xiiSwapchainComponentManager);

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  xiiSwapchainComponent();
  ~xiiSwapchainComponent();

  void                    SetPresentMode(xiiEnum<xiiPresentMode> mode);    // [ property ]
  xiiEnum<xiiPresentMode> GetPresentMode() const { return m_PresentMode; } // [ property ]

  void     SetBufferCount(xiiUInt8 uiCount);                  // [ property ]
  xiiUInt8 GetBufferCount() const { return m_uiBufferCount; } // [ property ]

  void SetHDR(bool bHDR);                // [ property ]
  bool GetHDR() const { return m_bHDR; } // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiEnum<xiiPresentMode> m_PresentMode   = xiiPresentMode::Fifo;
  xiiUInt8                m_uiBufferCount = 2;
  bool                    m_bHDR          = false;
};
