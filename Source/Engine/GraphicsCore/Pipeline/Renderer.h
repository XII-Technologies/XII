#pragma once

#include <GraphicsCore/Pipeline/RenderData.h>

/// \brief This is the base class for types that handle rendering of different object types.
///
/// E.g. There are different renderers for meshes, particle effects, light sources, etc.
class XII_GRAPHICSCORE_DLL xiiRenderer : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderer, xiiReflectedClass);

public:
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const = 0;

  virtual void RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const = 0;
};
