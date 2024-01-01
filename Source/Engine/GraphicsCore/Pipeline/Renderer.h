#pragma once

#include <GraphicsCore/Pipeline/RenderData.h>

/// \brief This is the base class for types that handle rendering of different object types.
///
/// E.g. there are different renderers for meshes, particle effects, light sources, etc.
class XII_GRAPHICSCORE_DLL xiiRenderer : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderer, xiiReflectedClass);

public:
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const                    = 0;
  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const = 0;

  virtual void UpdateBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) = 0;
  virtual void RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const = 0;
};
