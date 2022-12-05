#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>

namespace xiiDiligentUtils
{
  XII_ALWAYS_INLINE xiiEnum<xiiGALMSAASampleCount> ToGALMSAASampleCount(xiiUInt32 uiSampleCount)
  {
    xiiEnum<xiiGALMSAASampleCount> result;
    switch (uiSampleCount)
    {
      case 1:
        result = xiiGALMSAASampleCount::None;
        break;
      case 2:
        result = xiiGALMSAASampleCount::TwoSamples;
        break;
      case 4:
        result = xiiGALMSAASampleCount::FourSamples;
        break;
      case 8:
        result = xiiGALMSAASampleCount::EightSamples;
        break;

        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return result;
  }

  XII_ALWAYS_INLINE xiiUInt32 ToDiligentMSAACount(xiiEnum<xiiGALMSAASampleCount> sampleCount)
  {
    return static_cast<xiiUInt32>(sampleCount.GetValue());
  }

  XII_ALWAYS_INLINE Diligent::RENDER_DEVICE_TYPE GetDiligentRenderDeviceType()
  {
    switch (xiiGraphicsDevice::Default)
    {
      case xiiGraphicsDevice::Undefined:
        return Diligent::RENDER_DEVICE_TYPE_UNDEFINED;

      case xiiGraphicsDevice::D3D11:
        return Diligent::RENDER_DEVICE_TYPE_D3D11;

      case xiiGraphicsDevice::D3D12:
        return Diligent::RENDER_DEVICE_TYPE_D3D12;

      case xiiGraphicsDevice::OpenGL:
        return Diligent::RENDER_DEVICE_TYPE_GL;

      case xiiGraphicsDevice::Vulkan:
        return Diligent::RENDER_DEVICE_TYPE_VULKAN;

      case xiiGraphicsDevice::Metal:
        return Diligent::RENDER_DEVICE_TYPE_METAL;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
        return Diligent::RENDER_DEVICE_TYPE_UNDEFINED;
    }
  }
} // namespace xiiDiligentUtils
