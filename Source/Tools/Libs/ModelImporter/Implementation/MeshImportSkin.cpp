/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ModelImporter/ModelImporterPCH.h>

#include <Foundation/Containers/HashSet.h>
#include <ModelImporter/Implementation/MeshImportInternal.h>

namespace
{
  struct Influences
  {
    XII_DECLARE_POD_TYPE();
    xiiUInt16 m_Bones[4]   = {};
    float     m_Weights[4] = {};
    xiiUInt32 m_uiCount    = 0;

    void Add(xiiUInt16 bone, float weight)
    {
      xiiUInt32 position = m_uiCount;
      for (xiiUInt32 i = 0; i < m_uiCount; ++i)
      {
        if (weight > m_Weights[i] || (weight == m_Weights[i] && bone < m_Bones[i]))
        {
          position = i;
          break;
        }
      }
      if (position == 4)
        return;
      m_uiCount = xiiMath::Min(m_uiCount + 1, 4U);
      for (xiiUInt32 i = m_uiCount - 1; i > position; --i)
      {
        m_Bones[i]   = m_Bones[i - 1];
        m_Weights[i] = m_Weights[i - 1];
      }
      m_Bones[position]   = bone;
      m_Weights[position] = weight;
    }
  };

  xiiResult GetBone(xiiStringView name, const xiiMat4& inverseBind, xiiMeshResourceDescriptor& mesh, xiiUInt16& out_index, xiiStringBuilder* error)
  {
    using namespace xiiMeshImportInternal;
    if (name.IsEmpty() || !xiiUnicodeUtils::IsValidUtf8(name.GetStartPointer(), name.GetEndPointer()) || !inverseBind.IsValid())
      return Fail(error, "Source mesh contains an invalid bone name or inverse bind pose.");
    auto inverse = inverseBind;
    if (inverse.Invert(0).Failed() || !inverse.IsValid())
      return Fail(error, "Source mesh contains a singular inverse bind pose.");
    xiiHashedString key;
    key.Assign(name);
    xiiMeshBoneData existing;
    if (mesh.m_Bones.TryGetValue(key, existing))
    {
      if (!existing.m_GlobalInverseRestPoseMatrix.IsEqual(inverseBind, 0.001f))
        return Fail(error, "Mesh instances have conflicting inverse bind poses for the same named bone.");
      out_index = existing.m_uiBoneIndex;
      return XII_SUCCESS;
    }
    if (mesh.m_Bones.GetCount() == xiiMath::MaxValue<xiiUInt16>())
      return Fail(error, "Source mesh exceeds the native bone palette limit.");
    xiiMeshBoneData bone;
    bone.m_uiBoneIndex                 = static_cast<xiiUInt16>(mesh.m_Bones.GetCount());
    bone.m_GlobalInverseRestPoseMatrix = inverseBind;
    out_index                          = bone.m_uiBoneIndex;
    mesh.m_Bones.Insert(key, bone);
    return XII_SUCCESS;
  }
} // namespace

