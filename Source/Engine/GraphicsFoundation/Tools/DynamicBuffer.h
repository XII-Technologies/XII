/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Fence.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALDynamicBuffer : public xiiGALObject
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALDynamicBuffer);

  XII_ADD_DYNAMIC_REFLECTION(xiiGALDynamicBuffer, xiiGALObject);

public:
  xiiGALDynamicBuffer(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALBufferCreationDescription& description);
  ~xiiGALDynamicBuffer();

  /// Returns the buffer description.
  [[nodiscard]] xiiGALBufferCreationDescription GetDescription() const;

  /// Returns a reference-counted pointer to the buffer object.
  ///
  /// \remarks If the buffer has not been initialized, the method returns null.
  ///          If the buffer may need to be updated (resized or initialized), use the Update() method.
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetBuffer() const;

  /// Returns the dynamic buffer version. The version is incremented whenever a new internal buffer is created.
  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32 GetVersion() const { return m_Version; }

  /// Returns true if the buffer must be updated before use (e.g., it has been resized, but the internal buffer has not been initialized or updated). When update is not pending, Update() may be called with null command list.
  [[nodiscard]] bool PendingUpdate() const;

  /// Resizes the buffer to the new size.
  ///
  /// \param pCommandList    - The command list that will be used to copy existing contents to the new buffer. This parameter may be null (see remarks).
  /// \param uiNewSize       - The new buffer size. This may be zero.
  /// \param bDiscardContent - Whether to discard previous buffer content.
  ///
  /// \return A reference-counted pointer to the new buffer.
  ///
  /// \remarks If pCommandList is non-null, existing contents are copied to the new internal buffer.
  ///          If pCommandList is null, the new internal buffer is created but existing contents are not copied.
  ///          If uiNewSize is zero, the internal buffer will be released.
  xiiSharedPtr<xiiGALBuffer> Resize(xiiSharedPtr<xiiGALCommandList> pCommandList, xiiUInt64 uiNewSize, bool bDiscardContent = false);

  /// Updates the internal buffer object, initializing or resizing as necessary.
  ///
  /// \param pCommandList - The command list that will be used to copy existing contents to the new buffer. This parameter may be null (see remarks).
  ///
  /// \return A reference-counted pointer to the new buffer.
  ///
  /// \remarks If the buffer has been resized, but the internal buffer has not been initialized, pCommandList must not be null.
  ///          If the buffer does not need to be updated (PendingUpdate() returns false), pCommandList may be null.
  xiiSharedPtr<xiiGALBuffer> Update(xiiSharedPtr<xiiGALCommandList> pCommandList);

private:
  void InitializeBuffer();
  void ResolvePendingResize(xiiSharedPtr<xiiGALCommandList> pCommandList, bool bPermitNull);
  void CopyStaleBuffer(xiiSharedPtr<xiiGALCommandList> pCommandList);

  xiiSharedPtr<xiiGALDevice> m_pDevice;

  xiiGALBufferCreationDescription m_Description;

  xiiAtomicIntegerU32 m_Version;

  mutable xiiMutex           m_Mutex;
  xiiSharedPtr<xiiGALBuffer> m_pBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pStaleBuffer;

  xiiUInt64 m_uiPendingSize = 0ULL;

  xiiUInt64 m_uiNextAfterResizeFenceValue = 1ULL;
  xiiUInt64 m_uiLastAfterResizeFenceValue = 0ULL;

  xiiSharedPtr<xiiGALFence> m_pAfterResizeFence;
};
