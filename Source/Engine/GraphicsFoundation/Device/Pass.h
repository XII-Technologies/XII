#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/Resources/RenderPass.h>
#include <GraphicsFoundation/Resources/RenderTargetSetup.h>

/// \brief This describes the shader variable property flags.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALCommandEncoderType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Graphics,
    Compute,
    Mesh,
    RayTracing,
    Tile,

    ENUM_COUNT,

    Invalid = 0xFFU,

    Default = Graphics
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALCommandEncoderType);

class XII_GRAPHICSFOUNDATION_DLL xiiGALPass
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALPass);

protected:
  xiiGALPass(xiiGALDevice& device);
  virtual ~xiiGALPass();

public:
  xiiGALGraphicsCommandEncoder* BeginRendering(const xiiGALRenderingSetup& renderingSetup, xiiStringView sName = {});
  void                          EndRendering(xiiGALGraphicsCommandEncoder* pCommandEncoder);

  xiiGALComputeCommandEncoder* BeginCompute(xiiStringView sName = {});
  void                         EndCompute(xiiGALComputeCommandEncoder* pCommandEncoder);

  void ReleaseCachedRenderPassesAndFramebuffers();

#if 0 // Not yet implemented.
  xiiGALGraphicsCommandEncoder* BeginMesh(xiiStringView sName = {});
  void                          EndMesh(xiiGALGraphicsCommandEncoder* pCommandEncoder);

  xiiGALComputeCommandEncoder* BeginRayTracing(xiiStringView sName = {});
  void                         EndRayTracing(xiiGALComputeCommandEncoder* pCommandEncoder);
#endif

protected:
  virtual xiiGALGraphicsCommandEncoder* BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiStringView sName = {}) = 0;
  virtual void                          EndRenderingPlatform(xiiGALGraphicsCommandEncoder* pCommandEncoder)                                                                                          = 0;

  virtual xiiGALComputeCommandEncoder* BeginComputePlatform(xiiStringView sName = {})                   = 0;
  virtual void                         EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder) = 0;

#if 0 // Not yet implemented.
  virtual xiiGALGraphicsCommandEncoder* BeginMeshPlatform(xiiStringView sName = {})                    = 0;
  virtual void                          EndMeshPlatform(xiiGALGraphicsCommandEncoder* pCommandEncoder) = 0;

  virtual xiiGALComputeCommandEncoder* BeginRayTracingPlatform(xiiStringView sName = {})                   = 0;
  virtual void                         EndRayTracingPlatform(xiiGALComputeCommandEncoder* pCommandEncoder) = 0;
#endif

protected:
  struct RenderPassFrameBufferInfo
  {
    XII_DECLARE_POD_TYPE();

    xiiGALRenderPassHandle  hRenderPass;
    xiiGALFramebufferHandle hFrameBuffer;
  };

  struct ResourceCacheHash
  {
    static xiiUInt32 Hash(const xiiGALRenderTargetSetup& renderTargetSetup);
    static bool      Equal(const xiiGALRenderTargetSetup& a, const xiiGALRenderTargetSetup& b);

    static xiiUInt32 Hash(const xiiGALRenderingSetup& renderingSetup);
    static bool      Equal(const xiiGALRenderingSetup& a, const xiiGALRenderingSetup& b);
  };

  xiiEventSubscriptionID m_DeviceEventID;

  void GetRenderPassAndFramebuffer(const xiiGALRenderingSetup& renderingSetup, xiiGALRenderPass** out_pRenderPass, xiiGALFramebuffer** out_pFramebuffer);

  xiiGALDevice& m_Device;

  bool                              m_bMarkerPushed             = false;
  xiiEnum<xiiGALCommandEncoderType> m_CurrentCommandEncoderType = xiiGALCommandEncoderType::Invalid;

  xiiHashTable<xiiGALRenderTargetSetup, xiiGALRenderPassHandle, xiiGALPass::ResourceCacheHash> m_RenderPassCache;
  xiiHashTable<xiiGALRenderingSetup, RenderPassFrameBufferInfo, xiiGALPass::ResourceCacheHash> m_FramebufferCache;
};

#include <GraphicsFoundation/Device/Implementation/Pass_inl.h>
