/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Lighting/LightingSystem.h>

class xiiLightingManagerState;

/// Generation-checked reference to one view's lighting data and GPU resources.
///
/// The handle is safe to retain after high-level shutdown. It simply becomes invalid when the
/// subsystem releases its contexts; it never owns allocator-backed or GAL state itself.
struct XII_GRAPHICSCORE_DLL xiiLightingContextHandle
{
  XII_DECLARE_POD_TYPE();

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex && m_uiGeneration != 0U; }

  xiiUInt32 m_uiIndex      = xiiInvalidIndex;
  xiiUInt32 m_uiGeneration = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLightingContextHandle);

/// GraphicsCore subsystem that owns every per-view lighting context.
///
/// Lighting remains isolated per camera, but all allocator-backed arrays and GAL resources are
/// created after Foundation startup and destroyed during high-level shutdown while the device is
/// still alive. Views retain handles rather than directly owning teardown-sensitive state.
class XII_GRAPHICSCORE_DLL xiiLightingManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiLightingManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, LightingManager);

public:
  xiiLightingManager() = delete;

  [[nodiscard]] static bool IsInitialized();

  [[nodiscard]] static xiiLightingContextHandle CreateContext(const xiiLightingSystemSettings& settings = {});
  static void                                      DestroyContext(xiiLightingContextHandle handle);
  [[nodiscard]] static bool                        IsValid(xiiLightingContextHandle handle);

  /// Returns a non-owning context pointer. It is valid until DestroyContext or subsystem shutdown.
  [[nodiscard]] static xiiLightingSystem*       GetContext(xiiLightingContextHandle handle);
  [[nodiscard]] static const xiiLightingSystem* GetContextConst(xiiLightingContextHandle handle);

private:
  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  static xiiUniquePtr<xiiLightingManagerState> s_pState;
};

/// Lightweight per-owner facade over a subsystem-owned lighting context.
///
/// This type intentionally stores only a generation-checked handle. It preserves the convenient
/// per-view API without making the view responsible for allocator or GAL resource lifetime.
class XII_GRAPHICSCORE_DLL xiiLightingContext
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiLightingContext);

public:
  xiiLightingContext() = default;
  ~xiiLightingContext();

  [[nodiscard]] xiiResult Initialize(const xiiLightingSystemSettings& settings = {});
  void                    Shutdown();
  [[nodiscard]] bool      IsInitialized() const;

  void BuildFrameData(const xiiView& view, const xiiExtractedRenderData& extractedData, xiiUInt32 uiFrameIndex);
  void UploadFrameData(xiiGALCommandList& ref_commandList);

  void BindFrameConstants(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const;
  void BindLightData(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const;
  void BindIESProfiles(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const;
  void BindLightingResources(xiiGALCommandList& ref_commandList, xiiBitflags<xiiGALShaderType> shaderStages) const;
  void WriteBlackboard(xiiRenderGraphBlackboard& ref_blackboard) const;

  [[nodiscard]] const xiiLightingSystemSettings&          GetSettings() const;
  [[nodiscard]] const xiiLightingSystem::FrameStatistics& GetFrameStatistics() const;
  [[nodiscard]] xiiUInt32                                 GetActiveLightCount() const;
  [[nodiscard]] xiiUInt32                                 GetClusterCountX() const;
  [[nodiscard]] xiiUInt32                                 GetClusterCountY() const;
  [[nodiscard]] xiiUInt32                                 GetClusterCountZ() const;
  [[nodiscard]] xiiUInt32                                 GetTotalClusterCount() const;
  [[nodiscard]] xiiGALBuffer*                             GetLightDataBuffer() const;

  /// Non-owning access for graph extensions that consume the view's lighting context.
  [[nodiscard]] xiiLightingSystem* BorrowSystem() const;

private:
  [[nodiscard]] xiiLightingSystem* GetSystem() const;

  xiiLightingContextHandle m_Handle;
};

