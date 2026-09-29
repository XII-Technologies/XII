/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/DiscAreaLightComponent.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Components/Lights/RectangleAreaLightComponent.h>
#include <GraphicsCore/Components/Lights/SpotLightComponent.h>
#include <GraphicsCore/Lighting/PhotometricUnits.h>

XII_CREATE_SIMPLE_TEST_GROUP(Lighting);

XII_CREATE_SIMPLE_TEST(Lighting, PhotometricUnits)
{
  constexpr float fTolerance = 0.0001f;
  const float     fFourPi    = 4.0f * xiiMath::Pi<float>();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Flux and intensity")
  {
    XII_TEST_FLOAT(xiiPhotometricUtils::LuminousFluxToIntensity(fFourPi, fFourPi), 1.0f, fTolerance);
    XII_TEST_FLOAT(xiiPhotometricUtils::LuminousIntensityToFlux(1.0f, fFourPi), fFourPi, fTolerance);
    XII_TEST_FLOAT(xiiPhotometricUtils::ConeSolidAngle(xiiAngle::MakeFromDegree(60.0f)), 2.0f * xiiMath::Pi<float>() * (1.0f - xiiMath::Cos(xiiAngle::MakeFromDegree(30.0f))), fTolerance);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Inverse-square illuminance")
  {
    XII_TEST_FLOAT(xiiPhotometricUtils::LuminousIntensityToIlluminance(100.0f, 2.0f), 25.0f, fTolerance);
    XII_TEST_FLOAT(xiiPhotometricUtils::IlluminanceToLuminousIntensity(25.0f, 2.0f), 100.0f, fTolerance);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Lambertian area emitter")
  {
    XII_TEST_FLOAT(xiiPhotometricUtils::LuminanceToLuminousFlux(100.0f, 2.0f), 200.0f * xiiMath::Pi<float>(), fTolerance);
    XII_TEST_FLOAT(xiiPhotometricUtils::LuminousFluxToLuminance(200.0f * xiiMath::Pi<float>(), 2.0f), 100.0f, fTolerance);
    XII_TEST_FLOAT(xiiPhotometricUtils::LuminanceToLuminousIntensity(50.0f, 0.25f), 12.5f, fTolerance);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Canonical conversion")
  {
    XII_TEST_FLOAT(xiiPhotometricUtils::ToLuminousIntensity(fFourPi, xiiPhotometricUnit::Lumen, fFourPi), 1.0f, fTolerance);
    XII_TEST_FLOAT(xiiPhotometricUtils::ToIlluminance(1000.0f, xiiPhotometricUnit::Nit, 0.01f), 10.0f, fTolerance);
    XII_TEST_FLOAT(xiiPhotometricUtils::ToLuminance(100.0f * xiiMath::Pi<float>(), xiiPhotometricUnit::Lumen, 1.0f, 1.0f), 100.0f, fTolerance);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid physical inputs are contained")
  {
    XII_TEST_FLOAT(xiiPhotometricUtils::LuminousFluxToIntensity(-10.0f, fFourPi), 0.0f, 0.0f);
    XII_TEST_FLOAT(xiiPhotometricUtils::LuminousFluxToIntensity(100.0f, 0.0f), 0.0f, 0.0f);
    XII_TEST_FLOAT(xiiPhotometricUtils::LuminousIntensityToLuminance(100.0f, 0.0f), 0.0f, 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Emitter defaults use canonical physical units")
  {
    xiiPointLightComponent         pointLight;
    xiiSpotLightComponent          spotLight;
    xiiDirectionalLightComponent   directionalLight;
    xiiRectangleAreaLightComponent rectangleLight;
    xiiDiscAreaLightComponent      discLight;

    XII_TEST_INT(pointLight.GetIntensityUnit().GetValue(), xiiPhotometricUnit::Candela);
    XII_TEST_INT(spotLight.GetIntensityUnit().GetValue(), xiiPhotometricUnit::Candela);
    XII_TEST_INT(directionalLight.GetIntensityUnit().GetValue(), xiiPhotometricUnit::Lux);
    XII_TEST_INT(rectangleLight.GetIntensityUnit().GetValue(), xiiPhotometricUnit::Nit);
    XII_TEST_INT(discLight.GetIntensityUnit().GetValue(), xiiPhotometricUnit::Nit);
  }
}
