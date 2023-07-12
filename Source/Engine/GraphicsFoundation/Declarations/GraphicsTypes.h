#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>

#include <GraphicsFoundation/Declarations/Constants.h>

/// \brief Defines the graphics device.
struct XII_GRAPHICSFOUNDATION_DLL xiiGraphicsDeviceType
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    Undefined = 0, ///< Undefined graphics device type.
    D3D11,         ///< DirectX 11 graphics device.
    D3D12,         ///< DirectX 12 graphics device.
    Vulkan,        ///< Vulkan graphics device.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGraphicsDeviceType);
