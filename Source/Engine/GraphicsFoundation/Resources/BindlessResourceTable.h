/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsFoundation/Resources/BindlessResource.h>
#include <GraphicsFoundation/Resources/BufferView.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Resources/TextureView.h>
#include <GraphicsFoundation/States/PipelineResourceSignature.h>

class xiiGALCommandList;

/// Capacity policy for the engine-wide sparse descriptor tables.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALBindlessResourceTableDescription
{
  xiiUInt32 m_uiBufferSRVCapacity  = XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY;
  xiiUInt32 m_uiBufferUAVCapacity  = XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY;
  xiiUInt32 m_uiTextureSRVCapacity = XII_GAL_DEFAULT_BINDLESS_RESOURCE_CAPACITY;
  xiiUInt32 m_uiTextureUAVCapacity = 1024U;
  xiiUInt32 m_uiSamplerCapacity    = 256U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALBindlessResourceTableDescription);

struct XII_GRAPHICSFOUNDATION_DLL xiiGALBindlessResourceTableStats
{
  XII_DECLARE_POD_TYPE();
  xiiUInt32 m_uiBufferSRVCount  = 0U;
  xiiUInt32 m_uiBufferUAVCount  = 0U;
  xiiUInt32 m_uiTextureSRVCount = 0U;
  xiiUInt32 m_uiTextureUAVCount = 0U;
  xiiUInt32 m_uiSamplerCount    = 0U;
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
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsFoundation, BindlessResourceTable);

public:
  xiiGALBindlessResourceTable() = delete;

  /// Rebuilds empty descriptor tables with a new capacity policy.
  /// Reconfiguration is rejected while live or retired handles still occupy slots.
  [[nodiscard]] static xiiResult                                     Configure(const xiiGALBindlessResourceTableDescription& description);
  [[nodiscard]] static const xiiGALBindlessResourceTableDescription& GetConfiguration();
  [[nodiscard]] static bool                                          IsInitialized();

  static void Clear();

  [[nodiscard]] static xiiGALBindlessResourceHandle RegisterBufferSRV(xiiSharedPtr<xiiGALBufferView> pView);
  [[nodiscard]] static xiiGALBindlessResourceHandle RegisterBufferUAV(xiiSharedPtr<xiiGALBufferView> pView);
  [[nodiscard]] static xiiGALBindlessResourceHandle RegisterTextureSRV(xiiSharedPtr<xiiGALTextureView> pView);
  [[nodiscard]] static xiiGALBindlessResourceHandle RegisterTextureUAV(xiiSharedPtr<xiiGALTextureView> pView);
  [[nodiscard]] static xiiGALBindlessResourceHandle RegisterSampler(xiiSharedPtr<xiiGALSampler> pSampler);

  static bool UpdateBufferSRV(xiiGALBindlessResourceHandle handle, xiiSharedPtr<xiiGALBufferView> pView);
  static bool UpdateBufferUAV(xiiGALBindlessResourceHandle handle, xiiSharedPtr<xiiGALBufferView> pView);
  static bool UpdateTextureSRV(xiiGALBindlessResourceHandle handle, xiiSharedPtr<xiiGALTextureView> pView);
  static bool UpdateTextureUAV(xiiGALBindlessResourceHandle handle, xiiSharedPtr<xiiGALTextureView> pView);
  static bool UpdateSampler(xiiGALBindlessResourceHandle handle, xiiSharedPtr<xiiGALSampler> pSampler);

  static bool RetireBufferSRV(xiiGALBindlessResourceHandle handle, xiiUInt64 uiLastUseFenceValue);
  static bool RetireBufferUAV(xiiGALBindlessResourceHandle handle, xiiUInt64 uiLastUseFenceValue);
  static bool RetireTextureSRV(xiiGALBindlessResourceHandle handle, xiiUInt64 uiLastUseFenceValue);
  static bool RetireTextureUAV(xiiGALBindlessResourceHandle handle, xiiUInt64 uiLastUseFenceValue);
  static bool RetireSampler(xiiGALBindlessResourceHandle handle, xiiUInt64 uiLastUseFenceValue);

  static void Collect(xiiUInt64 uiCompletedFenceValue);

  static void BindBufferSRVs(xiiGALCommandList& commandList, const xiiTempHashedString& sResourceName, xiiBitflags<xiiGALShaderType> stages = xiiGALShaderType::Unknown);
  static void BindBufferUAVs(xiiGALCommandList& commandList, const xiiTempHashedString& sResourceName, xiiBitflags<xiiGALShaderType> stages = xiiGALShaderType::Unknown);
  static void BindTextureSRVs(xiiGALCommandList& commandList, const xiiTempHashedString& sResourceName, xiiBitflags<xiiGALShaderType> stages = xiiGALShaderType::Unknown);
  static void BindTextureUAVs(xiiGALCommandList& commandList, const xiiTempHashedString& sResourceName, xiiBitflags<xiiGALShaderType> stages = xiiGALShaderType::Unknown);
  static void BindSamplers(xiiGALCommandList& commandList, const xiiTempHashedString& sResourceName, xiiBitflags<xiiGALShaderType> stages = xiiGALShaderType::Unknown);

  [[nodiscard]] static xiiGALBindlessResourceTableStats GetStats();

private:
  class State;

  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();
  static void Initialize(const xiiGALBindlessResourceTableDescription& description);

  static xiiUniquePtr<State> s_pState;
};
