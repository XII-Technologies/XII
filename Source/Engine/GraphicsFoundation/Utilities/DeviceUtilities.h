#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/GraphicsTypes.h>
#include <GraphicsFoundation/Device/Device.h>

/// \brief Provides utility functions that are common with interfacing with the GAL device.
class XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceUtilities
{
public:
  /// \brief This returns the graphics adapter vendor type from the given ID.
  static [[nodiscard]] xiiEnum<xiiGALGraphicsAdapterVendor> GetVendorFromID(xiiUInt32 uiID);

  /// \brief Creates a vertex buffer with the given vertex size and vertex count.
  ///
  /// \param pDevice        - The device associated with the buffer.
  /// \param uiVertexSize   - The size of a single vertex in the buffer.
  /// \param uiVertexCount  - The number of vertices in the buffer.
  /// \param pInitialData   - The initial data in bytes, that the buffer should contain after creation.
  /// \param bDataIsMutable - Specifies whether the buffer should be considered immutable in its usage.
  static [[nodiscard]] xiiGALBufferHandle CreateVertexBuffer(xiiGALDevice* pDevice, xiiUInt32 uiVertexSize, xiiUInt32 uiVertexCount, xiiArrayPtr<xiiUInt8> pInitialData = xiiArrayPtr<xiiUInt8>(), bool bDataIsMutable = false);

  enum class IndexType
  {
    None,   ///< Indices are not used, vertices are only used to form primitives.
    UShort, ///< 16 bit indices are used to select which vertices shall form a primitive, thus meshes can only use up to 65535 vertices.
    UInt,   ///< 32 bit indices are used to select which vertices shall form a primitive.
  };

  /// \brief Creates an index buffer with the given index type and index count.
  ///
  /// \param pDevice        - The device associated with the buffer.
  /// \param indexType      - The index buffer type. See xiiGALDeviceUtilities::IndexType for details.
  /// \param uiIndexCount   - The number of indices in the buffer.
  /// \param pInitialData   - The initial data in bytes, that the buffer should contain after creation.
  /// \param bDataIsMutable - Specifies whether the buffer should be considered immutable in its usage.
  static [[nodiscard]] xiiGALBufferHandle CreateIndexBuffer(xiiGALDevice* pDevice, IndexType indexType, xiiUInt32 uiIndexCount, xiiArrayPtr<xiiUInt8> pInitialData = xiiArrayPtr<xiiUInt8>(), bool bDataIsMutable = false);

  /// \brief Creates a constant buffer with the given size.
  ///
  /// \param  pDevice     - The device associated with the buffer.
  /// \param uiBufferSize - The size of the buffer in bytes.
  static [[nodiscard]] xiiGALBufferHandle CreateConstantBuffer(xiiGALDevice* pDevice, xiiUInt32 uiBufferSize);

  /// \brief Creates a render target description with the given paramters.
  ///
  /// \param size          - The size (width and height) of the render target.
  /// \param format        - The render target format. See xiiGALTextureFormat for details.
  /// \param uiSampleCount - The number of samples in the render target. The default is xiiGALMSAASampleCount::OneSample.
  static [[nodiscard]] xiiGALTextureCreationDescription CreateRenderTargetDescription(xiiSizeU32 size, xiiGALTextureFormat::Enum format, xiiUInt32 uiSampleCount = xiiGALMSAASampleCount::OneSample);
};
