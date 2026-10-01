/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Lighting/SensorRendering.h>

namespace
{
  bool IsEqual(const xiiSensorProfile& lhs, const xiiSensorProfile& rhs)
  {
    return lhs.m_Type == rhs.m_Type && lhs.m_Shutter == rhs.m_Shutter && lhs.m_NoiseModel == rhs.m_NoiseModel &&
      lhs.m_uiResolutionX == rhs.m_uiResolutionX && lhs.m_uiResolutionY == rhs.m_uiResolutionY &&
      lhs.m_fFocalLengthXPixels == rhs.m_fFocalLengthXPixels && lhs.m_fFocalLengthYPixels == rhs.m_fFocalLengthYPixels &&
      lhs.m_fPrincipalPointXPixels == rhs.m_fPrincipalPointXPixels && lhs.m_fPrincipalPointYPixels == rhs.m_fPrincipalPointYPixels &&
      lhs.m_fNearPlaneMeters == rhs.m_fNearPlaneMeters && lhs.m_fFarPlaneMeters == rhs.m_fFarPlaneMeters &&
      lhs.m_fExposureSeconds == rhs.m_fExposureSeconds && lhs.m_fRollingShutterSeconds == rhs.m_fRollingShutterSeconds &&
      lhs.m_fWavelengthNanometers == rhs.m_fWavelengthNanometers && lhs.m_vSpectralSensitivity == rhs.m_vSpectralSensitivity &&
      lhs.m_fQuantumEfficiency == rhs.m_fQuantumEfficiency && lhs.m_fRadianceToElectrons == rhs.m_fRadianceToElectrons && lhs.m_fAnalogGain == rhs.m_fAnalogGain &&
      lhs.m_fReadNoiseElectrons == rhs.m_fReadNoiseElectrons && lhs.m_fShotNoiseScale == rhs.m_fShotNoiseScale &&
      lhs.m_fSaturationElectrons == rhs.m_fSaturationElectrons &&
      lhs.m_fDepthNoiseStandardDeviationMeters == rhs.m_fDepthNoiseStandardDeviationMeters && lhs.m_fDepthNoiseScalePerMeter == rhs.m_fDepthNoiseScalePerMeter &&
      lhs.m_uiOutputBitDepth == rhs.m_uiOutputBitDepth &&
      lhs.m_uiNoiseSeed == rhs.m_uiNoiseSeed;
  }
} // namespace

class xiiSensorRenderingManager::State
{
public:
  mutable xiiMutex                  m_Mutex;
  xiiDynamicArray<xiiSensorProfile> m_Profiles;
  xiiSensorProfileHandle            m_hDefaultProfiles[xiiSensorType::ENUM_COUNT];
};

xiiUniquePtr<xiiSensorRenderingManager::State> xiiSensorRenderingManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, SensorRenderingManager)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiSensorRenderingManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiSensorRenderingManager::Shutdown();
  }
