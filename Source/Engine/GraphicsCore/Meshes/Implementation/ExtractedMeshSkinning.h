/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Meshes/MeshComponent.h>

struct xiiExtractedMeshSkinning
{
  xiiBoundingBoxSphere m_Bounds;
  xiiUInt32            m_uiVertexStride      = 0;
  xiiUInt32            m_uiBoneIndicesOffset = 0;
  xiiUInt32            m_uiBoneWeightsOffset = 0;
};

xiiResult xiiPrepareExtractedMeshSkinning(const xiiMeshResource& mesh, const xiiMeshRenderData& packet, xiiExtractedMeshSkinning& out_skinning);
xiiResult xiiComputeMorphedMeshBounds(const xiiMeshResource& mesh, xiiArrayPtr<const float> weights, xiiBoundingBoxSphere& out_bounds);
