/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Meshes/MeshSkinning.h>

namespace
{
  bool IsAffine(const xiiMat4& matrix)
  {
    return matrix.IsValid() && matrix.GetRow(3) == xiiVec4(0, 0, 0, 1);
  }
} // namespace

xiiResult xiiMeshSkinning::ValidatePalette(xiiArrayPtr<const xiiMat4> palette)
{
  for (const auto& matrix : palette)
    if (!IsAffine(matrix))
      return XII_FAILURE;
  return XII_SUCCESS;
}

xiiResult xiiMeshSkinning::TransformPosition(const xiiVec3& position, const xiiVec4U16& indices,
                                             const xiiColorLinearUB& weights, xiiArrayPtr<const xiiMat4> palette, xiiVec3& out_position)
{
  if (!position.IsValid())
    return XII_FAILURE;
  const xiiUInt16 joints[]    = {indices.x, indices.y, indices.z, indices.w};
  const xiiUInt8  influence[] = {weights.r, weights.g, weights.b, weights.a};
  const xiiUInt32 totalWeight = xiiUInt32(weights.r) + weights.g + weights.b + weights.a;
  xiiVec3         result      = position;
  if (!palette.IsEmpty() && totalWeight != 0)
  {
    result.SetZero();
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      if (influence[i] == 0)
        continue;
      if (joints[i] >= palette.GetCount() || !IsAffine(palette[joints[i]]))
        return XII_FAILURE;
      result += palette[joints[i]].TransformPosition(position) * (float(influence[i]) / float(totalWeight));
    }
  }
  if (!result.IsValid())
    return XII_FAILURE;
  out_position = result;
  return XII_SUCCESS;
}

xiiResult xiiMeshSkinning::ComputeBounds(const xiiBoundingBoxSphere& bindBounds,
                                         xiiArrayPtr<const xiiMat4> palette, xiiBoundingBoxSphere& out_bounds)
{
  if (!bindBounds.IsValid() || !xiiMath::IsFinite(bindBounds.m_fSphereRadius))
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(ValidatePalette(palette));
  if (palette.IsEmpty())
  {
    out_bounds = bindBounds;
    return XII_SUCCESS;
  }
  auto box = bindBounds.GetBox();
  if (!box.IsValid())
    return XII_FAILURE;
  for (const auto& matrix : palette)
  {
    auto transformed = bindBounds.GetBox();
    transformed.TransformFromOrigin(matrix);
    if (!transformed.IsValid())
      return XII_FAILURE;
    box.ExpandToInclude(transformed);
  }
  const auto bounds = xiiBoundingBoxSphere::MakeFromBox(box);
  if (!bounds.IsValid() || !xiiMath::IsFinite(bounds.m_fSphereRadius))
    return XII_FAILURE;
  out_bounds = bounds;
  return XII_SUCCESS;
}
