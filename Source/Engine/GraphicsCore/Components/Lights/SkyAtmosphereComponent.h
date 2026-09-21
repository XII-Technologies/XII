/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Lights/LightComponent.h>

using xiiSkyAtmosphereComponentManager = xiiComponentManager<class xiiSkyAtmosphereComponent, xiiBlockStorageType::Compact>;

/// The light scattering technique to use for evaluating light scattering.
struct xiiLightScatteringTechnique
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    EpipolarSampling = 0U, ///< Epipolar sampling based technique with various optimizations, which is the recommended technique for real-time applications.
    BruteForceRayMarching, ///< A high-quality brute-force ray marching technique for every pixel without any optimizations.

    ENUM_COUNT,

    Default = EpipolarSampling
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLightScatteringTechnique);

/// The method to use for processing shadow map cascades when rendering light scattering.
struct xiiLightScatteringCascadeProcessingMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    SinglePass = 0U,    ///< Process all shadow map cascades in a single pass. This is the recommended mode for best performance and quality.
    MultiPass,          ///< Process each shadow map cascade in a separate pass.
    MultiPassInstanced, ///< Process each shadow map cascade in a separate pass with instancing.

    ENUM_COUNT,

    Default = SinglePass
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLightScatteringCascadeProcessingMode);

/// The criterion to use for detecting discontinuities when refining the sample locations for light scattering.
struct xiiLightScatteringRefinementCriterion
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Depth = 0U,         ///< Refine based on depth discontinuities, which can be detected by checking the depth difference between neighboring samples. This is the most effective criterion for improving quality in areas with depth discontinuities, such as near the ground or objects, but it can also increase the number of samples significantly in scenes with a lot of geometry.
    InscatteringChange, ///< Refine based on changes in inscattering, which can be detected by checking the inscattering difference between neighboring samples. This can be more effective for improving quality in areas with smooth depth but rapidly changing inscattering, such as around the sun, but it can also increase the number of samples significantly in scenes with a bright sun or other strong light sources.

    ENUM_COUNT,

    Default = Depth
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLightScatteringRefinementCriterion);

/// The method to use for evaluating extinction when attenuating background.
struct xiiLightScatteringExtinctionEvaluationMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    PerPixel = 0U, ///< Evaluate extinction per pixel using analytic methods by Eric Bruneton, which can be more accurate but also more expensive.
    EpipolarSlice, ///< Evaluate extinction per epipolar slice and perform bilateral filtering in the same manner as for inscattering.

    ENUM_COUNT,

    Default = PerPixel
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLightScatteringExtinctionEvaluationMode);

/// The method to use for evaluating a single light scattering.
struct xiiLightScatteringSingleEvaluationMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None = 0U,   ///< Do not evaluate light scattering. This can be useful for debugging and artistic purposes.
    Integration, ///< Evaluate light scattering by integrating the scattering along the view ray, which is the standard method for evaluating light scattering and can produce high quality results with enough samples.
    LookupTable, ///< Evaluate light scattering by sampling from a precomputed 2D lookup table based on the view zenith angle and the sun zenith angle, which can be much faster than integration but also less accurate and with limited artistic control.

    ENUM_COUNT,

    Default = Integration
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLightScatteringSingleEvaluationMode);

struct xiiLightScatteringHighOrderEvaluationMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None = 0U,      ///< No high-order scattering evaluation. This can be useful for debugging and artistic purposes.
    UnoccludedOnly, ///< Evaluate high-order scattering only for the unoccluded part of the view ray, which can be a good optimization since the occluded part of the view ray usually contributes much less to the final scattering.
    OccludedOnly,   ///< Evaluate high-order scattering only for the occluded part of the view ray, which can be a good optimization in scenes with a lot of occlusion since the unoccluded part of the view ray usually contributes much more to the final scattering.

    ENUM_COUNT,

    Default = UnoccludedOnly
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiLightScatteringHighOrderEvaluationMode);

/// A sky atmosphere component. This represents a sky atmosphere that is rendered in the background of the scene.
class XII_GRAPHICSCORE_DLL xiiSkyAtmosphereRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkyAtmosphereRenderData, xiiRenderData);

