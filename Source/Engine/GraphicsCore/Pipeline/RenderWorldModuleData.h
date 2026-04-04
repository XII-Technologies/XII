#pragma once

#include <GraphicsCore/Pipeline/RenderGraph.h>

/// \brief Frame setup pass data.
struct XII_GRAPHICSCORE_DLL xiiFrameSetupPassData
{
  xiiUInt32 m_uiTimestampSlot = 0U;
};

struct XII_GRAPHICSCORE_DLL xiiDynamicResPassData
{
  xiiRGBufferHandle m_hTimingInput;
  xiiRGBufferHandle m_hVelocityInput;
  xiiRGBufferHandle m_hResolutionOutput;
};

struct XII_GRAPHICSCORE_DLL xiiAOData
{
  xiiRGTextureHandle m_hDepth, m_hNR, m_hHiZ, m_hRawAO, m_hStableAO, m_hVelocity;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U, m_uiSlices = 3U, m_uiSteps = 4U;
  float              fRadius = 0.6f;
};

struct XII_GRAPHICSCORE_DLL xiiASPassData
{
  bool bHasRT = false;
};

struct XII_GRAPHICSCORE_DLL xiiAtmCompData
{
  xiiRGTextureHandle m_hDepth, m_hTransmittance, m_hMultiScatter, m_hDirectLight, m_hSky;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U;
  float              fSunSolidAngle = 6.8e-5f;
};

struct XII_GRAPHICSCORE_DLL xiiAtmLUTData
{
  xiiRGTextureHandle m_hTransmittanceLUT;
  xiiRGTextureHandle m_hMultiScatterLUT;
};

struct XII_GRAPHICSCORE_DLL xiiBloomData
{
  xiiRGTextureHandle m_hInput, m_hBloomOutput;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U, m_uiMips = 7U;
  float              fThreshold = 1.0f, fKnee = 0.5f, fIntensity = 0.04f;
};

struct XII_GRAPHICSCORE_DLL xiiClusterData
{
  xiiRGTextureHandle m_hDepth;
  xiiRGBufferHandle  m_hClusterDescs;
  xiiRGBufferHandle  m_hLightGrid;
  xiiRGBufferHandle  m_hLightIndex;
  xiiUInt32          cX = 16U, cY = 8U, cZ = 24U;
};

struct XII_GRAPHICSCORE_DLL xiiColorGradData
{
  xiiRGTextureHandle m_hLDR, m_hLUT, m_hGraded;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U;
  float              fVignette = 0.3f, fGrain = 0.02f, fSat = 1.0f, fContrast = 1.0f;
};

struct XII_GRAPHICSCORE_DLL xiiContactShadowData
{
  xiiRGTextureHandle m_hDepth;
  xiiRGTextureHandle m_hContactShadow;
  xiiUInt32          m_uiWidth = 1U, m_uiHeight = 1U, m_uiSteps = 16U;
  float              fMaxDist = 0.5f;
};

struct XII_GRAPHICSCORE_DLL xiiDecalPassData
{
  xiiRGTextureHandle m_hDepth;
  xiiRGBufferHandle  m_hDecalTileList;
  xiiRGTextureHandle m_hGBufAlbedo;
  xiiRGTextureHandle m_hGBufNormal;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U, m_uiTileSize = 8U;
};

struct XII_GRAPHICSCORE_DLL xiiDirShadowData
{
  xiiRGBufferHandle  m_hCascadeMatrices;
  xiiRGTextureHandle m_hShadowAtlas;
  xiiUInt32          m_uiAtlasSize    = 4096U;
  xiiUInt32          m_uiCascadeCount = 4U;
};

struct XII_GRAPHICSCORE_DLL xiiDrawCmdBuildData
{
  xiiRGBufferHandle m_hSurviving;
  xiiRGBufferHandle m_hLOD;
  xiiRGBufferHandle m_hDrawArgs;
  xiiRGBufferHandle m_hDrawCount;
  xiiUInt32         m_uiInstanceCount = 0U;
};

struct XII_GRAPHICSCORE_DLL xiiEmissiveData
{
  xiiRGTextureHandle m_hDepth, m_hDrawArgs, m_hEmissive;
  xiiRGBufferHandle  m_hArgs;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U;
};

struct XII_GRAPHICSCORE_DLL xiiExposureData
{
  xiiRGTextureHandle m_hHDRScene;
  xiiRGBufferHandle  m_hHistogram;
  xiiRGBufferHandle  m_hExposure;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U;
};

