/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalCameraPass.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

/// Captures current-view raster constants. Geometry/material residency remains device-owned.
/// An optional temporal snapshot supplies coherent current/previous matrices and jitter.
class XII_GRAPHICSCORE_DLL xiiSurfaceRasterConstantsPass final : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSurfaceRasterConstantsPass, xiiRenderPipelinePass);

public:
  xiiRenderPipelineDataInputPin    m_TemporalCamera{xiiGetStaticRTTI<xiiTemporalCameraSnapshot>(), true};
  xiiRenderPipelineBufferOutputPin m_Constants{false, xiiGALBindFlags::UniformBuffer};
  xiiUInt32                        m_uiVertexStride    = sizeof(xiiMeshPackedVertex);
  xiiUInt32                        m_uiNormalOffset    = offsetof(xiiMeshPackedVertex, m_vNormal);
  xiiUInt32                        m_uiTangentOffset   = offsetof(xiiMeshPackedVertex, m_vTangent);
  xiiUInt32                        m_uiTexCoordOffset  = offsetof(xiiMeshPackedVertex, m_vTexCoord0);
  xiiUInt32                        m_uiMeshGroupCountX = 1;
  xiiUInt32                        m_uiMeshGroupCountY = 1;
  xiiResult                        Configure(const xiiVariantDictionary& properties) override;
  xiiResult                        Build(xiiRenderPipelinePassContext& context) override;
};
