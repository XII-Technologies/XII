/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/PipelineState.h>

struct ID3D12RootSignature;
struct ID3D12StateObject;
struct ID3D12StateObjectProperties;

class XII_GRAPHICSD3D12_DLL xiiGALRayTracingPipelineStateD3D12 final : public xiiGALRayTracingPipelineState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALRayTracingPipelineStateD3D12, xiiGALRayTracingPipelineState);

public:
  [[nodiscard]] XII_ALWAYS_INLINE ID3D12StateObject* GetD3D12StateObject() const { return m_pD3D12StateObject; }
  [[nodiscard]] XII_ALWAYS_INLINE ID3D12RootSignature* GetD3D12RootSignature() const { return m_pD3D12RootSignature; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiUInt8> GetShaderGroupHandles() const { return m_ShaderGroupHandles; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiUInt32> GetRayGenerationGroupIndices() const { return m_RayGenerationGroupIndices; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiUInt32> GetMissGroupIndices() const { return m_MissGroupIndices; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiUInt32> GetHitGroupIndices() const { return m_HitGroupIndices; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiArrayPtr<const xiiUInt32> GetCallableGroupIndices() const { return m_CallableGroupIndices; }

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALRayTracingPipelineStateD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALRayTracingPipelineStateCreationDescription& creationDescription);

  virtual ~xiiGALRayTracingPipelineStateD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  ID3D12StateObject*           m_pD3D12StateObject           = nullptr;
  ID3D12StateObjectProperties* m_pD3D12StateObjectProperties = nullptr;
  ID3D12RootSignature*         m_pD3D12RootSignature         = nullptr;

  xiiDynamicArray<xiiUInt8>  m_ShaderGroupHandles;
  xiiDynamicArray<xiiUInt32> m_RayGenerationGroupIndices;
  xiiDynamicArray<xiiUInt32> m_MissGroupIndices;
  xiiDynamicArray<xiiUInt32> m_HitGroupIndices;
  xiiDynamicArray<xiiUInt32> m_CallableGroupIndices;
};
