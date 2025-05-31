#pragma once

#include <GraphicsCore/Declarations.h>

class xiiDirectionalLightComponent;
class xiiPointLightComponent;
class xiiSpotLightComponent;
class xiiView;
struct xiiRenderWorldExtractionEvent;
struct xiiRenderWorldRenderEvent;

class XII_GRAPHICSCORE_DLL xiiShadowPool
{
public:
  static xiiUInt32 AddDirectionalLight(const xiiDirectionalLightComponent* pDirLight, const xiiView* pReferenceView);
  static xiiUInt32 AddPointLight(const xiiPointLightComponent* pPointLight, float fScreenSpaceSize, const xiiView* pReferenceView);
  static xiiUInt32 AddSpotLight(const xiiSpotLightComponent* pSpotLight, float fScreenSpaceSize, const xiiView* pReferenceView);

  static xiiSharedPtr<xiiGALTexture> GetShadowAtlasTexture();
  static xiiSharedPtr<xiiGALBuffer>  GetShadowDataBuffer();

  /// \brief All exclude tags on this white list are copied from the reference views to the shadow views.
  static void AddExcludeTagToWhiteList(const xiiTag& tag);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, ShadowPool);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnExtractionEvent(const xiiRenderWorldExtractionEvent& e);
  static void OnRenderEvent(const xiiRenderWorldRenderEvent& e);

  struct Data;
  static Data* s_pData;
};
