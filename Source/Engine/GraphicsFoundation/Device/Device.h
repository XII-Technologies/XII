#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>

/// \brief The xiiRenderDevice class is the primary interface for interactions with rendering APIs.
/// It contains a set of (non-virtual) functions to set state, create resources etc. which rely on API specific implementations provided by protected virtual functions.
/// Redundant state changes are prevented at the platform independent level in the non-virtual functions.
class XII_GRAPHICSFOUNDATION_DLL xiiGALDevice
{
public:
  // Initialize and shutdown functions

  xiiResult Initialize();
  xiiResult Shutdown();

  void BeginPipeline(const char* szName, xiiGALSwapChainHandle hSwapChain);
  void EndPipeline(xiiGALSwapChainHandle hSwapChain);

  xiiGALPass* BeginPass(const char* szName);
  void        EndPass(xiiGALPass* pPass);

  // State creation functions

  xiiGALBlendStateHandle CreateBlendState(const xiiGALBlendStateCreationDescription& description);
  void                   DestroyBlendState(xiiGALBlendStateHandle hBlendState);

  xiiGALDepthStencilStateHandle CreateDepthStencilState(const xiiGALDepthStencilStateCreationDescription& description);
  void                          DestroyDepthStencilState(xiiGALDepthStencilStateHandle hDepthStencilState);

  xiiGALRasterizerStateHandle CreateRasterizerState(const xiiGALRasterizerStateCreationDescription& description);
  void                        DestroyRasterizerState(xiiGALRasterizerStateHandle hRasterizerState);

  // Resource creation functions

  xiiGALShaderHandle CreateShader(const xiiGALShaderCreationDescription& description);
  void               DestroyShader(xiiGALShaderHandle hShader);

  xiiGALBufferHandle CreateBuffer(const xiiGALBufferCreationDescription& description, xiiArrayPtr<const xiiUInt8> initialData = xiiArrayPtr<const xiiUInt8>());
  void               DestroyBuffer(xiiGALBufferHandle hBuffer);

  xiiGALSamplerHandle CreateSampler(const xiiGALSamplerCreationDescription& description);
  void                DestroySampler(xiiGALSamplerHandle hSamplerState);

  xiiGALTextureHandle CreateTexture(const xiiGALTextureCreationDescription& description, xiiArrayPtr<xiiGALTextureData> initialData = xiiArrayPtr<xiiGALTextureData>());
  void                DestroyTexture(xiiGALTextureHandle hTexture);

  // Resource views
  xiiGALBufferViewHandle  GetDefaultResourceView(xiiGALBufferHandle hBuffer);
  xiiGALTextureViewHandle GetDefaultResourceView(xiiGALTextureHandle hTexture);

  xiiGALBufferViewHandle  CreateBufferView(const xiiGALBufferViewCreationDescription& description);
  xiiGALTextureViewHandle CreateTextureView(const xiiGALTextureViewCreationDescription& description);
  void                    DestroyBufferView(xiiGALBufferViewHandle hBufferView);
  void                    DestroyTextureView(xiiGALTextureViewHandle hTextureView);

  // Helper functions for buffers (for common, simple use cases)

  xiiGALBufferHandle CreateVertexBuffer(xiiUInt32 uiVertexSize, xiiUInt32 uiVertexCount, const char* szName, xiiArrayPtr<const xiiUInt8> initialData = xiiArrayPtr<const xiiUInt8>(), bool bDataIsMutable = false);
  xiiGALBufferHandle CreateIndexBuffer(xiiUInt8 uiSize, xiiUInt32 uiIndexCount, const char* szName, xiiArrayPtr<const xiiUInt8> initialData = xiiArrayPtr<const xiiUInt8>(), bool bDataIsMutable = false);
  xiiGALBufferHandle CreateConstantBuffer(xiiUInt32 uiBufferSize, const char* szName);

public:
  xiiEvent<const xiiGALDeviceEvent&> m_Events;
};

#include <GraphicsFoundation/Device/Implementation/Device_inl.h>
