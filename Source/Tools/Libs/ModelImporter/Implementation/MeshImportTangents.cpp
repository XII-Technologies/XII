/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ModelImporter/ModelImporterPCH.h>

#include <ModelImporter/Implementation/MeshImportInternal.h>
#include <mikktspace/mikktspace.h>

namespace
{
  struct TangentContext
  {
    xiiArrayPtr<xiiMeshPackedVertex> m_Corners;
    bool                             m_bValid = true;
  };

  xiiMeshPackedVertex& Corner(const SMikkTSpaceContext* context, int face, int vertex)
  {
    return static_cast<TangentContext*>(context->m_pUserData)->m_Corners[face * 3 + vertex];
  }
} // namespace

xiiResult xiiMeshImportInternal::GenerateTangents(xiiDynamicArray<xiiMeshPackedVertex>& vertices, xiiDynamicArray<xiiUInt32>& indices, xiiStringBuilder* error)
{
  if (indices.GetCount() > xiiMath::MaxValue<xiiUInt32>() / sizeof(xiiMeshPackedVertex))
    return Fail(error, "Mesh exceeds tangent generation storage capacity.");

  // MikkTSpace returns a basis per face corner. Expand before writing it so mirrored
  // UVs and tangent discontinuities never overwrite a basis needed by another face.
  // The subsequent full-attribute remap welds only corners with identical skinning,
  // colors, UVs, normals AND the generated tangent/handedness.
  xiiDynamicArray<xiiMeshPackedVertex> corners;
  corners.SetCountUninitialized(indices.GetCount());
  for (xiiUInt32 i = 0; i < indices.GetCount(); ++i)
    corners[i] = vertices[indices[i]];

  TangentContext       data{corners};
  SMikkTSpaceInterface callbacks{};
  callbacks.m_getNumFaces = [](const SMikkTSpaceContext* context) {
    return static_cast<int>(static_cast<TangentContext*>(context->m_pUserData)->m_Corners.GetCount() / 3);
  };
  callbacks.m_getNumVerticesOfFace = [](const SMikkTSpaceContext*, int) {
    return 3;
  };
  callbacks.m_getPosition = [](const SMikkTSpaceContext* context, float output[], int face, int vertex) {
    const auto& value = Corner(context, face, vertex).m_vPosition;
    for (xiiUInt32 i = 0; i < 3; ++i)
      output[i] = value.GetData()[i];
  };
  callbacks.m_getNormal = [](const SMikkTSpaceContext* context, float output[], int face, int vertex) {
    const auto& value = Corner(context, face, vertex).m_vNormal;
    for (xiiUInt32 i = 0; i < 3; ++i)
      output[i] = value.GetData()[i];
  };
  callbacks.m_getTexCoord = [](const SMikkTSpaceContext* context, float output[], int face, int vertex) {
    const auto& value = Corner(context, face, vertex).m_vTexCoord0;
    output[0]         = value.x;
    output[1]         = value.y;
  };
  callbacks.m_setTSpaceBasic = [](const SMikkTSpaceContext* context, const float tangent[], float sign, int face, int vertex) {
    auto&   corner = Corner(context, face, vertex);
    xiiVec3 direction(tangent[0], tangent[1], tangent[2]);
    direction -= corner.m_vNormal * corner.m_vNormal.Dot(direction);
    if (!direction.IsValid() || !xiiMath::IsFinite(sign))
      static_cast<TangentContext*>(context->m_pUserData)->m_bValid = false;
    else
    {
      if (direction.NormalizeIfNotZero().Failed())
      {
        direction = corner.m_vNormal.GetOrthogonalVector();
        direction.Normalize();
      }
      corner.m_vTangent = xiiVec4(direction, sign < 0 ? -1.0f : 1.0f);
    }
  };
  SMikkTSpaceContext context{&callbacks, &data};
  if (!genTangSpaceDefault(&context) || !data.m_bValid)
    return Fail(error, "MikkTSpace could not generate a finite tangent basis for this mesh.");
  vertices = std::move(corners);
  for (xiiUInt32 i = 0; i < indices.GetCount(); ++i)
    indices[i] = i;
  return XII_SUCCESS;
}
