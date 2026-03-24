#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>

#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>

/// \brief This is the base class for types that handle rendering of different object types.
///
/// E.g. There are different renderers for meshes, particle effects, light sources, etc.
class XII_GRAPHICSCORE_DLL xiiRenderer : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderer, xiiReflectedClass);

public:
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
  {
    XII_IGNORE_UNUSED(out_types);
  }

  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& out_types) const
  {
    XII_IGNORE_UNUSED(out_types);
  }

  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& out_categories) const
  {
    XII_IGNORE_UNUSED(out_categories);
  }

  virtual void RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const = 0;
};
