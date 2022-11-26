#pragma once

#include <RendererCore/Meshes/MeshRenderer.h>

/// \brief Implements rendering of meshes with procedural generated vertex colors
class XII_PROCGENPLUGIN_DLL xiiProcVertexColorRenderer : public xiiMeshRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcVertexColorRenderer, xiiMeshRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiProcVertexColorRenderer);

public:
  xiiProcVertexColorRenderer();
  ~xiiProcVertexColorRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& types) const override;

protected:
  virtual void SetAdditionalData(const xiiRenderViewContext& renderViewContext, const xiiMeshRenderData* pRenderData) const override;
  virtual void FillPerInstanceData(
    xiiArrayPtr<xiiPerInstanceData> instanceData,
    const xiiRenderDataBatch&       batch,
    xiiUInt32                       uiStartIndex,
    xiiUInt32&                      out_uiFilteredCount) const override;
};
