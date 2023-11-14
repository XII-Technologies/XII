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
  xiiGALGraphicsCommandEncoder* BeginRendering(const xiiGALRenderPassCreationDescription& renderPassDescription);
  void                          EndRendering(xiiGALGraphicsCommandEncoder* pCommandEncoder);

  xiiGALComputeCommandEncoder* BeginCompute(xiiStringView sName = {});
  void                         EndCompute(xiiGALComputeCommandEncoder* pCommandEncoder);

#if 0 // Not yet implemented.
  xiiGALGraphicsCommandEncoder* BeginMesh(xiiStringView sName = {});
  void                          EndMesh(xiiGALGraphicsCommandEncoder* pCommandEncoder);

  xiiGALComputeCommandEncoder* BeginRayTracing(xiiStringView sName = {});
  void                         EndRayTracing(xiiGALComputeCommandEncoder* pCommandEncoder);
#endif

protected:
  virtual xiiGALGraphicsCommandEncoder* BeginRenderingPlatform(xiiGALRenderPass* pRenderPass, xiiGALFramebuffer* pFramebuffer, xiiStringView sName = {}) = 0;
  virtual void                          EndRenderingPlatform(xiiGALGraphicsCommandEncoder* pCommandEncoder)                                              = 0;

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
    xiiGALRenderPassHandle  hRenderPass;
    xiiGALFramebufferHandle hFrameBuffer;
  };

  void GetRenderPassAndFramebuffer(const xiiGALRenderPassCreationDescription& renderPassDescription, xiiGALRenderPass* out_pRenderPass, xiiGALFramebuffer* out_pFramebuffer);

  xiiGALDevice& m_Device;

  bool                              m_bMarkerPushed             = false;
  xiiEnum<xiiGALCommandEncoderType> m_CurrentCommandEncoderType = xiiGALCommandEncoderType::Invalid;

  xiiHashTable<xiiUInt32, RenderPassFrameBufferInfo> m_RenderPassFramebufferCache;
};

#include <GraphicsFoundation/Device/Implementation/Pass_inl.h>
