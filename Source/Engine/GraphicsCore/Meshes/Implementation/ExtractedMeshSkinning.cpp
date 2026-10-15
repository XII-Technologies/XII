/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Meshes/Implementation/ExtractedMeshSkinning.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Meshes/MeshSkinning.h>

xiiResult xiiComputeMorphedMeshBounds(const xiiMeshResource& mesh, xiiArrayPtr<const float> weights, xiiBoundingBoxSphere& out_bounds)
{
  if (weights.GetCount() > mesh.GetMorphTargets().GetCount())
    return XII_FAILURE;
  if (weights.IsEmpty())
  {
    out_bounds = mesh.GetBounds();
    return XII_SUCCESS;
  }
  xiiResourceLock<xiiMeshBufferResource> buffer(mesh.GetMeshBuffer(), xiiResourceAcquireMode::PointerOnly);
  if (buffer.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return XII_FAILURE;
  auto center  = mesh.GetBounds().m_vCenter;
  auto extents = mesh.GetBounds().m_vBoxHalfExtents;
  for (xiiUInt32 i = 0; i < weights.GetCount(); ++i)
  {
    if (!xiiMath::IsFinite(weights[i]))
      return XII_FAILURE;
    const auto& target = mesh.GetMorphTargets()[i];
    if (target.m_uiVertexOffset > buffer->GetVertexCount() || target.m_uiVertexCount > buffer->GetVertexCount() - target.m_uiVertexOffset)
      return XII_FAILURE;
    if (weights[i] == 0)
      continue;
    const auto& delta = mesh.GetMorphTargets()[i].m_Bounds;
    if (!delta.IsValid())
      return XII_FAILURE;
    center += weights[i] * delta.m_vCenter;
    extents += xiiMath::Abs(weights[i]) * delta.m_vBoxHalfExtents;
  }
  out_bounds = xiiBoundingBoxSphere::MakeFromBox(xiiBoundingBox::MakeFromCenterAndHalfExtents(center, extents));
  return out_bounds.IsValid() ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiPrepareExtractedMeshSkinning(const xiiMeshResource& mesh, const xiiMeshRenderData& packet, xiiExtractedMeshSkinning& out_skinning)
{
  xiiExtractedMeshSkinning result;
  xiiBoundingBoxSphere     morphed;
  XII_SUCCEED_OR_RETURN(xiiComputeMorphedMeshBounds(mesh, packet.m_MorphWeights, morphed));
  XII_SUCCEED_OR_RETURN(xiiMeshSkinning::ComputeBounds(morphed, packet.m_SkinningMatrices, result.m_Bounds));
  if (!packet.m_SkinningMatrices.IsEmpty())
  {
    xiiResourceLock<xiiMeshBufferResource> buffer(mesh.GetMeshBuffer(), xiiResourceAcquireMode::PointerOnly);
    if (buffer.GetAcquireResult() != xiiResourceAcquireResult::Final)
      return XII_FAILURE;
    const auto& descriptor  = buffer->GetDescriptor();
    result.m_uiVertexStride = descriptor.GetVertexStride();
    bool indicesFound = false, weightsFound = false;
    for (const auto& stream : descriptor.m_VertexStreams)
    {
      if (stream.m_Semantic == xiiMeshVertexSemantic::BoneIndices0 && stream.m_Format == xiiMeshVertexStreamFormat::UShort4)
      {
        result.m_uiBoneIndicesOffset = stream.m_uiOffset;
        indicesFound                 = true;
      }
      if (stream.m_Semantic == xiiMeshVertexSemantic::BoneWeights0 && stream.m_Format == xiiMeshVertexStreamFormat::UByte4Normalized)
      {
        result.m_uiBoneWeightsOffset = stream.m_uiOffset;
        weightsFound                 = true;
      }
    }
    if (!indicesFound || !weightsFound || result.m_uiVertexStride < 8 || (result.m_uiVertexStride & 3) != 0 || result.m_uiBoneIndicesOffset > result.m_uiVertexStride - 8 || (result.m_uiBoneIndicesOffset & 1) != 0 || result.m_uiBoneWeightsOffset > result.m_uiVertexStride - 4)
      return XII_FAILURE;
  }
  out_skinning = result;
  return XII_SUCCESS;
}
