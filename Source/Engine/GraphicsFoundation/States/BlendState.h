#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief This describes the blend factor.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBlendFactor
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Undefined = 0U,          ///< Undefined blend factor.
    Zero,                    ///< The blend factor is zero.
    One,                     ///< The blend factor is one.
    SourceColor,             ///< The blend factor is RGB data from a pixel shader.
    InverseSourceColor,      ///< The blend factor is 1-RGB, where RGB is the data from a pixel shader.
    SourceAlpha,             ///< The blend factor is alpha (A) data from a pixel shader.
    InverseSourceAlpha,      ///< The blend factor is 1-A, where A is alpha data from a pixel shader.
    DestinationAlpha,        ///< The blend factor is alpha (A) data from a render target.
    InverseDestinationAlpha, ///< The blend factor is 1-A, where A is alpha data from a render target.
    DestinationColor,        ///< The blend factor is RGB data from a render target.
    InverseDestinationColor, ///< The blend factor is 1-RGB, where RGB is the data from a render target.
    SourceAlphaSaturate,     ///< The blend factor is (f,f,f,1), where f = min(As, 1-Ad), As is alpha data from a pixel shader, and Ad is alpha data from a render target.
    BlendFactor,             ///< The blend factor is the constant blend factor set in the device.
    InverseBlendFactor,      ///< The blend factor is one minus constant blend factor set in the device.
    SourceOneColor,          ///< The blend factor is the second RGB data output from a pixel shader.
    InverseSourceOneColor,   ///< The blend factor is 1-RGB, where RGB is the second RGB data output from a pixel shader.
    SourceOneAlpha,          ///< The blend factor is the second alpha (A) data output from a pixel shader.
    InverseSourceOneAlpha,   ///< The blend factor is 1-A, where A is the second alpha data output from a pixel shader.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALBlendFactor);

/// \brief This describes the blend operation.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBlendOperation
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Undefined = 0U,  ///< Undefined blend operation.
    Add,             ///< Add source and destination color components.
    Subtract,        ///< Subtract destination color components from source color components.
    ReverseSubtract, ///< Subtract source color components from destination color components.
    Min,             ///< Compute the minimum of source and destination color components.
    Max,             ///< Compute the maximum of source and destination color components.

    ENUM_COUNT,

    Default = Undefined
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALBlendOperation);

/// \brief This describes the color component write flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALColorMask
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    None  = 0U,         ///< Do not write to any components.
    Red   = XII_BIT(0), ///< Write to the red component.
    Green = XII_BIT(1), ///< Write to the green component.
    Blue  = XII_BIT(2), ///< Write to the blue component.
    Alpha = XII_BIT(3), ///< Write to the alpha component.

    ENUM_COUNT = 5U,

    RG   = Red | Green,                ///< Write to the red and green components.
    RGB  = Red | Green | Blue,         ///< Write to the red, green and blue components.
    RGBA = Red | Green | Blue | Alpha, ///< Write to the red, green, blue, and alpha components.

    Default = (((Red | Green) | Blue) | Alpha) ///< Write to all components.
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALColorMask);

/// \brief This describes the blend state for a single render target.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALRenderTargetBlendDescription : public xiiHashableStruct<xiiGALRenderTargetBlendDescription>
{
  XII_DECLARE_POD_TYPE();

  bool                          m_bBlendEnable = false;  ///< Enable or disable blending for this render target.
  xiiEnum<xiiGALBlendFactor>    m_SourceBlend;           ///< Specifies the blend factor to apply to the RGB value output from the pixel shader.
  xiiEnum<xiiGALBlendFactor>    m_DestinationBlend;      ///< Specifies the blend factor to apply to the RGB value in the render target.
  xiiEnum<xiiGALBlendOperation> m_BlendOperation;        ///< Defines how to combine the source and destination RGB values after applying the source and destination blend factors.
  xiiEnum<xiiGALBlendFactor>    m_SourceBlendAlpha;      ///< Specifies the blend factor to apply to the alpha value output from the pixel shader.
  xiiEnum<xiiGALBlendFactor>    m_DestinationBlendAlpha; ///< Specifies the blend factor to apply to the alpha value in the render target.
  xiiEnum<xiiGALBlendOperation> m_BlendOperationAlpha;   ///< Defines how to combine the source and destination alpha values after applying the source and destination blend alpha factors.
  xiiEnum<xiiGALColorMask>      m_ColorMask;             ///< Render target color write mask.
};

/// \brief This describes the blend state for all render targets in the graphics pipeline.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBlendStateCreationDescription : public xiiHashableStruct<xiiGALBlendStateCreationDescription>
{
  XII_DECLARE_POD_TYPE();

  bool                               m_bAlphaToCoverage  = false;                     ///< Specifies whether to use alpha-to-coverage as a multisampling technique when setting a pixel to a render target.
  bool                               m_bIndependentBlend = false;                     ///< Specifies whether to enable independent blending in simultaneous render targets. If set to false, only m_RenderTargets[0] is used.
  xiiGALRenderTargetBlendDescription m_RenderTargets[XII_GAL_MAX_RENDERTARGET_COUNT]; ///< An array of RenderTargetBlendDesc structures that describe the blend states for render targets.
};

#include <GraphicsFoundation/States/Implementation/BlendState_inl.h>