public:
  xiiUInt32 m_uiEpipolarSliceCount           = 512U; ///< Total number of epipolar slices. Set this value to (screen width + screen height) / 2 for best quality.
  xiiUInt32 m_uiMaxSamplesInSlice            = 256U; ///< Maximum number of samples in each epipolar slice. Set this value to max(screen width, screen height) / 2 for best quality.
  xiiUInt32 m_uiInitialSampleStepInSlice     = 16U;  ///< Initial ray marching sample spacing on an epipolar slice. Additional samples are added at discontinuities.
  xiiUInt32 m_uiEpipoleSamplingDensityFactor = 2U;   ///< Factor for controlling the density of sampling around the epipole. It is the sample density scale near the epipole where inscattering changes rapidly. Note that sampling near the epipole is very cheap since only a few steps are required to perform ray marching.

  float m_fRefinementThreshold            = 0.03f; ///< Refinement threshold for discontinuity detection. Lower values lead to more samples being added, which can increase quality but also reduce performance.
  bool  m_bShowSampleLocations            = false; ///< Whether to visualize the sample locations as red dots. This can be useful for debugging and finding good values for the other parameters.
  bool  m_bCorrectScatteringAtDepthBreaks = false; ///< Whether to apply a correction to the inscattering calculation at depth discontinuities, which can reduce artifacts when the camera is close to the ground.
  bool  m_bShowDepthBreaks                = false; ///< Whether to visualize depth discontinuities as blue dots. This can be useful for debugging and finding good values for the other parameters.

  bool      m_bShowLightingOnly           = false; ///< Whether to only show the lighting without the sky color. This can be useful for debugging and artistic purposes.
  bool      m_bOptimizeSampleLocations    = true;  ///< Whether to optimize sample locations to avoid oversampling in areas where the inscattering changes slowly. This can improve performance with little impact on quality if the other parameters are set well.
  bool      m_bEnableLightShafts          = true;  ///< Whether to render light shafts (god rays) when the sun is visible or render unshadowed inscattering. This can add a lot to the visual quality of the sky, especially during sunrise and sunset, but it can also reduce performance.
  xiiUInt32 m_uiInscatteringIntegralSteps = 30U;   ///< Number of steps for integrating the inscattering along the view ray. Higher values can improve quality but also reduce performance.

  xiiSizeU32 m_ShadowMapTexelSize                = xiiSizeU32::MakeZero(); ///< The size of the shadow map texel, (1 / width, 1 / height).
  xiiUInt32  m_uiMaxSamplesOnTheRay              = 512U;                   ///< Maximum number of ray marching samples on a single ray. Typically this value should match the maximum shadow map cascade resolution. Using lower value will improve performance but may result in moire patterns. Note that in most typical cases, significantly less samples are actually taken.
  xiiUInt32  m_uiSampleOnTheRayAtDepthBreakCount = 32U;                    ///< The number of ray marching samples on a ray when running scattering correction pass. This value should typically be much lower than the maximum number of samples on a single ray because 1D min-max optimization is not available during the correction pass.

  xiiUInt32 m_uiMinMaxShadowMapResolution = 0U;    ///< This defines the number of samples at the lowest level of the min-max binary tree and should match the maximum cascade shadow map resolution.
  xiiUInt32 m_uiCascadeCount              = 0U;    ///< The number of shadow map cascades.
  xiiUInt32 m_uiFirstCascadeToRayMarch    = 2U;    ///< The first cascade to use for ray marching. Usually the first few cascades are small, and ray marching them is inefficient.
  float     m_fMaxShadowMapStep           = 16.0f; ///< The maximum shadow map step in texels. This can be increased for higher shadow map resolutions.

  bool                                             m_bUse1DMinMaxTree         = true;                                                ///< Whether to use a 1D min-max tree for optimizing ray marching. This can significantly improve performance, especially with high resolution shadow maps, but it can also increase memory usage and cause artifacts if the parameters are not set well.
  bool                                             m_bIs32BitMinMaxMipMap     = false;                                               ///< Whether to use 32-bit or 16-bit unsigned integer for the min-max mip map. This can reduce artifacts when using high precision shadow maps, but it also increases memory usage and may reduce performance on some platforms.
  xiiEnum<xiiLightScatteringTechnique>             m_LightScatteringTechnique = xiiLightScatteringTechnique::EpipolarSampling;       ///< Technique used to evaluate the light scattering.
  xiiEnum<xiiLightScatteringCascadeProcessingMode> m_CascadeProcessingMode    = xiiLightScatteringCascadeProcessingMode::SinglePass; ///< Method used for processing shadow map cascades when rendering light scattering.

  xiiEnum<xiiLightScatteringRefinementCriterion>      m_RefinementCriterion      = xiiLightScatteringRefinementCriterion::InscatteringChange; ///< Criterion used for detecting discontinuities when refining the sample locations for light scattering.
  xiiEnum<xiiLightScatteringSingleEvaluationMode>     m_SingleEvaluationMode     = xiiLightScatteringSingleEvaluationMode::Integration;       ///< Method used for evaluating a single light scattering.
  xiiEnum<xiiLightScatteringHighOrderEvaluationMode>  m_HighOrderEvaluationMode  = xiiLightScatteringHighOrderEvaluationMode::UnoccludedOnly; ///< Method used for evaluating high-order scattering.
  xiiEnum<xiiLightScatteringExtinctionEvaluationMode> m_ExtinctionEvaluationMode = xiiLightScatteringExtinctionEvaluationMode::EpipolarSlice; ///< Method used for evaluating extinction when attenuating background.

  bool  m_bUseOzoneApproximation           = true;  ///< Whether to use an approximation for ozone absorption based on the Brewer-Dobson circulation model, which can be much faster than using a full ozone profile and can produce good results for most cases, but it can also be less accurate in scenes with unusual ozone distributions or when the camera is very close to the ground.
  bool  m_bUseCustomScatteringCoefficients = false; ///< Whether to use custom scattering coefficients instead of the ones derived from the atmospheric parameters. This can be useful for artistic purposes, but it can also produce less physically accurate results.
  float m_fAerosolDensityScale             = 1.0f;  ///< Scale for the aerosol density, which can be used to control the overall amount of aerosols in the atmosphere and can affect the color and brightness of the sky, especially during sunrise and sunset.
  float m_fAerosolAbsorptionScale          = 1.0f;  ///< Scale for the aerosol absorption, which can be used to control the overall amount of light absorbed by aerosols in the atmosphere and can affect the color and brightness of the sky, especially during sunrise and sunset.

  xiiVec4 m_vCustomRelieghBeta     = xiiVec4(5.8E-6f, 13.5E-6f, 33.1E-6f, 0.0f);    ///< Custom Rayleigh scattering beta coefficients, which can be used to control the color of the sky and can produce interesting artistic effects, but it can also produce less physically accurate results.
  xiiVec4 m_vCustomMieBeta         = xiiVec4(2.E-5f, 2.E-5f, 2.E-5f, 0.0f);         ///< Custom Mie scattering beta coefficients, which can be used to control the color of the sky and can produce interesting artistic effects, but it can also produce less physically accurate results.
  xiiVec4 m_vCustomOzoneAbsorption = xiiVec4(0.650f, 1.881f, 0.085f, 0.0f) * 1E-6f; ///< Custom ozone absorption coefficients, which can be used to control the color of the sky and can produce interesting artistic effects, but it can also produce less physically accurate results.

  xiiVec3 m_vPlanetCenter = xiiVec3::MakeZero(); ///< The center of the planet, which is used for calculating the view ray direction and the sun direction in the shader. This should typically be set to the origin of the world or the center of the scene.
  float   m_fPlanetRadius = 6360000.0f;          ///< The radius of the planet in meters, which is used for calculating the view ray direction and the sun direction in the shader. This should typically be set to the average radius of the planet, which is 6360000 meters for Earth, but it can be adjusted for artistic purposes or for rendering other planets.
};
