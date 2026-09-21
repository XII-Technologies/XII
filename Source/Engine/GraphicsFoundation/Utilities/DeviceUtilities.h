/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>

/// Provides utility functions that are common with interfacing with the GAL device.
class XII_GRAPHICSFOUNDATION_DLL xiiGALDeviceUtilities
{
public:
  /// This returns the graphics adapter vendor type from the given ID.
  [[nodiscard]] static xiiEnum<xiiGALGraphicsAdapterVendor> GetVendorFromID(xiiUInt32 uiID);

  /// Creates a vertex buffer with the given vertex size and vertex count.
  ///
  /// \param pDevice        - The device associated with the buffer.
  /// \param uiVertexSize   - The size of a single vertex in the buffer.
  /// \param uiVertexCount  - The number of vertices in the buffer.
  /// \param pInitialData   - The initial data in bytes, that the buffer should contain after creation.
  /// \param bDataIsMutable - Specifies whether the buffer should be considered immutable in its usage.
  [[nodiscard]] static xiiSharedPtr<xiiGALBuffer> CreateVertexBuffer(xiiGALDevice* pDevice, xiiUInt32 uiVertexSize, xiiUInt32 uiVertexCount, xiiArrayPtr<xiiUInt8> pInitialData = xiiArrayPtr<xiiUInt8>(), bool bDataIsMutable = false);

  enum class IndexType
  {
    None,   ///< Indices are not used, vertices are only used to form primitives.
    UShort, ///< 16 bit indices are used to select which vertices shall form a primitive, thus meshes can only use up to 65535 vertices.
    UInt,   ///< 32 bit indices are used to select which vertices shall form a primitive.
  };

  /// Creates an index buffer with the given index type and index count.
  ///
  /// \param pDevice        - The device associated with the buffer.
  /// \param indexType      - The index buffer type. See xiiGALDeviceUtilities::IndexType for details.
  /// \param uiIndexCount   - The number of indices in the buffer.
  /// \param pInitialData   - The initial data in bytes, that the buffer should contain after creation.
  /// \param bDataIsMutable - Specifies whether the buffer should be considered immutable in its usage.
  [[nodiscard]] static xiiSharedPtr<xiiGALBuffer> CreateIndexBuffer(xiiGALDevice* pDevice, IndexType indexType, xiiUInt32 uiIndexCount, xiiArrayPtr<xiiUInt8> pInitialData = xiiArrayPtr<xiiUInt8>(), bool bDataIsMutable = false);

  /// Creates a constant buffer with the given size.
  ///
  /// \param  pDevice     - The device associated with the buffer.
  /// \param uiBufferSize - The size of the buffer in bytes.
  /// \param sDebugName   - Optional debug name for the buffer.
  [[nodiscard]] static xiiSharedPtr<xiiGALBuffer> CreateConstantBuffer(xiiGALDevice* pDevice, xiiUInt32 uiBufferSize, xiiStringView sDebugName = {});

  /// Creates a staging buffer with the given size.
  ///
  /// \param  pDevice     - The device associated with the buffer.
  /// \param uiBufferSize - The size of the buffer in bytes.
  /// \param sDebugName   - Optional debug name for the buffer.
  [[nodiscard]] static xiiSharedPtr<xiiGALBuffer> CreateStagingBuffer(xiiGALDevice* pDevice, xiiUInt32 uiBufferSize, xiiStringView sDebugName = {});

  /// Creates a render target description with the given parameters.
  ///
  /// \param size          - The size (width and height) of the render target.
  /// \param format        - The render target format. See xiiGALResourceFormat for details.
  /// \param uiSampleCount - The number of samples in the render target. The default is xiiGALSampleCount::OneSample.
  [[nodiscard]] static xiiGALTextureCreationDescription CreateRenderTargetDescription(xiiSizeU32 size, xiiGALResourceFormat::Enum format, xiiUInt32 uiSampleCount = xiiGALSampleCount::OneSample);

  /// Maps a buffer and updates it with the provided source data.
  ///
  /// \param pCommandList        - Pointer to the command list.
  /// \param hBuffer             - Handle to the buffer to be updated.
  /// \param uiDestinationOffset - Offset in the destination buffer where the data should be copied.
  /// \param pSourceData         - Array pointer to the source data to be copied.
  /// \param mapFlags            - Flags specifying the mapping behavior. Default is xiiGALMapFlags::Discard.
  /// \return xiiResult indicating the success or failure of the operation.
  static xiiResult MapAndUpdateBuffer(xiiGALCommandList* pCommandList, xiiSharedPtr<xiiGALBuffer> pBuffer, xiiUInt32 uiDestinationOffset, xiiArrayPtr<const xiiUInt8> pSourceData, xiiBitflags<xiiGALMapFlags> mapFlags = xiiGALMapFlags::Discard);
};
