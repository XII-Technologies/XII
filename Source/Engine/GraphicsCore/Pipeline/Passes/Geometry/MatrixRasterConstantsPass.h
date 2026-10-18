/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Captures explicit matrix raster constants without a live camera or world. Geometry/material residency remains device-owned.
/// This producer has no temporal history; motion rasterization requires its own explicit history.
class XII_GRAPHICSCORE_DLL xiiMatrixRasterConstantsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMatrixRasterConstantsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineBufferOutputPin m_Constants{false, xiiGALBindFlags::UniformBuffer};
  xiiMat4                          m_ViewProjection = xiiMat4::MakeIdentity(), m_PreviousViewProjection = xiiMat4::MakeIdentity();
  xiiUInt32                        m_uiWidth = 1, m_uiHeight = 1;
  xiiUInt32                        m_uiVertexStride    = sizeof(xiiMeshPackedVertex);
  xiiUInt32                        m_uiNormalOffset    = offsetof(xiiMeshPackedVertex, m_vNormal);
  xiiUInt32                        m_uiTangentOffset   = offsetof(xiiMeshPackedVertex, m_vTangent);
  xiiUInt32                        m_uiTexCoordOffset  = offsetof(xiiMeshPackedVertex, m_vTexCoord0);
  xiiUInt32                        m_uiMeshGroupCountX = 1;
  xiiUInt32                        m_uiMeshGroupCountY = 1;
  xiiResult                        Configure(const xiiVariantDictionary& properties) override;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