XII_END_SUBSYSTEM_DECLARATION;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSensorType, 1)
  XII_ENUM_CONSTANTS(xiiSensorType::RGBCamera, xiiSensorType::InfraredCamera, xiiSensorType::DepthCamera, xiiSensorType::LiDAR)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSensorShutterType, 1)
  XII_ENUM_CONSTANTS(xiiSensorShutterType::Global, xiiSensorShutterType::Rolling)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSensorNoiseModel, 1)
  XII_ENUM_CONSTANTS(xiiSensorNoiseModel::None, xiiSensorNoiseModel::Gaussian, xiiSensorNoiseModel::PhotonShotAndRead)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSensorProfile, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSensorProfile>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Type", xiiSensorType, m_Type),
    XII_ENUM_MEMBER_PROPERTY("Shutter", xiiSensorShutterType, m_Shutter),
    XII_ENUM_MEMBER_PROPERTY("NoiseModel", xiiSensorNoiseModel, m_NoiseModel),
    XII_MEMBER_PROPERTY("ResolutionX", m_uiResolutionX)->AddAttributes(new xiiClampValueAttribute(1U, 32768U)),
    XII_MEMBER_PROPERTY("ResolutionY", m_uiResolutionY)->AddAttributes(new xiiClampValueAttribute(1U, 32768U)),
    XII_MEMBER_PROPERTY("FocalLengthXPixels", m_fFocalLengthXPixels)->AddAttributes(new xiiClampValueAttribute(0.001f, xiiVariant()), new xiiSuffixAttribute(" px")),
    XII_MEMBER_PROPERTY("FocalLengthYPixels", m_fFocalLengthYPixels)->AddAttributes(new xiiClampValueAttribute(0.001f, xiiVariant()), new xiiSuffixAttribute(" px")),
    XII_MEMBER_PROPERTY("PrincipalPointXPixels", m_fPrincipalPointXPixels)->AddAttributes(new xiiSuffixAttribute(" px")),
    XII_MEMBER_PROPERTY("PrincipalPointYPixels", m_fPrincipalPointYPixels)->AddAttributes(new xiiSuffixAttribute(" px")),
    XII_MEMBER_PROPERTY("NearPlaneMeters", m_fNearPlaneMeters)->AddAttributes(new xiiClampValueAttribute(0.0001f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("FarPlaneMeters", m_fFarPlaneMeters)->AddAttributes(new xiiClampValueAttribute(0.0001f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("ExposureSeconds", m_fExposureSeconds)->AddAttributes(new xiiClampValueAttribute(0.000001f, xiiVariant()), new xiiSuffixAttribute(" s")),
    XII_MEMBER_PROPERTY("RollingShutterSeconds", m_fRollingShutterSeconds)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" s")),
    XII_MEMBER_PROPERTY("WavelengthNanometers", m_fWavelengthNanometers)->AddAttributes(new xiiClampValueAttribute(1.0f, xiiVariant()), new xiiSuffixAttribute(" nm")),
    XII_MEMBER_PROPERTY("SpectralSensitivity", m_vSpectralSensitivity),
    XII_MEMBER_PROPERTY("QuantumEfficiency", m_fQuantumEfficiency)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("RadianceToElectrons", m_fRadianceToElectrons)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("AnalogGain", m_fAnalogGain)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("ReadNoiseElectrons", m_fReadNoiseElectrons)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("ShotNoiseScale", m_fShotNoiseScale)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("SaturationElectrons", m_fSaturationElectrons)->AddAttributes(new xiiClampValueAttribute(1.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("DepthNoiseStandardDeviationMeters", m_fDepthNoiseStandardDeviationMeters)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiSuffixAttribute(" m")),
    XII_MEMBER_PROPERTY("DepthNoiseScalePerMeter", m_fDepthNoiseScalePerMeter)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("OutputBitDepth", m_uiOutputBitDepth)->AddAttributes(new xiiClampValueAttribute(1U, 32U)),
    XII_MEMBER_PROPERTY("NoiseSeed", m_uiNoiseSeed),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSensorProfileHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSensorProfileHandle>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Index", m_uiIndex),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSensorProfileRegistryStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSensorProfileRegistryStats>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ProfileCount", m_uiProfileCount),
    XII_MEMBER_PROPERTY("RGBProfileCount", m_uiRGBProfileCount),
    XII_MEMBER_PROPERTY("InfraredProfileCount", m_uiInfraredProfileCount),
    XII_MEMBER_PROPERTY("DepthProfileCount", m_uiDepthProfileCount),
    XII_MEMBER_PROPERTY("LiDARProfileCount", m_uiLiDARProfileCount),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

bool xiiSensorRenderingManager::IsInitialized()
{
  return s_pState != nullptr;
}

bool xiiSensorRenderingManager::IsValidProfile(const xiiSensorProfile& profile)
{
  return profile.m_Type.GetValue() < xiiSensorType::ENUM_COUNT &&
    profile.m_Shutter.GetValue() < xiiSensorShutterType::ENUM_COUNT &&
    profile.m_NoiseModel.GetValue() < xiiSensorNoiseModel::ENUM_COUNT &&
    profile.m_uiResolutionX > 0U && profile.m_uiResolutionX <= 32768U &&
    profile.m_uiResolutionY > 0U && profile.m_uiResolutionY <= 32768U &&
    xiiMath::IsFinite(profile.m_fFocalLengthXPixels) && profile.m_fFocalLengthXPixels > 0.0f &&
    xiiMath::IsFinite(profile.m_fFocalLengthYPixels) && profile.m_fFocalLengthYPixels > 0.0f &&
    xiiMath::IsFinite(profile.m_fPrincipalPointXPixels) && xiiMath::IsFinite(profile.m_fPrincipalPointYPixels) &&
    xiiMath::IsFinite(profile.m_fNearPlaneMeters) && profile.m_fNearPlaneMeters > 0.0f &&
    xiiMath::IsFinite(profile.m_fFarPlaneMeters) && profile.m_fFarPlaneMeters > profile.m_fNearPlaneMeters &&
    xiiMath::IsFinite(profile.m_fExposureSeconds) && profile.m_fExposureSeconds > 0.0f &&
    xiiMath::IsFinite(profile.m_fRollingShutterSeconds) && profile.m_fRollingShutterSeconds >= 0.0f &&
    xiiMath::IsFinite(profile.m_fWavelengthNanometers) && profile.m_fWavelengthNanometers > 0.0f &&
    profile.m_vSpectralSensitivity.IsValid() && profile.m_vSpectralSensitivity.x >= 0.0f &&
    profile.m_vSpectralSensitivity.y >= 0.0f && profile.m_vSpectralSensitivity.z >= 0.0f &&
    profile.m_vSpectralSensitivity.GetLengthSquared() > 0.0f &&
    xiiMath::IsFinite(profile.m_fQuantumEfficiency) && profile.m_fQuantumEfficiency >= 0.0f && profile.m_fQuantumEfficiency <= 1.0f &&
    xiiMath::IsFinite(profile.m_fRadianceToElectrons) && profile.m_fRadianceToElectrons >= 0.0f &&
    xiiMath::IsFinite(profile.m_fAnalogGain) && profile.m_fAnalogGain >= 0.0f &&
    xiiMath::IsFinite(profile.m_fReadNoiseElectrons) && profile.m_fReadNoiseElectrons >= 0.0f &&
    xiiMath::IsFinite(profile.m_fShotNoiseScale) && profile.m_fShotNoiseScale >= 0.0f &&
    xiiMath::IsFinite(profile.m_fSaturationElectrons) && profile.m_fSaturationElectrons > 0.0f &&
    xiiMath::IsFinite(profile.m_fDepthNoiseStandardDeviationMeters) && profile.m_fDepthNoiseStandardDeviationMeters >= 0.0f &&
    xiiMath::IsFinite(profile.m_fDepthNoiseScalePerMeter) && profile.m_fDepthNoiseScalePerMeter >= 0.0f &&
    profile.m_uiOutputBitDepth > 0U && profile.m_uiOutputBitDepth <= 32U &&
    (profile.m_Shutter != xiiSensorShutterType::Global || profile.m_fRollingShutterSeconds == 0.0f);
}

xiiResult xiiSensorRenderingManager::AcquireProfile(const xiiSensorProfile& profile, xiiSensorProfileHandle& out_handle)
{
  out_handle = {};
  if (s_pState == nullptr || !IsValidProfile(profile))
    return XII_FAILURE;

  XII_LOCK(s_pState->m_Mutex);
  for (xiiUInt32 i = 0U; i < s_pState->m_Profiles.GetCount(); ++i)
  {
    if (IsEqual(s_pState->m_Profiles[i], profile))
    {
      out_handle.m_uiIndex = i;
      return XII_SUCCESS;
    }
  }

  s_pState->m_Profiles.PushBack(profile);
  out_handle.m_uiIndex = s_pState->m_Profiles.GetCount() - 1U;
  return XII_SUCCESS;
}

xiiSensorProfileHandle xiiSensorRenderingManager::GetDefaultProfileHandle()
{
  return GetDefaultProfileHandle(xiiSensorType::RGBCamera);
}

xiiSensorProfileHandle xiiSensorRenderingManager::GetDefaultProfileHandle(xiiSensorType::Enum type)
{
  if (s_pState == nullptr)
    return {};

  XII_LOCK(s_pState->m_Mutex);
  return type < xiiSensorType::ENUM_COUNT ? s_pState->m_hDefaultProfiles[type] : xiiSensorProfileHandle{};
}

xiiResult xiiSensorRenderingManager::GetProfile(xiiSensorProfileHandle handle, xiiSensorProfile& out_profile)
{
  if (s_pState == nullptr || !handle.IsValid())
    return XII_FAILURE;

  XII_LOCK(s_pState->m_Mutex);
  if (handle.m_uiIndex >= s_pState->m_Profiles.GetCount())
    return XII_FAILURE;

  out_profile = s_pState->m_Profiles[handle.m_uiIndex];
  return XII_SUCCESS;
}

xiiSensorProfileRegistryStats xiiSensorRenderingManager::GetRegistryStats()
{
  xiiSensorProfileRegistryStats stats;
  if (s_pState == nullptr)
    return stats;

  XII_LOCK(s_pState->m_Mutex);
  stats.m_uiProfileCount = s_pState->m_Profiles.GetCount();
  for (const xiiSensorProfile& profile : s_pState->m_Profiles)
  {
    switch (profile.m_Type.GetValue())
    {
      case xiiSensorType::RGBCamera:
        ++stats.m_uiRGBProfileCount;
        break;
      case xiiSensorType::InfraredCamera:
        ++stats.m_uiInfraredProfileCount;
        break;
      case xiiSensorType::DepthCamera:
        ++stats.m_uiDepthProfileCount;
        break;
      case xiiSensorType::LiDAR:
        ++stats.m_uiLiDARProfileCount;
        break;
      default:
        break;
    }
  }
  return stats;
}

void xiiSensorRenderingManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Sensor rendering manager started twice.");
  s_pState = XII_DEFAULT_NEW(State);

  xiiSensorProfile profile;
  AcquireProfile(profile, s_pState->m_hDefaultProfiles[xiiSensorType::RGBCamera]).AssertSuccess("Failed to register the default RGB sensor profile.");

  profile.m_Type                  = xiiSensorType::InfraredCamera;
  profile.m_fWavelengthNanometers = 850.0f;
  profile.m_vSpectralSensitivity  = xiiVec3(0.02f, 0.18f, 0.80f);
  AcquireProfile(profile, s_pState->m_hDefaultProfiles[xiiSensorType::InfraredCamera]).AssertSuccess("Failed to register the default infrared sensor profile.");

  profile.m_Type                  = xiiSensorType::DepthCamera;
  profile.m_NoiseModel            = xiiSensorNoiseModel::Gaussian;
  profile.m_fWavelengthNanometers = 940.0f;
  profile.m_vSpectralSensitivity  = xiiVec3(0.01f, 0.09f, 0.90f);
  AcquireProfile(profile, s_pState->m_hDefaultProfiles[xiiSensorType::DepthCamera]).AssertSuccess("Failed to register the default depth sensor profile.");

  profile.m_Type                               = xiiSensorType::LiDAR;
  profile.m_fWavelengthNanometers              = 905.0f;
  profile.m_vSpectralSensitivity               = xiiVec3(0.01f, 0.12f, 0.87f);
  profile.m_fFarPlaneMeters                    = 250.0f;
  profile.m_fDepthNoiseStandardDeviationMeters = 0.01f;
  profile.m_fDepthNoiseScalePerMeter           = 0.0005f;
  AcquireProfile(profile, s_pState->m_hDefaultProfiles[xiiSensorType::LiDAR]).AssertSuccess("Failed to register the default LiDAR sensor profile.");
}

void xiiSensorRenderingManager::Shutdown()
{
  s_pState.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_SensorRendering);
