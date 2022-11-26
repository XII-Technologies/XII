#pragma once

#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

XII_DECLARE_FLAGS(xiiUInt8, xiiReflectionProbeUpdaterFlags, SkyLight, HasCustomCubeMap);

/// \brief Renders reflection probes and stores filtered mipmap chains into an atlas texture as well as computing sky irradiance
/// Rendering sky irradiance is optional and only done if m_iIrradianceOutputIndex != -1.
class xiiReflectionProbeUpdater
{
public:
  /// \brief Defines the target specular reflection probe atlas and index as well as the sky irradiance atlas and index in case the rendered cube map is a sky light.
  struct TargetSlot
  {
    xiiGALTextureHandle m_hSpecularOutputTexture;      ///< Must be a valid cube map texture array handle.
    xiiGALTextureHandle m_hIrradianceOutputTexture;    ///< Optional. Must be set if m_iIrradianceOutputIndex != -1.
    xiiInt32            m_iSpecularOutputIndex   = -1; ///< Must be a valid index into the atlas texture.
    xiiInt32            m_iIrradianceOutputIndex = -1; ///< If -1, no irradiance is computed.
  };

public:
  xiiReflectionProbeUpdater();
  ~xiiReflectionProbeUpdater();

  /// \brief Returns how many new probes can be started this frame.
  /// \param out_updatesFinished Contains the probes that finished last frame.
  /// \return The number of new probes can be started this frame.
  xiiUInt32 GetFreeUpdateSlots(xiiDynamicArray<xiiReflectionProbeRef>& out_updatesFinished);

  /// \brief Starts rendering a new reflection probe.
  /// \param probe The world and probe index to be rendered. Used as an identifier.
  /// \param desc Probe render settings.
  /// \param globalTransform World position to be rendered.
  /// \param target Where the probe should be rendered into.
  /// \return Returns XII_FAILURE if no more free slots are available.
  xiiResult StartDynamicUpdate(const xiiReflectionProbeRef& probe, const xiiReflectionProbeDesc& desc, const xiiTransform& globalTransform, const TargetSlot& target);

  /// \brief Starts filtering an existing cube map into a new reflection probe.
  /// \param probe The world and probe index to be rendered. Used as an identifier.
  /// \param desc Probe render settings.
  /// \param sourceTexture Cube map that should be filtered into a reflection probe.
  /// \param target Where the probe should be rendered into.
  /// \return Returns XII_FAILURE if no more free slots are available.
  xiiResult StartFilterUpdate(const xiiReflectionProbeRef& probe, const xiiReflectionProbeDesc& desc, xiiTextureCubeResourceHandle sourceTexture, const TargetSlot& target);

  /// \brief Cancel a previously started update.
  void CancelUpdate(const xiiReflectionProbeRef& probe);

  /// \brief Generates update steps. Should be called in PreExtraction phase.
  void GenerateUpdateSteps();

  /// \brief Schedules probe rendering views. Should be called at some point during the extraction phase. Can be called multiple times. It will only do work on the first call after GenerateUpdateSteps.
  void ScheduleUpdateSteps();

private:
  struct ReflectionView
  {
    xiiViewHandle m_hView;
    xiiCamera     m_Camera;
  };

  struct UpdateStep
  {
    typedef xiiUInt8 StorageType;

    enum Enum
    {
      RenderFace0,
      RenderFace1,
      RenderFace2,
      RenderFace3,
      RenderFace4,
      RenderFace5,
      Filter,

      ENUM_COUNT,

      Default = Filter
    };

    static bool IsRenderStep(Enum value) { return value >= UpdateStep::RenderFace0 && value <= UpdateStep::RenderFace5; }
    static Enum NextStep(Enum value) { return static_cast<UpdateStep::Enum>((value + 1) % UpdateStep::ENUM_COUNT); }
  };

  struct ProbeUpdateInfo
  {
    ProbeUpdateInfo();
    ~ProbeUpdateInfo();

    xiiBitflags<xiiReflectionProbeUpdaterFlags> m_flags;
    xiiReflectionProbeRef                       m_probe;
    xiiReflectionProbeDesc                      m_desc;
    xiiTransform                                m_globalTransform;
    xiiTextureCubeResourceHandle                m_sourceTexture;
    TargetSlot                                  m_TargetSlot;

    struct Step
    {
      XII_DECLARE_POD_TYPE();

      xiiUInt8            m_uiViewIndex;
      xiiEnum<UpdateStep> m_UpdateStep;
    };

    bool                m_bInUse = false;
    xiiEnum<UpdateStep> m_LastUpdateStep;

    xiiHybridArray<Step, 8> m_UpdateSteps;

    xiiGALTextureHandle m_hCubemap;
    xiiGALTextureHandle m_hCubemapProxies[6];
  };

private:
  static void CreateViews(
    xiiDynamicArray<ReflectionView>& views,
    xiiUInt32                        uiMaxRenderViews,
    const char*                      szNameSuffix,
    const char*                      szRenderPipelineResource);
  void CreateReflectionViewsAndResources();

  void ResetProbeUpdateInfo(xiiUInt32 uiInfo);
  void AddViewToRender(const ProbeUpdateInfo::Step& step, ProbeUpdateInfo& updateInfo);

  bool m_bUpdateStepsFlushed = true;

  xiiDynamicArray<ReflectionView> m_RenderViews;
  xiiDynamicArray<ReflectionView> m_FilterViews;

  // Active Dynamic Updates
  xiiDynamicArray<xiiUniquePtr<ProbeUpdateInfo>> m_DynamicUpdates;
  xiiHybridArray<xiiReflectionProbeRef, 4>       m_FinishedLastFrame;
};
