/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/DepthSensorPass.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/InfraredSensorPass.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/LidarSensorPass.h>
#include <GraphicsCore/Pipeline/Passes/Sensors/RgbSensorPass.h>
#include <GraphicsCore/Pipeline/RenderPipelineComposition.h>
#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

XII_CREATE_SIMPLE_TEST(Pipeline, NativeSensorSchemas)
{
  XII_TEST_BOOL(!xiiGetStaticRTTI<xiiSensorCapture>()->GetAllocator()->CanAllocate());
  const auto check = [](xiiSensorCapture& sensor, xiiUInt32 expected, bool radiance, bool surfaces) {
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    sensor.GetPins(pins);
    XII_TEST_INT(pins.GetCount(), expected);
    bool hasRadiance = false, hasNormal = false, hasAlbedo = false;
    for (const auto& pin : pins)
    {
      hasRadiance |= pin.m_sName == "Radiance";
      hasNormal |= pin.m_sName == "Normal";
      hasAlbedo |= pin.m_sName == "Albedo";
    }
    XII_TEST_BOOL(hasRadiance == radiance && hasNormal == surfaces && hasAlbedo == surfaces);
    XII_TEST_RESULT(sensor.InitializePinBindings(pins));
    XII_TEST_BOOL(!pins[sensor.m_InputSceneDepth.GetBindingIndex()].m_bOptional);
    XII_TEST_BOOL(pins[sensor.m_InputVelocity.GetBindingIndex()].m_bOptional && pins[sensor.m_InputPerFrameGlobal.GetBindingIndex()].m_bOptional);
    xiiVariantDictionary properties;
    XII_TEST_RESULT(sensor.GetProperties(properties));
    XII_TEST_RESULT(sensor.Configure(properties));
    auto calibration = properties.GetValue("Calibration")->Get<xiiVariantDictionary>();
    XII_TEST_BOOL(!calibration.Contains("Type"));
    calibration.Insert("ResolutionX", 0U);
    properties.Insert("Calibration", calibration);
    XII_TEST_BOOL(sensor.Configure(properties).Failed());
    sensor.m_bUseViewProfile = true;
    pins.Clear();
    sensor.GetPins(pins);
    XII_TEST_RESULT(sensor.InitializePinBindings(pins));
    XII_TEST_BOOL(pins[sensor.m_InputSceneDepth.GetBindingIndex()].m_bOptional);
    XII_TEST_BOOL(sensor.GetExtractionRequirements().IsNoFlagSet() && sensor.GetRuntimeFeatures().IsNoFlagSet());
  };
  xiiDepthSensorPass depth;
  check(depth, 8, false, false);
  xiiRgbSensorPass rgb;
  check(rgb, 9, true, false);
  xiiInfraredSensorPass infrared;
  check(infrared, 9, true, false);
  xiiLidarSensorPass lidar;
  check(lidar, 10, false, true);
}

XII_CREATE_SIMPLE_TEST(Pipeline, SensorCalibrationReflectionRoundtrip)
{
  xiiSensorProfile source;
  source.m_Type                   = xiiSensorType::LiDAR;
  source.m_Shutter                = xiiSensorShutterType::Rolling;
  source.m_fRollingShutterSeconds = 0.01f;
  source.m_uiResolutionX          = 31;
  source.m_uiResolutionY          = 17;
  source.m_fFocalLengthXPixels    = 43.0f;
  source.m_vRadialDistortion      = xiiVec3(0.01f, 0.02f, 0.03f);
  source.m_uiNoiseSeed            = 47;
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  xiiReflectionSerializer::WriteObjectToBinary(writer, xiiGetStaticRTTI<xiiSensorProfile>(), &source);
  xiiMemoryStreamReader reader(&storage);
  const xiiRTTI*        type   = nullptr;
  void*                 loaded = xiiReflectionSerializer::ReadObjectFromBinary(reader, type);
  if (!XII_TEST_BOOL(loaded && type == xiiGetStaticRTTI<xiiSensorProfile>()))
    return;
  XII_SCOPE_EXIT(type->GetAllocator()->Deallocate(loaded));
  const auto& profile = *static_cast<xiiSensorProfile*>(loaded);
  XII_TEST_BOOL(type->GetParentType() == xiiGetStaticRTTI<xiiSensorCalibration>());
  XII_TEST_BOOL(profile.m_Type == xiiSensorType::LiDAR && profile.m_Shutter == xiiSensorShutterType::Rolling);
  XII_TEST_INT(profile.m_uiResolutionX, 31);
  XII_TEST_INT(profile.m_uiResolutionY, 17);
  XII_TEST_FLOAT(profile.m_fFocalLengthXPixels, 43, 0.00001f);
  XII_TEST_BOOL(profile.m_vRadialDistortion == source.m_vRadialDistortion);
  XII_TEST_INT(profile.m_uiNoiseSeed, 47);
  XII_TEST_RESULT(xiiSensorRenderingManager::IsValidProfile(profile) ? XII_SUCCESS : XII_FAILURE);
}
