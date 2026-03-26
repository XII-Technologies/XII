#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

/// \brief Registry to get a renderer for a specific render data type. Instances of all renderers are automatically created and registered.
class XII_GRAPHICSCORE_DLL xiiRendererRegistry
{
public:
  XII_FORCE_INLINE static const xiiRenderer* GetRenderer(const xiiRTTI* pRenderDataType)
  {
    if (s_bRendererInstancesDirty)
    {
      CreateRendererInstances();
    }

    xiiUInt32 uiIndex = 0;
    if (s_RenderDataTypeToRendererIndex.TryGetValue(pRenderDataType, uiIndex))
    {
      return s_RendererInstances[uiIndex].Borrow();
    }

    return nullptr;
  }

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RendererRegistry);

  static void PluginEventHandler(const xiiPluginEvent& e);
  static void UpdateRendererTypes();

  static void CreateRendererInstances();
  static void ClearRendererInstances();

  static xiiHybridArray<const xiiRTTI*, 16>         s_RendererTypes;
  static xiiDynamicArray<xiiUniquePtr<xiiRenderer>> s_RendererInstances;
  static xiiHashTable<const xiiRTTI*, xiiUInt32>    s_RenderDataTypeToRendererIndex;
  static bool                                       s_bRendererInstancesDirty;
};