xiiResult xiiMeshImportInternal::ReadSkin(const aiMesh& mesh, xiiStringView sNodeName, const xiiMat4& meshTransform, const xiiModelMeshImportOptions& options, xiiMeshResourceDescriptor& inout_mesh, xiiArrayPtr<xiiMeshPackedVertex> vertices, xiiStringBuilder* error)
{
  auto inverseMesh = meshTransform;
  if (inverseMesh.Invert(0).Failed() || !inverseMesh.IsValid())
    return Fail(error, "Skinned mesh instance has a singular transform.");
  xiiDynamicArray<Influences> influences;
  influences.SetCount(vertices.GetCount());
  for (auto& influence : influences)
    influence = {};
  xiiHashSet<xiiUInt16> usedBones;
  xiiHashSet<xiiUInt32> usedVertices;
  for (xiiUInt32 b = 0; b < mesh.mNumBones; ++b)
  {
    if (mesh.mBones == nullptr || mesh.mBones[b] == nullptr)
      return Fail(error, "Source mesh has an invalid bone array.");
    const auto& bone        = *mesh.mBones[b];
    const auto  inverseBind = ConvertMatrix(bone.mOffsetMatrix) * inverseMesh;
    xiiUInt16   paletteIndex;
    XII_SUCCEED_OR_RETURN(GetBone(bone.mName.C_Str(), inverseBind, inout_mesh, paletteIndex, error));
    if (usedBones.Insert(paletteIndex) || (bone.mNumWeights != 0 && bone.mWeights == nullptr))
      return Fail(error, "Source mesh has duplicate bones or an invalid weight array.");
    usedVertices.Clear();
    for (xiiUInt32 w = 0; w < bone.mNumWeights; ++w)
    {
      const auto& weight = bone.mWeights[w];
      if (weight.mVertexId >= influences.GetCount() || !xiiMath::IsFinite(weight.mWeight) || weight.mWeight < 0 || (!options.m_bNormalizeWeights && weight.mWeight > 1) || usedVertices.Insert(weight.mVertexId))
        return Fail(error, "Source mesh has invalid or duplicate vertex weights.");
      if (weight.mWeight > 0)
      {
        influences[weight.mVertexId].Add(paletteIndex, weight.mWeight);
        const float offset = inverseBind.TransformPosition(vertices[weight.mVertexId].m_vPosition).GetLength();
        if (!xiiMath::IsFinite(offset))
          return Fail(error, "Source bone influence produces a non-finite vertex offset.");
        inout_mesh.m_fMaxBoneVertexOffset = xiiMath::Max(inout_mesh.m_fMaxBoneVertexOffset, offset);
      }
    }
  }
  xiiUInt16 rigidBone = xiiMath::MaxValue<xiiUInt16>();
  for (xiiUInt32 i = 0; i < vertices.GetCount(); ++i)
  {
    auto& influence = influences[i];
    if (influence.m_uiCount == 0)
    {
      if (rigidBone == xiiMath::MaxValue<xiiUInt16>())
        XII_SUCCEED_OR_RETURN(GetBone(sNodeName, inverseMesh, inout_mesh, rigidBone, error));
      influence.Add(rigidBone, 1);
      const float offset = inverseMesh.TransformPosition(vertices[i].m_vPosition).GetLength();
      if (!xiiMath::IsFinite(offset))
        return Fail(error, "Rigid skinning produces a non-finite vertex offset.");
      inout_mesh.m_fMaxBoneVertexOffset = xiiMath::Max(inout_mesh.m_fMaxBoneVertexOffset, offset);
    }
    const float sum = influence.m_Weights[0] + influence.m_Weights[1] + influence.m_Weights[2] + influence.m_Weights[3];
    if (!xiiMath::IsFinite(sum) || sum <= 0)
      return Fail(error, "Source vertex weights cannot be normalized.");
    xiiUInt32 quantizedSum  = 0;
    float     remainders[4] = {};
    auto&     vertex        = vertices[i];
    vertex.m_BoneWeights0   = xiiColorLinearUB(0, 0, 0, 0);
    for (xiiUInt32 c = 0; c < influence.m_uiCount; ++c)
    {
      vertex.m_vBoneIndices0.GetData()[c] = influence.m_Bones[c];
      const float weight                  = influence.m_Weights[c] / (options.m_bNormalizeWeights ? sum : 1.0f) * 255.0f;
      const auto  value                   = static_cast<xiiUInt8>(xiiMath::Clamp(xiiMath::Floor(weight), 0.0f, 255.0f));
      vertex.m_BoneWeights0.GetData()[c]  = value;
      remainders[c]                       = weight - value;
      quantizedSum += value;
    }
    // Largest remainder quantization preserves a normalized sum of exactly 255.
    if (options.m_bNormalizeWeights)
    {
      while (quantizedSum < 255)
      {
        xiiUInt32 best = 0;
        for (xiiUInt32 c = 1; c < influence.m_uiCount; ++c)
          if (remainders[c] > remainders[best])
            best = c;
        ++vertex.m_BoneWeights0.GetData()[best];
        remainders[best] = -1;
        ++quantizedSum;
      }
    }
  }
  return XII_SUCCESS;
}
