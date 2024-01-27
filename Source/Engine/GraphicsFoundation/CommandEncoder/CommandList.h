#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Math/Rect.h>

#include <GraphicsFoundation/Declarations/DeviceObject.h>
#include <GraphicsFoundation/Declarations/GraphicsTypes.h>

/// \brief Interface that defines methods to manipulate a command list object.
class XII_GRAPHICSFOUNDATION_DLL xiiGALCommandList : public xiiGALDeviceObject
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandList, xiiGALDeviceObject);

public:
  // State functions.

  void SetPipelineState(xiiGALPipelineStateHandle hPipelineState);

  void SetStencilRef(xiiUInt8 uiStencilRef);
  void SetBlendFactor(const xiiColor& blendFactor);
  void SetBlendFactor(const xiiColor& blendFactor);

  void SetViewports(xiiArrayPtr<xiiRectFloat> pViewports, float fMinDepth = 0.0f, float fMaxDepth = 1.0f);
  void SetScissorRects(xiiArrayPtr<xiiRectU32> pRects);

  void SetIndexBuffer(xiiGALBufferHandle hIndexBuffer, xiiUInt32 uiByteOffset = 0U);
  void SetVertexBuffers(xiiUInt32 uiStartSlot, xiiArrayPtr<xiiGALBufferHandle> pVertexBuffers, xiiArrayPtr<xiiUInt32> pByteOffsets);

  void ClearRenderTargetView(xiiGALTextureViewHandle hRenderTargetView, const xiiColor& clearColor);
  void ClearDepthStencilView(xiiGALTextureViewHandle hDepthStencilView, bool bClearDepth, bool bClearStencil, float fDepthClear, xiiUInt8 uiStencilClear);

  // Draw functions.

  xiiResult Draw(xiiUInt32 uiVertexCount, xiiUInt32 uiStartVertex);
  xiiResult DrawIndexed(xiiUInt32 uiIndexCount, xiiUInt32 uiStartIndex);
  xiiResult DrawIndexedInstanced(xiiUInt32 uiIndexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartIndex);
  xiiResult DrawIndexedInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);
  xiiResult DrawInstanced(xiiUInt32 uiVertexCountPerInstance, xiiUInt32 uiInstanceCount, xiiUInt32 uiStartVertex);
  xiiResult DrawInstancedIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);

  // Dispatch functions.

  xiiResult Dispatch(xiiUInt32 uiThreadGroupCountX, xiiUInt32 uiThreadGroupCountY, xiiUInt32 uiThreadGroupCountZ);
  xiiResult DispatchIndirect(xiiGALBufferHandle hIndirectArgumentBuffer, xiiUInt32 uiArgumentOffsetInBytes);

  // Query functions.

  void BeginQuery(xiiGALQueryHandle hQuery);
  void EndQuery(xiiGALQueryHandle hQuery);

  // Debug functions.

  void BeginDebugGroup(xiiStringView sName, const xiiColor& color = xiiColor::Black);
  void EndDebugGroup();

  void InsertDebugLabel(xiiStringView sName, const xiiColor& color = xiiColor::Black);

  void InvalidateState();

protected:
  friend class xiiGALDevice;

  xiiGALCommandList();

  virtual ~xiiGALCommandList();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) = 0;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) = 0;
};

#include <GraphicsFoundation/CommandEncoder/Implementation/CommandList_inl.h>
