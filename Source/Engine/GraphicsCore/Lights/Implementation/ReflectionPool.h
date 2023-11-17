#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/Declarations.h>

class xiiGALTextureHandle;
class xiiGALBufferHandle;
class xiiView;
class xiiWorld;
class xiiComponent;
struct xiiRenderWorldExtractionEvent;
struct xiiRenderWorldRenderEvent;
struct xiiMsgExtractRenderData;
struct xiiReflectionProbeDesc;
class xiiReflectionProbeRenderData;
using xiiReflectionProbeId = xiiGenericId<24, 8>;
class xiiReflectionProbeComponentBase;
class xiiSkyLightComponent;

class XII_GRAPHICSCORE_DLL xiiReflectionPool
{
public:
  //Probes
  static xiiReflectionProbeId RegisterReflectionProbe(const xiiWorld* pWorld, const xiiReflectionProbeDesc& desc, const xiiReflectionProbeComponentBase* pComponent);
  static void                 DeregisterReflectionProbe(const xiiWorld* pWorld, xiiReflectionProbeId id);
  static void                 UpdateReflectionProbe(const xiiWorld* pWorld, xiiReflectionProbeId id, const xiiReflectionProbeDesc& desc, const xiiReflectionProbeComponentBase* pComponent);
  static void                 ExtractReflectionProbe(const xiiComponent* pComponent, xiiMsgExtractRenderData& ref_msg, xiiReflectionProbeRenderData* pRenderData, const xiiWorld* pWorld, xiiReflectionProbeId id, float fPriority);

  // SkyLight
  static xiiReflectionProbeId RegisterSkyLight(const xiiWorld* pWorld, xiiReflectionProbeDesc& ref_desc, const xiiSkyLightComponent* pComponent);
  static void                 DeregisterSkyLight(const xiiWorld* pWorld, xiiReflectionProbeId id);
  static void                 UpdateSkyLight(const xiiWorld* pWorld, xiiReflectionProbeId id, const xiiReflectionProbeDesc& desc, const xiiSkyLightComponent* pComponent);


  static void SetConstantSkyIrradiance(const xiiWorld* pWorld, const xiiAmbientCube<xiiColor>& skyIrradiance);
  static void ResetConstantSkyIrradiance(const xiiWorld* pWorld);

  static xiiUInt32           GetReflectionCubeMapSize();
  static xiiGALTextureHandle GetReflectionSpecularTexture(xiiUInt32 uiWorldIndex, xiiEnum<xiiCameraUsageHint> cameraUsageHint);
  static xiiGALTextureHandle GetSkyIrradianceTexture();

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, ReflectionPool);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnExtractionEvent(const xiiRenderWorldExtractionEvent& e);
  static void OnRenderEvent(const xiiRenderWorldRenderEvent& e);

  struct Data;
  static Data* s_pData;
};
