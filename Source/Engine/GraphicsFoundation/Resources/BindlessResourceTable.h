/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/Resources/BindlessResource.h>
#include <GraphicsFoundation/Resources/BufferView.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Resources/TextureView.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>

class xiiGALCommandList;

/// Capacity policy for the engine-wide sparse descriptor tables.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBindlessResourceTableDescription
{
  xiiUInt32 m_uiBufferSRVCapacity = XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY;
  xiiUInt32 m_uiBufferUAVCapacity = XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY;
  xiiUInt32 m_uiTextureSRVCapacity = XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY;
  xiiUInt32 m_uiTextureUAVCapacity = 1024U;
  xiiUInt32 m_uiSamplerCapacity = 256U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALBindlessResourceTableDescription);

struct XII_GRAPHICSFOUNDATION_DLL xiiGALBindlessResourceTableStats
{
  XII_DECLARE_POD_TYPE();
  xiiUInt32 m_uiBufferSRVCount = 0U;
  xiiUInt32 m_uiBufferUAVCount = 0U;
  xiiUInt32 m_uiTextureSRVCount = 0U;
  xiiUInt32 m_uiTextureUAVCount = 0U;
  xiiUInt32 m_uiSamplerCount = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALBindlessResourceTableStats);

/// Backend-independent owner for sparse bindless descriptor tables.
///
/// Resources remain strongly referenced after Retire() until Collect() observes the last-use
/// fence. The descriptor and its stable index therefore cannot alias while an older command list
/// is still executing. A table can be rebound to every command list because only occupied slots
/// are emitted by descriptor-indexing backends.
class XII_GRAPHICSFOUNDATION_DLL xiiGALBindlessResourceTable
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALBindlessResourceTable);

public:
  xiiGALBindlessResourceTable() = default;

  void Initialize(const xiiGALBindlessResourceTableDescription& description = {});
  void Clear();

  [[nodiscard]] xiiGALBindlessResourceHandle RegisterBufferSRV(xiiSharedPtr<xiiGALBufferView> pView);
  [[nodiscard]] xiiGALBindlessResourceHandle RegisterBufferUAV(xiiSharedPtr<xiiGALBufferView> pView);
  [[nodiscard]] xiiGALBindlessResourceHandle RegisterTextureSRV(xiiSharedPtr<xiiGALTextureView> pView);
  [[nodiscard]] xiiGALBindlessResourceHandle RegisterTextureUAV(xiiSharedPtr<xiiGALTextureView> pView);
  [[nodiscard]] xiiGALBindlessResourceHandle RegisterSampler(xiiSharedPtr<xiiGALSampler> pSampler);

  bool UpdateBufferSRV(xiiGALBindlessResourceHandle handle, xiiSharedPtr<xiiGALBufferView> pView);
  bool UpdateBufferUAV(xiiGALBindlessResourceHandle handle, xiiSharedPtr<xiiGALBufferView> pView);
  bool UpdateTextureSRV(xiiGALBindlessResourceHandle handle, xiiSharedPtr<xiiGALTextureView> pView);
  bool UpdateTextureUAV(xiiGALBindlessResourceHandle handle, xiiSharedPtr<xiiGALTextureView> pView);
  bool UpdateSampler(xiiGALBindlessResourceHandle handle, xiiSharedPtr<xiiGALSampler> pSampler);

  bool RetireBufferSRV(xiiGALBindlessResourceHandle handle, xiiUInt64 uiLastUseFenceValue);
  bool RetireBufferUAV(xiiGALBindlessResourceHandle handle, xiiUInt64 uiLastUseFenceValue);
  bool RetireTextureSRV(xiiGALBindlessResourceHandle handle, xiiUInt64 uiLastUseFenceValue);
  bool RetireTextureUAV(xiiGALBindlessResourceHandle handle, xiiUInt64 uiLastUseFenceValue);
  bool RetireSampler(xiiGALBindlessResourceHandle handle, xiiUInt64 uiLastUseFenceValue);

  void Collect(xiiUInt64 uiCompletedFenceValue);

  void BindBufferSRVs(xiiGALCommandList& commandList, const xiiTempHashedString& sResourceName, xiiBitflags<xiiGALShaderType> stages = xiiGALShaderType::Unknown) const;
  void BindBufferUAVs(xiiGALCommandList& commandList, const xiiTempHashedString& sResourceName, xiiBitflags<xiiGALShaderType> stages = xiiGALShaderType::Unknown) const;
  void BindTextureSRVs(xiiGALCommandList& commandList, const xiiTempHashedString& sResourceName, xiiBitflags<xiiGALShaderType> stages = xiiGALShaderType::Unknown) const;
  void BindTextureUAVs(xiiGALCommandList& commandList, const xiiTempHashedString& sResourceName, xiiBitflags<xiiGALShaderType> stages = xiiGALShaderType::Unknown) const;
  void BindSamplers(xiiGALCommandList& commandList, const xiiTempHashedString& sResourceName, xiiBitflags<xiiGALShaderType> stages = xiiGALShaderType::Unknown) const;

  [[nodiscard]] xiiGALBindlessResourceTableStats GetStats() const;

private:
  template <typename TObject>
  struct TableStorage
  {
    xiiGALBindlessResourceAllocator      m_Allocator;
    xiiDynamicArray<xiiSharedPtr<TObject>> m_Objects;
    xiiDynamicArray<xiiUInt64>           m_RetireFences;
  };

  template <typename TObject>
  static xiiGALBindlessResourceHandle Register(TableStorage<TObject>& table, xiiSharedPtr<TObject> pObject);
  template <typename TObject>
  static bool Update(TableStorage<TObject>& table, xiiGALBindlessResourceHandle handle, xiiSharedPtr<TObject> pObject);
  template <typename TObject>
  static bool Retire(TableStorage<TObject>& table, xiiGALBindlessResourceHandle handle, xiiUInt64 uiFenceValue);
  template <typename TObject>
  static void Collect(TableStorage<TObject>& table, xiiUInt64 uiCompletedFenceValue);

  mutable xiiMutex                    m_Mutex;
  TableStorage<xiiGALBufferView>      m_BufferSRVs;
  TableStorage<xiiGALBufferView>      m_BufferUAVs;
  TableStorage<xiiGALTextureView>     m_TextureSRVs;
  TableStorage<xiiGALTextureView>     m_TextureUAVs;
  TableStorage<xiiGALSampler>         m_Samplers;
};