struct XII_GRAPHICSCORE_DLL xiiFroxelData
{
  xiiRGBufferHandle  m_hFroxelMeta;
  xiiRGTextureHandle m_hFroxelScattering;
  xiiUInt32          cX = 160U, cY = 90U, cZ = 64U;
};

struct XII_GRAPHICSCORE_DLL xiiFrustumCullPassData
{
  xiiRGBufferHandle m_hBounds;
  xiiRGBufferHandle m_hLOD;
  xiiRGBufferHandle m_hVisible;
  xiiUInt32         m_uiInstanceCount = 0U;
};

struct XII_GRAPHICSCORE_DLL xiiGBufferData
{
  xiiRGTextureHandle m_hDepth;
  xiiRGBufferHandle  m_hDrawArgs;
  xiiRGTextureHandle m_hAlbedo, m_hNormal, m_hMaterial, m_hEmissive;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U;
};

struct XII_GRAPHICSCORE_DLL xiiHiZBuildData
{
  xiiRGTextureHandle m_hOccluderDepth;
  xiiRGTextureHandle m_hHiZPyramid;
  xiiUInt32          m_uiWidth = 1U, m_uiHeight = 1U, m_uiMips = 1U;
};

struct XII_GRAPHICSCORE_DLL xiim_HiZOcclData
{
  xiiRGBufferHandle  m_hCandidates;
  xiiRGBufferHandle  m_hBounds;
  xiiRGTextureHandle m_hHiZ;
  xiiRGBufferHandle  m_hSurviving;
  xiiUInt32          m_uiInstanceCount = 0U;
};

struct XII_GRAPHICSCORE_DLL xiiInstanceTransformPassData
{
  xiiRGBufferHandle m_hSceneTransforms;
  xiiRGBufferHandle m_hWorldMatrices;
  xiiRGBufferHandle m_hBounds;
  xiiUInt32         m_uiInstanceCount = 0U;
};

struct XII_GRAPHICSCORE_DLL xiiLightingData
{
  xiiRGTextureHandle m_hAlbedo, m_hNormal, m_hMaterial, m_hDepth, m_hAO;
  xiiRGTextureHandle m_hDirShadow, m_hContactShadow, m_hRTShadow;
  xiiRGTextureHandle m_hRTRefl, m_hSSR, m_hRTGI;
  xiiRGBufferHandle  m_hLightGrid, m_hLightIndex, m_hCameraBuffer;
  xiiRGTextureHandle m_hDirectLight, m_hIndirectLight;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U;
  bool               bRTShadows = true, bContactShadows = true, bRTRefl = true, bSSR = true;
};

struct XII_GRAPHICSCORE_DLL xiiLocalShadowData
{
  xiiRGTextureHandle m_hShadowAtlas;
  xiiRGBufferHandle  m_hAtlasDescs;
  xiiUInt32          m_uiAtlasSize = 4096U;
};

struct XII_GRAPHICSCORE_DLL xiiLodPassData
{
  xiiRGBufferHandle m_hBounds;
  xiiRGBufferHandle m_hLODOutput;
  xiiUInt32         m_uiInstanceCount = 0U;
};

struct XII_GRAPHICSCORE_DLL xiiMainDepthData
{
  xiiRGBufferHandle  m_hDrawArgs;
  xiiRGBufferHandle  m_hDrawCount;
  xiiRGTextureHandle m_hSceneDepth;
  xiiUInt32          m_uiWidth = 1U, m_uiHeight = 1U;
};

struct XII_GRAPHICSCORE_DLL xiiMotionVecData
{
  xiiRGTextureHandle m_hDepth;
  xiiRGTextureHandle m_hVelocity;
  xiiUInt32          m_uiWidth = 1U, m_uiHeight = 1U;
};

struct XII_GRAPHICSCORE_DLL xiiNRPrepassData
{
  xiiRGTextureHandle m_hDepth;
  xiiRGTextureHandle m_hNormalRoughness;
  xiiUInt32          m_uiWidth = 1U, m_uiHeight = 1U;
};

struct XII_GRAPHICSCORE_DLL xiiOccluderDepthData
{
  xiiRGTextureHandle m_hDepth;
  xiiUInt32          m_uiWidth = 1U, m_uiHeight = 1U;
};

struct XII_GRAPHICSCORE_DLL xiiOpaqueCompData
{
  xiiRGTextureHandle m_hDirect, m_hIndirect, m_hEmissive, m_hVolumetric, m_hSky, m_hHDRScene;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U;
};

struct XII_GRAPHICSCORE_DLL xiiParticleData
{
  xiiRGTextureHandle m_hDepth, m_hHDRScene;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U, m_uiMaxParticles = 1u << 20U;
};

