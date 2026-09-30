/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// Physical output represented by a simulated imaging sensor.
struct XII_GRAPHICSCORE_DLL xiiSensorType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    RGBCamera,
    InfraredCamera,
    DepthCamera,
    LiDAR,

    ENUM_COUNT,

    Default = RGBCamera
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSensorType);

/// Exposure model used when sampling a frame.
struct XII_GRAPHICSCORE_DLL xiiSensorShutterType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Global,
    Rolling,

    ENUM_COUNT,

    Default = Global
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSensorShutterType);

/// Noise model applied to the physically calibrated sensor signal.
struct XII_GRAPHICSCORE_DLL xiiSensorNoiseModel
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None,
    Gaussian,
    PhotonShotAndRead,

    ENUM_COUNT,

    Default = PhotonShotAndRead
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSensorNoiseModel);

/// Calibrated camera and detector response used by robotics and scientific views.
///
/// Focal length and principal point use pixel units so recorded calibration data can
/// be transferred without conversion. Distances are metres, exposure is seconds and
/// wavelength is nanometres. Spectral sensitivity converts scene RGB radiance into a
/// scalar detector response for infrared, depth-return and LiDAR modes.
struct XII_GRAPHICSCORE_DLL xiiSensorProfile
{
  xiiEnum<xiiSensorType>        m_Type;
  xiiEnum<xiiSensorShutterType> m_Shutter;
  xiiEnum<xiiSensorNoiseModel>  m_NoiseModel;

  xiiUInt32 m_uiResolutionX = 1280U;
  xiiUInt32 m_uiResolutionY = 720U;

  float m_fFocalLengthXPixels = 640.0f;
  float m_fFocalLengthYPixels = 640.0f;
  float m_fPrincipalPointXPixels = 639.5f;
  float m_fPrincipalPointYPixels = 359.5f;

  float m_fNearPlaneMeters = 0.05f;
  float m_fFarPlaneMeters = 1000.0f;
  float m_fExposureSeconds = 1.0f / 60.0f;
  float m_fRollingShutterSeconds = 0.0f;

  float   m_fWavelengthNanometers = 550.0f;
  xiiVec3 m_vSpectralSensitivity = xiiVec3(0.2126f, 0.7152f, 0.0722f);
  float   m_fQuantumEfficiency = 0.72f;
  float   m_fRadianceToElectrons = 10000.0f;
  float   m_fAnalogGain = 1.0f;
  float   m_fReadNoiseElectrons = 1.5f;
  float   m_fShotNoiseScale = 1.0f;
  float   m_fSaturationElectrons = 30000.0f;
  float   m_fDepthNoiseStandardDeviationMeters = 0.002f;
  float   m_fDepthNoiseScalePerMeter = 0.001f;
  xiiUInt32 m_uiOutputBitDepth = 16U;
  xiiUInt32 m_uiNoiseSeed = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSensorProfile);

/// Stable process-local identity of an immutable sensor calibration profile.
struct XII_GRAPHICSCORE_DLL xiiSensorProfileHandle
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiIndex = xiiInvalidIndex;

  [[nodiscard]] bool IsValid() const { return m_uiIndex != xiiInvalidIndex; }
  [[nodiscard]] bool operator==(const xiiSensorProfileHandle& rhs) const { return m_uiIndex == rhs.m_uiIndex; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSensorProfileHandle);

/// Tool-facing registry statistics.
struct XII_GRAPHICSCORE_DLL xiiSensorProfileRegistryStats
{
  xiiUInt32 m_uiProfileCount = 0U;
  xiiUInt32 m_uiRGBProfileCount = 0U;
  xiiUInt32 m_uiInfraredProfileCount = 0U;
  xiiUInt32 m_uiDepthProfileCount = 0U;
  xiiUInt32 m_uiLiDARProfileCount = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSensorProfileRegistryStats);

/// Process-wide immutable calibration registry.
///
/// The registry is allocated by the GraphicsCore subsystem after Foundation's
/// allocator has started. Sensor profiles are immutable and deduplicated, so render
/// views, recorders and editor tools can safely share stable handles across frames.
class XII_GRAPHICSCORE_DLL xiiSensorRenderingManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSensorRenderingManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, SensorRenderingManager);

public:
  xiiSensorRenderingManager() = delete;

  [[nodiscard]] static bool IsInitialized();
  [[nodiscard]] static bool IsValidProfile(const xiiSensorProfile& profile);

  /// Finds an identical immutable profile or appends a new one.
  [[nodiscard]] static xiiResult AcquireProfile(const xiiSensorProfile& profile, xiiSensorProfileHandle& out_handle);
  [[nodiscard]] static xiiSensorProfileHandle GetDefaultProfileHandle();
  [[nodiscard]] static xiiSensorProfileHandle GetDefaultProfileHandle(xiiSensorType::Enum type);
  [[nodiscard]] static xiiResult GetProfile(xiiSensorProfileHandle handle, xiiSensorProfile& out_profile);
  [[nodiscard]] static xiiSensorProfileRegistryStats GetRegistryStats();

private:
  static void Startup();
  static void Shutdown();

  class State;
  static xiiUniquePtr<State> s_pState;
};

