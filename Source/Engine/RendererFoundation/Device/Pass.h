
#pragma once

#include <RendererFoundation/Resources/RenderTargetSetup.h>

class XII_RENDERERFOUNDATION_DLL xiiGALPass
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALPass);

public:
  xiiGALRenderCommandEncoder* BeginRendering(const xiiGALRenderingSetup& renderingSetup, const char* szName = "");
  void                        EndRendering(xiiGALRenderCommandEncoder* pCommandEncoder);

  xiiGALComputeCommandEncoder* BeginCompute(const char* szName = "");
  void                         EndCompute(xiiGALComputeCommandEncoder* pCommandEncoder);

  xiiGALRenderCommandEncoder* BeginRenderPass();
  void                        EndRenderPass();

  // BeginRaytracing() could be here as well (would match Vulkan)

protected:
  virtual xiiGALRenderCommandEncoder* BeginRenderingPlatform(const xiiGALRenderingSetup& renderingSetup, const char* szName) = 0;
  virtual void                        EndRenderingPlatform(xiiGALRenderCommandEncoder* pCommandEncoder)                      = 0;

  virtual xiiGALComputeCommandEncoder* BeginComputePlatform(const char* szName)                         = 0;
  virtual void                         EndComputePlatform(xiiGALComputeCommandEncoder* pCommandEncoder) = 0;

  virtual xiiGALRenderCommandEncoder* BeginRenderPassPlatform() = 0;
  virtual void                        EndRenderPassPlatform()   = 0;

  xiiGALPass(xiiGALDevice& device);
  virtual ~xiiGALPass();

  xiiGALDevice& m_Device;

  enum class CommandEncoderType
  {
    Invalid,
    Render,
    Compute
  };

  CommandEncoderType m_CurrentCommandEncoderType = CommandEncoderType::Invalid;
  bool               m_bMarker                   = false;
};