struct XII_GRAPHICSCORE_DLL xiiPresentData
{
  xiiRGTextureHandle m_hFinalColor;
  bool               bVSync = true;
};

struct XII_GRAPHICSCORE_DLL xiiReadbackData
{
  bool      bDoReadback = false;
  xiiUInt32 m_uiSlot    = 0U;
};

struct XII_GRAPHICSCORE_DLL xiiRTGIData
{
  xiiRGTextureHandle m_hDepth, m_hNR, m_hVelocity, m_hRawGI, m_hFinalGI;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U, m_uiRPP = 1U, m_uiCandidates = 8U;
};

struct XII_GRAPHICSCORE_DLL xiiRTReflData
{
  xiiRGTextureHandle m_hDepth, m_hNR, m_hVelocity, m_hHDRScene, m_hSSR, m_hRawRefl, m_hFinalRefl;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U, m_uiMaxRays = 2U;
  float              fMaxRoughness = 0.4f;
};

struct XII_GRAPHICSCORE_DLL xiiRTShadowData
{
  xiiRGTextureHandle m_hDepth, m_hNR, m_hVelocity, m_hRawMask, m_hFinalMask;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U, m_uiRPP = 1U, m_uiHistory = 16U;
  float              fLightRadius = 0.05f;
};

struct XII_GRAPHICSCORE_DLL xiiShadowCascadeData
{
  xiiRGBufferHandle m_hCascadeMatrices;
  xiiUInt32         m_uiCascadeCount = 0U;
};

struct XII_GRAPHICSCORE_DLL xiiShadowCullData
{
  xiiRGBufferHandle m_hCascadeMatrices;
  xiiRGBufferHandle m_hInstanceBounds;
  xiiRGBufferHandle m_hShadowCasters;
  xiiUInt32         m_uiCascadeCount = 0U;
};

struct XII_GRAPHICSCORE_DLL xiiSharpenData
{
  xiiRGTextureHandle m_hInput, m_hOutput;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U;
  float              fStrength = 0.5f;
};

struct XII_GRAPHICSCORE_DLL xiiSkinPassData
{
  xiiRGBufferHandle m_hSkinInput;
  xiiRGBufferHandle m_hBonePalette;
  xiiRGBufferHandle m_hMorphWeights;
  xiiRGBufferHandle m_hSkinnedOutput;
  xiiUInt32         m_uiVertexCount = 0U;
};

struct XII_GRAPHICSCORE_DLL xiiSSRData
{
  xiiRGTextureHandle m_hDepth, m_hNR, m_hHiZ, m_hSSR;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U, m_uiMaxSteps = 64U;
  float              fRoughThresh = 0.5f, fThickness = 0.05f;
};

struct XII_GRAPHICSCORE_DLL xiiTAAData
{
  xiiRGTextureHandle m_hCurrent, m_hHistory, m_hVelocity, m_hDepth, m_hResolved;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U;
  float              fAlpha = 0.1f, fSharpness = 0.25f;
};

struct XII_GRAPHICSCORE_DLL xiiTonemapData
{
  xiiRGTextureHandle m_hHDR, m_hBloom, m_hExposure, m_hLDR;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U;
  xiiUInt8           m_uiOperator = 0U;
  float              fBias        = 0.0f;
};

struct XII_GRAPHICSCORE_DLL xiiTransparentData
{
  xiiRGTextureHandle m_hDepth, m_hHDRScene;
  xiiRGBufferHandle  m_hDrawArgs;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U;
  bool               bOIT = true;
};

struct XII_GRAPHICSCORE_DLL xiim_UICompData
{
  xiiRGTextureHandle m_hScene, m_hUI, m_hFinal;
  xiiUInt32          m_uiW = 1U, m_uiH = 1U;
};

struct XII_GRAPHICSCORE_DLL xiiUploadPassData
{
  xiiRGBufferHandle m_hCameraBuffer;
  xiiRGBufferHandle m_hLightBuffer;
  xiiRGBufferHandle m_hGlobalBuffer;
};

struct XII_GRAPHICSCORE_DLL xiiUpscaleData
{
  xiiRGTextureHandle m_hTAAResolved, m_hUpscaled;
  xiiUInt32          m_uiSrcW = 1U, m_uiSrcH = 1U, m_uiDstW = 1U, m_uiDstH = 1U;
  float              fSharpening = 0.4f;
};

struct XII_GRAPHICSCORE_DLL xiiVolIntData
{
  xiiRGTextureHandle m_hFroxelScattering, m_hDepth, m_hVolumetricOut;
  xiiUInt32          cX = 160U, cY = 90U, cZ = 64U;
};
