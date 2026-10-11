/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ModelImporter/ModelImporterPCH.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Strings/StringBuilder.h>
#include <ModelImporter/Implementation/SceneImportUtils.h>
#include <ModelImporter/SkeletonImport.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace
{
  using namespace xiiModelImportInternal;

  struct PendingNode
  {
    XII_DECLARE_POD_TYPE();
    const aiNode* m_pNode;
    xiiUInt16     m_uiParent;
  };

  xiiUInt16 FindColliderJoint(xiiStringView name, const xiiHashTable<xiiString, xiiUInt16>& joints)
  {
    if (!name.TrimWordStart("UCX_"))
      return xiiMath::MaxValue<xiiUInt16>();
    // Prefer the longest matching joint name, so joints containing underscores remain unambiguous.
    while (!name.IsEmpty())
    {
      xiiUInt16 index;
      if (joints.TryGetValue(name, index))
        return index;
      const char* separator = name.FindLastSubString("_");
      if (separator == nullptr)
        break;
      name = xiiStringView(name.GetStartPointer(), separator);
    }
    return xiiMath::MaxValue<xiiUInt16>();
  }
} // namespace

xiiResult xiiModelSkeletonImport::ReadFile(xiiStringView sSourceFile, xiiModelSkeletonImport& out_import, xiiStringBuilder* out_pError)
{
  out_import.m_Skeleton.Clear();
  out_import.m_Colliders.Clear();
  if (out_pError != nullptr)
    out_pError->Clear();
  auto fail = [&](xiiStringView message) -> xiiResult {
    if (out_pError != nullptr)
      *out_pError = message;
    return XII_FAILURE;
  };
  if (sSourceFile.IsEmpty() || !sSourceFile.IsAbsolutePath())
    return fail("Skeleton import requires an absolute source filename.");

  Assimp::Importer importer;
  const xiiString  source(sSourceFile);
  const auto*      scene = importer.ReadFile(source.GetData(), aiProcess_Triangulate | aiProcess_ValidateDataStructure);
  if (scene == nullptr || scene->mRootNode == nullptr)
    return fail(importer.GetErrorString());

  xiiModelSkeletonImport             result;
  xiiDynamicArray<PendingNode>       pending;
  xiiDynamicArray<const aiNode*>     nodes;
  xiiHashSet<const aiNode*>          visited;
  xiiHashTable<xiiString, xiiUInt16> joints;
  pending.PushBack({scene->mRootNode, xiiMath::MaxValue<xiiUInt16>()});
  while (!pending.IsEmpty())
  {
    const auto entry = pending.PeekBack();
    pending.PopBack();
    const auto* node = entry.m_pNode;
    if (node == nullptr || visited.Insert(node) || result.m_Skeleton.m_Joints.GetCount() == xiiMath::MaxValue<xiiUInt16>())
      return fail("Source skeleton has invalid ownership or exceeds the native joint limit.");
    xiiTransform pose;
    if (ConvertPose(node->mTransformation, pose).Failed())
      return fail("Source joint pose is non-finite, singular or cannot be represented without shear.");
    if (node->mName.length == 0 || joints.Contains(node->mName.C_Str()))
      return fail("Source skeleton has empty or duplicate joint names.");

    const auto index        = result.m_Skeleton.AddJoint(node->mName.C_Str(), entry.m_uiParent, pose);
    auto&      joint        = result.m_Skeleton.m_Joints[index];
    joint.m_ModelRestPose   = entry.m_uiParent < index ? result.m_Skeleton.m_Joints[entry.m_uiParent].m_ModelRestPose * pose.GetAsMat4() : pose.GetAsMat4();
    joint.m_InverseBindPose = joint.m_ModelRestPose;
    if (!joint.m_ModelRestPose.IsValid() || joint.m_InverseBindPose.Invert(0).Failed() || !joint.m_InverseBindPose.IsValid())
      return fail("Source skeleton has a non-invertible model pose.");
    joints.Insert(node->mName.C_Str(), index);
    nodes.PushBack(node);
    if (node->mNumChildren != 0 && node->mChildren == nullptr)
      return fail("Source skeleton has an invalid child array.");
    for (xiiUInt32 i = node->mNumChildren; i > 0; --i)
      pending.PushBack({node->mChildren[i - 1], index});
  }

  xiiBoundingBox bounds = xiiBoundingBox::MakeInvalid();
  for (xiiUInt32 i = 0; i < nodes.GetCount(); ++i)
  {
    const auto* node = nodes[i];
    bounds.ExpandToInclude(result.m_Skeleton.m_Joints[i].m_ModelRestPose.GetTranslationVector());
    if (node->mNumMeshes != 0 && node->mMeshes == nullptr)
      return fail("Source node has an invalid mesh array.");
    for (xiiUInt32 m = 0; m < node->mNumMeshes; ++m)
    {
      if (node->mMeshes[m] >= scene->mNumMeshes || scene->mMeshes == nullptr || scene->mMeshes[node->mMeshes[m]] == nullptr)
        return fail("Source node references an invalid mesh.");
      const auto* mesh  = scene->mMeshes[node->mMeshes[m]];
      auto        index = FindColliderJoint(mesh->mName.C_Str(), joints);
      if (index == xiiMath::MaxValue<xiiUInt16>())
        index = FindColliderJoint(node->mName.C_Str(), joints);
      if (index == xiiMath::MaxValue<xiiUInt16>())
        continue;
      if (!mesh->HasPositions() || !mesh->HasFaces())
        return fail("Source collider has no triangle geometry.");
      auto& collider          = result.m_Colliders.ExpandAndGetRef();
      collider.m_uiJointIndex = index;
      collider.m_sIdentifier  = mesh->mName.length != 0 ? mesh->mName.C_Str() : node->mName.C_Str();
      const auto transform    = result.m_Skeleton.m_Joints[index].m_InverseBindPose * result.m_Skeleton.m_Joints[i].m_ModelRestPose;
      collider.m_Vertices.Reserve(mesh->mNumVertices);
      for (xiiUInt32 v = 0; v < mesh->mNumVertices; ++v)
      {
        const auto& vertex = mesh->mVertices[v];
        const auto  point  = transform.TransformPosition(xiiVec3(vertex.x, vertex.y, vertex.z));
        if (!point.IsValid())
          return fail("Source collider contains non-finite vertices.");
        collider.m_Vertices.PushBack(point);
      }
      for (xiiUInt32 f = 0; f < mesh->mNumFaces; ++f)
      {
        const auto& face = mesh->mFaces[f];
        if (face.mNumIndices != 3 || face.mIndices == nullptr)
          return fail("Source collider is not a triangle mesh.");
        for (xiiUInt32 k = 0; k < 3; ++k)
        {
          if (face.mIndices[k] >= mesh->mNumVertices)
            return fail("Source collider references an invalid vertex.");
          collider.m_TriangleIndices.PushBack(face.mIndices[k]);
        }
      }
    }
  }
  result.m_Skeleton.m_Bounds = xiiBoundingBoxSphere::MakeFromBox(bounds);
  if (!result.m_Skeleton.m_Bounds.IsValid())
    return fail("Source skeleton exceeds the representable bounds.");
  result.m_Skeleton.ComputeRuntimeHash();
  out_import = std::move(result);
  return XII_SUCCESS;
}
