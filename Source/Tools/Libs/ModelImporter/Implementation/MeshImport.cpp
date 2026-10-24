/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ModelImporter/ModelImporterPCH.h>

#include <Foundation/Containers/HashSet.h>
#include <ModelImporter/Implementation/MeshImportInternal.h>
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <meshoptimizer/meshoptimizer.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiModelVertexColorConversion, 1)
  XII_ENUM_CONSTANT(xiiModelVertexColorConversion::None),
  XII_ENUM_CONSTANT(xiiModelVertexColorConversion::LinearToSRGB),
  XII_ENUM_CONSTANT(xiiModelVertexColorConversion::SRGBToLinear)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

namespace
{
  using namespace xiiMeshImportInternal;

  struct PendingNode
  {
    XII_DECLARE_POD_TYPE();
    const aiNode* m_pNode;
    xiiMat4       m_ParentTransform;
  };

  xiiVec3 ConvertVector(const aiVector3D& value) { return xiiVec3(value.x, value.y, value.z); }

  xiiModelMaterialTexture ExtractCommonUVMapping(xiiModelMeshMaterial& material)
  {
    if (material.m_Textures.IsEmpty())
      return {};
    const auto mapping = material.m_Textures[0];
    for (const auto& texture : material.m_Textures)
      if (texture.m_uiUVChannel != mapping.m_uiUVChannel || texture.m_vUVOffset != mapping.m_vUVOffset || texture.m_vUVScale != mapping.m_vUVScale || texture.m_fUVRotation != mapping.m_fUVRotation)
        return {}; // Independent mappings remain visible to material conversion diagnostics.
    for (auto& texture : material.m_Textures)
    {
      texture.m_uiUVChannel = 0;
      texture.m_vUVOffset.SetZero();
      texture.m_vUVScale.Set(1);
      texture.m_fUVRotation = 0;
    }
    return mapping;
  }

  void BakeUVMapping(const xiiModelMaterialTexture& mapping, xiiArrayPtr<xiiMeshPackedVertex> vertices)
  {
    const auto  angle  = xiiAngle::MakeFromRadian(mapping.m_fUVRotation);
    const float cosine = xiiMath::Cos(angle);
    const float sine   = xiiMath::Sin(angle);
    for (auto& vertex : vertices)
    {
      const auto uv       = (mapping.m_uiUVChannel == 1 ? vertex.m_vTexCoord1 : vertex.m_vTexCoord0).CompMul(mapping.m_vUVScale);
      vertex.m_vTexCoord0 = mapping.m_vUVOffset + xiiVec2(cosine * uv.x - sine * uv.y, sine * uv.x + cosine * uv.y);
    }
  }

  xiiResult ReadVertices(const aiMesh& mesh, const xiiMat4& transform, const xiiModelMeshImportOptions& options, xiiDynamicArray<xiiMeshPackedVertex>& vertices, xiiStringBuilder* error)
  {
    if (mesh.mNumVertices == 0 || mesh.mVertices == nullptr || mesh.mNormals == nullptr || mesh.mNumVertices > xiiMath::MaxValue<xiiUInt32>() / sizeof(xiiMeshPackedVertex))
      return Fail(error, "Source mesh has missing or oversized vertex data.");
    for (xiiUInt32 i = 2; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i)
      if (mesh.HasTextureCoords(i))
        xiiLog::Warning("Mesh '{}' has UV set {}; the native mesh retains UV sets 0 and 1.", mesh.mName.C_Str(), i);
    for (xiiUInt32 i = 1; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i)
      if (mesh.HasVertexColors(i))
        xiiLog::Warning("Mesh '{}' has color set {}; the native mesh retains color set 0.", mesh.mName.C_Str(), i);
    for (xiiUInt32 i = 0; i < 2; ++i)
      if (mesh.HasTextureCoords(i) && mesh.mNumUVComponents[i] > 2)
        return Fail(error, "Native mesh UV sets must have at most two components.");

    xiiMat3 normalTransform = transform.GetRotationalPart();
    if (normalTransform.Invert(0).Failed())
      return Fail(error, "Source mesh transform is singular.");
    normalTransform.Transpose();
    vertices.SetCountUninitialized(mesh.mNumVertices);
    for (xiiUInt32 i = 0; i < mesh.mNumVertices; ++i)
    {
      xiiMeshPackedVertex vertex{};
      vertex.m_vPosition = transform.TransformPosition(ConvertVector(mesh.mVertices[i]));
      vertex.m_vNormal   = normalTransform * ConvertVector(mesh.mNormals[i]);
      if (!vertex.m_vPosition.IsValid() || !vertex.m_vNormal.IsValid() || vertex.m_vNormal.NormalizeIfNotZero().Failed())
        return Fail(error, "Source mesh contains invalid positions or normals.");
      xiiVec3 tangent = vertex.m_vNormal.GetOrthogonalVector();
      float   sign    = 1;
      if (mesh.HasTangentsAndBitangents())
      {
        tangent = transform.TransformDirection(ConvertVector(mesh.mTangents[i]));
        tangent -= vertex.m_vNormal * tangent.Dot(vertex.m_vNormal);
        const auto bitangent = transform.TransformDirection(ConvertVector(mesh.mBitangents[i]));
        if (!bitangent.IsValid())
          return Fail(error, "Source mesh contains invalid bitangents.");
        sign = vertex.m_vNormal.CrossRH(tangent).Dot(bitangent) < 0 ? -1.0f : 1.0f;
      }
      if (!tangent.IsValid() || tangent.NormalizeIfNotZero().Failed())
        return Fail(error, "Source mesh contains invalid tangents.");
      vertex.m_vTangent = xiiVec4(tangent, sign);
      for (xiiUInt32 uv = 0; uv < 2; ++uv)
      {
        if (!mesh.HasTextureCoords(uv))
          continue;
        const auto&   source = mesh.mTextureCoords[uv][i];
        const xiiVec2 value(source.x, source.y);
        if (!value.IsValid())
          return Fail(error, "Source mesh contains invalid UV coordinates.");
        (uv == 0 ? vertex.m_vTexCoord0 : vertex.m_vTexCoord1) = value;
      }
      if (mesh.HasVertexColors(0))
      {
        const auto& source = mesh.mColors[0][i];
        xiiColor    color(source.r, source.g, source.b, source.a);
        if (!color.IsValid())
          return Fail(error, "Source mesh contains invalid vertex colors.");
        xiiVec3 rgb(color.r, color.g, color.b);
        if (options.m_VertexColorConversion == xiiModelVertexColorConversion::LinearToSRGB)
          rgb = xiiColor::LinearToGamma(rgb);
        else if (options.m_VertexColorConversion == xiiModelVertexColorConversion::SRGBToLinear)
          rgb = xiiColor::GammaToLinear(rgb);
        vertex.m_Color0 = xiiColor(rgb.x, rgb.y, rgb.z, color.a);
      }
      vertices[i] = vertex;
    }
    return XII_SUCCESS;
  }

  xiiResult ReadIndices(const aiMesh& mesh, bool mirrored, xiiDynamicArray<xiiUInt32>& indices, xiiStringBuilder* error)
  {
    if (mesh.mNumFaces == 0 || mesh.mFaces == nullptr || mesh.mNumFaces > xiiMath::MaxValue<xiiUInt32>() / (3 * sizeof(xiiUInt32)))
      return Fail(error, "Source mesh has missing or oversized triangle data.");
    indices.SetCountUninitialized(mesh.mNumFaces * 3);
    for (xiiUInt32 i = 0; i < mesh.mNumFaces; ++i)
    {
      const auto& face = mesh.mFaces[i];
      if (face.mNumIndices != 3 || face.mIndices == nullptr)
        return Fail(error, "Mesh import requires triangulated surface geometry.");
      for (xiiUInt32 c = 0; c < 3; ++c)
      {
        const auto index = face.mIndices[mirrored && c != 0 ? 3 - c : c];
        if (index >= mesh.mNumVertices)
          return Fail(error, "Source triangle references an invalid vertex.");
        indices[i * 3 + c] = index;
      }
    }
    return XII_SUCCESS;
  }

  void CompactVertices(xiiDynamicArray<xiiMeshPackedVertex>& vertices, xiiDynamicArray<xiiUInt32>& indices)
  {
    xiiDynamicArray<xiiUInt32> remap;
    remap.SetCountUninitialized(vertices.GetCount());
    const auto                           count = meshopt_generateVertexRemap(remap.GetData(), indices.GetData(), indices.GetCount(), vertices.GetData(), vertices.GetCount(), sizeof(xiiMeshPackedVertex));
    xiiDynamicArray<xiiMeshPackedVertex> compact;
    compact.SetCountUninitialized(static_cast<xiiUInt32>(count));
    meshopt_remapVertexBuffer(compact.GetData(), vertices.GetData(), vertices.GetCount(), sizeof(xiiMeshPackedVertex), remap.GetData());
    meshopt_remapIndexBuffer(indices.GetData(), indices.GetData(), indices.GetCount(), remap.GetData());
    vertices = std::move(compact);
  }

  xiiResult OptimizeMesh(const xiiModelMeshImportOptions& options, xiiDynamicArray<xiiMeshPackedVertex>& vertices, xiiDynamicArray<xiiUInt32>& indices, xiiStringBuilder* error)
  {
    // Source formats such as OBJ may duplicate every face corner. Weld identical attributes
    // before simplifying, otherwise the simplifier sees disconnected triangles and false seams.
    CompactVertices(vertices, indices);
    if (options.m_fSimplification > 0)
    {
      xiiDynamicArray<xiiUInt32> simplified;
      simplified.SetCountUninitialized(indices.GetCount());
      const size_t target = xiiMath::Max<size_t>(3, static_cast<size_t>(indices.GetCount() / 3 * (1.0 - options.m_fSimplification)) * 3);
      size_t       count;
      if (options.m_bAggressiveSimplification && !options.m_bImportSkinning)
        count = meshopt_simplifySloppy(simplified.GetData(), indices.GetData(), indices.GetCount(), &vertices[0].m_vPosition.x, vertices.GetCount(), sizeof(xiiMeshPackedVertex), nullptr, target, options.m_fMaxSimplificationError, nullptr);
      else
      {
        // Continuous attributes contribute to the error metric. Palette indices are
        // categorical: lock triangles crossing a palette boundary instead of treating
        // an average joint index as a meaningful skinning attribute.
        struct Attributes
        {
          XII_DECLARE_POD_TYPE();
          float m_Values[15];
        };
        xiiDynamicArray<Attributes>    attributes;
        xiiDynamicArray<unsigned char> locks;
        attributes.SetCountUninitialized(vertices.GetCount());
        locks.SetCount(vertices.GetCount());
        xiiMemoryUtils::ZeroFill(locks.GetData(), locks.GetCount());
        for (xiiUInt32 i = 0; i < vertices.GetCount(); ++i)
        {
          const auto& v = vertices[i];
          attributes[i] = {{v.m_vNormal.x, v.m_vNormal.y, v.m_vNormal.z,
                            v.m_vTexCoord0.x, v.m_vTexCoord0.y, v.m_vTexCoord1.x, v.m_vTexCoord1.y,
                            v.m_Color0.r / 255.0f, v.m_Color0.g / 255.0f, v.m_Color0.b / 255.0f, v.m_Color0.a / 255.0f,
                            v.m_BoneWeights0.r / 255.0f, v.m_BoneWeights0.g / 255.0f, v.m_BoneWeights0.b / 255.0f, v.m_BoneWeights0.a / 255.0f}};
        }
        if (options.m_bImportSkinning)
        {
          for (xiiUInt32 i = 0; i < indices.GetCount(); i += 3)
          {
            const auto a = indices[i], b = indices[i + 1], c = indices[i + 2];
            if (vertices[a].m_vBoneIndices0 != vertices[b].m_vBoneIndices0 || vertices[a].m_vBoneIndices0 != vertices[c].m_vBoneIndices0)
              locks[a] = locks[b] = locks[c] = meshopt_SimplifyVertex_Lock;
          }
          // All copies of a position must agree on the lock, including UV seams.
          xiiDynamicArray<xiiUInt32> positions;
          positions.SetCountUninitialized(vertices.GetCount());
          meshopt_generatePositionRemap(positions.GetData(), &vertices[0].m_vPosition.x, vertices.GetCount(), sizeof(xiiMeshPackedVertex));
          for (xiiUInt32 i = 0; i < vertices.GetCount(); ++i)
            locks[positions[i]] |= locks[i];
          for (xiiUInt32 i = 0; i < vertices.GetCount(); ++i)
            locks[i] = locks[positions[i]];
        }
        const float weights[] = {0.5f, 0.5f, 0.5f, 1, 1, 1, 1, 0.25f, 0.25f, 0.25f, 0.25f, 1, 1, 1, 1};
        count                 = meshopt_simplifyWithAttributes(simplified.GetData(), indices.GetData(), indices.GetCount(), &vertices[0].m_vPosition.x, vertices.GetCount(), sizeof(xiiMeshPackedVertex), attributes[0].m_Values, sizeof(Attributes), weights, XII_ARRAY_SIZE(weights), locks.GetData(), target, options.m_fMaxSimplificationError, 0, nullptr);
      }
      if (count == 0 || count % 3 != 0)
        return Fail(error, "Mesh simplification could not retain a valid surface.");
      simplified.SetCount(static_cast<xiiUInt32>(count));
      indices = std::move(simplified);
    }
    if (options.m_fSimplification > 0)
      CompactVertices(vertices, indices);
    meshopt_optimizeVertexCache(indices.GetData(), indices.GetData(), indices.GetCount(), vertices.GetCount());
    // Reordering skinned triangles by their bind-pose depth is not representative of
    // animated overdraw. Fetch optimization still benefits both static and skinned meshes.
    if (!options.m_bImportSkinning)
      meshopt_optimizeOverdraw(indices.GetData(), indices.GetData(), indices.GetCount(), &vertices[0].m_vPosition.x, vertices.GetCount(), sizeof(xiiMeshPackedVertex), 1.05f);
    const auto count = meshopt_optimizeVertexFetch(vertices.GetData(), indices.GetData(), indices.GetCount(), vertices.GetData(), vertices.GetCount(), sizeof(xiiMeshPackedVertex));
    vertices.SetCount(static_cast<xiiUInt32>(count));
    return XII_SUCCESS;
  }
} // namespace

xiiResult xiiModelMeshImport::ReadFile(xiiStringView sAbsoluteFile, const xiiModelMeshImportOptions& options, xiiModelMeshImport& out_import, xiiStringBuilder* out_pError)
{
  out_import.m_Mesh.Clear();
  out_import.m_Materials.Clear();
  out_import.m_EmbeddedTextures.Clear();
  if (out_pError != nullptr)
    out_pError->Clear();
  if (sAbsoluteFile.IsEmpty() || !sAbsoluteFile.IsAbsolutePath())
    return Fail(out_pError, "Mesh import requires an absolute source filename.");
  if (options.m_uiGeneratedLODs > 4)
    return Fail(out_pError, "Mesh import supports up to four generated LODs.");
  auto inverseRoot = options.m_RootTransform;
  if (!inverseRoot.IsValid() || inverseRoot.Invert(0).Failed() || !inverseRoot.IsValid() || !xiiMath::IsEqual(options.m_RootTransform.Element(0, 3), 0.0f, 0.00001f) || !xiiMath::IsEqual(options.m_RootTransform.Element(1, 3), 0.0f, 0.00001f) || !xiiMath::IsEqual(options.m_RootTransform.Element(2, 3), 0.0f, 0.00001f) || !xiiMath::IsEqual(options.m_RootTransform.Element(3, 3), 1.0f, 0.00001f) || !xiiMath::IsFinite(options.m_fSimplification) || options.m_fSimplification < 0 || options.m_fSimplification >= 1 || !xiiMath::IsFinite(options.m_fMaxSimplificationError) || options.m_fMaxSimplificationError < 0 || options.m_fMaxSimplificationError > 1 || options.m_VertexColorConversion.GetValue() >= xiiModelVertexColorConversion::ENUM_COUNT)
    return Fail(out_pError, "Mesh import settings contain an invalid transform, color conversion or simplification value.");

  Assimp::Importer importer;
  unsigned int     flags = aiProcess_Triangulate | aiProcess_ValidateDataStructure | aiProcess_GenSmoothNormals | aiProcess_SortByPType;
  importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
  unsigned int removed = 0;
  if (options.m_bRecalculateNormals)
    removed |= aiComponent_NORMALS;
  if (options.m_bRecalculateTangents || options.m_bRecalculateNormals)
    removed |= aiComponent_TANGENTS_AND_BITANGENTS;
  if (removed != 0)
  {
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, removed);
    flags |= aiProcess_RemoveComponent;
  }
  if (options.m_bFlipTextureV)
    flags |= aiProcess_FlipUVs;
  const xiiString source(sAbsoluteFile);
  const aiScene*  scene = importer.ReadFile(source.GetData(), flags);
  if (scene == nullptr || scene->mRootNode == nullptr)
    return Fail(out_pError, importer.GetErrorString());

  xiiModelMeshImport                       result;
  xiiDynamicArray<PendingNode>             pending;
  xiiHashSet<const aiNode*>                visited;
  xiiHashTable<xiiUInt32, xiiUInt32>       materials;
  xiiDynamicArray<xiiModelMaterialTexture> materialMappings;
  xiiDynamicArray<xiiMeshPackedVertex>     vertices, localVertices;
  xiiDynamicArray<xiiUInt32>               indices, localIndices;
  pending.PushBack({scene->mRootNode, options.m_RootTransform});
  while (!pending.IsEmpty())
  {
    const auto entry = pending.PeekBack();
    pending.PopBack();
    const auto* node = entry.m_pNode;
    if (node == nullptr || visited.Insert(node) || (node->mNumChildren != 0 && node->mChildren == nullptr) || (node->mNumMeshes != 0 && node->mMeshes == nullptr))
      return Fail(out_pError, "Source scene has invalid node ownership or connectivity.");
    const auto transform = entry.m_ParentTransform * ConvertMatrix(node->mTransformation);
    auto       inverse   = transform;
    if (!inverse.IsValid() || inverse.Invert(0).Failed() || !inverse.IsValid() || !xiiMath::IsEqual(transform.Element(0, 3), 0.0f, 0.00001f) || !xiiMath::IsEqual(transform.Element(1, 3), 0.0f, 0.00001f) || !xiiMath::IsEqual(transform.Element(2, 3), 0.0f, 0.00001f) || !xiiMath::IsEqual(transform.Element(3, 3), 1.0f, 0.00001f))
      return Fail(out_pError, "Source node transform is non-finite, singular or non-affine.");
    for (xiiUInt32 i = node->mNumChildren; i > 0; --i)
      pending.PushBack({node->mChildren[i - 1], transform});
    for (xiiUInt32 i = 0; i < node->mNumMeshes; ++i)
    {
      const auto index = node->mMeshes[i];
      if (index >= scene->mNumMeshes || scene->mMeshes == nullptr || scene->mMeshes[index] == nullptr)
        return Fail(out_pError, "Source node references an invalid mesh.");
      const auto& mesh = *scene->mMeshes[index];
      if ((mesh.mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0)
        continue;
      if (xiiStringView(node->mName.C_Str()).StartsWith("UCX_") || xiiStringView(mesh.mName.C_Str()).StartsWith("UCX_"))
        continue; // Authoring colliders are imported by SkeletonImport, not rendered as surfaces.
      if (options.m_bImportSkinning && mesh.mNumAnimMeshes != 0)
        return Fail(out_pError, "Animated morph targets are not supported by the native mesh skinning path.");
      if (result.m_Mesh.GetSubMeshes().GetCount() > xiiMath::MaxValue<xiiUInt16>() - 1U - options.m_uiGeneratedLODs)
        return Fail(out_pError, "Source scene exceeds the native mesh section limit.");
      xiiUInt32 material;
      if (!materials.TryGetValue(mesh.mMaterialIndex, material))
      {
        if (mesh.mMaterialIndex >= scene->mNumMaterials || scene->mMaterials == nullptr || scene->mMaterials[mesh.mMaterialIndex] == nullptr || result.m_Materials.GetCount() == xiiMath::MaxValue<xiiUInt16>())
          return Fail(out_pError, "Source mesh references an invalid material or exceeds the native material limit.");
        material                = result.m_Materials.GetCount();
        auto&            output = result.m_Materials.ExpandAndGetRef();
        xiiStringBuilder materialError;
        if (ReadMaterial(*scene, *scene->mMaterials[mesh.mMaterialIndex], output, result.m_EmbeddedTextures, &materialError).Failed())
        {
          const xiiString name(output.m_sName);
          output         = xiiModelMeshMaterial();
          output.m_sName = name;
          xiiLog::Warning("Model '{}', material {} ('{}'): {} Imported geometry with default material properties.", sAbsoluteFile, mesh.mMaterialIndex, name, materialError);
        }
        materialMappings.PushBack(ExtractCommonUVMapping(output));
        materials.Insert(mesh.mMaterialIndex, material);
        result.m_Mesh.AddMaterialSlot("");
      }
      XII_SUCCEED_OR_RETURN(ReadVertices(mesh, transform, options, localVertices, out_pError));
      XII_SUCCEED_OR_RETURN(ReadIndices(mesh, transform.GetRotationalPart().GetDeterminant() < 0, localIndices, out_pError));
      BakeUVMapping(materialMappings[material], localVertices);
      if (options.m_bImportSkinning)
        XII_SUCCEED_OR_RETURN(ReadSkin(mesh, node->mName.C_Str(), transform, options, result.m_Mesh, localVertices, out_pError));
      if (options.m_bRecalculateTangents || options.m_bRecalculateNormals || !mesh.HasTangentsAndBitangents() || materialMappings[material].m_uiUVChannel != 0 || materialMappings[material].m_vUVScale != xiiVec2(1) || materialMappings[material].m_fUVRotation != 0)
        XII_SUCCEED_OR_RETURN(GenerateTangents(localVertices, localIndices, out_pError));
      XII_SUCCEED_OR_RETURN(OptimizeMesh(options, localVertices, localIndices, out_pError));
      if (localVertices.GetCount() > xiiMath::MaxValue<xiiUInt32>() / sizeof(xiiMeshPackedVertex) - vertices.GetCount() || localIndices.GetCount() > xiiMath::MaxValue<xiiUInt32>() / sizeof(xiiUInt32) - indices.GetCount())
        return Fail(out_pError, "Imported mesh exceeds native vertex or index storage capacity.");
      const auto firstVertex    = vertices.GetCount();
      const auto firstPrimitive = indices.GetCount() / 3;
      vertices.PushBackRange(localVertices);
      for (auto vertexIndex : localIndices)
        indices.PushBack(firstVertex + vertexIndex);
      result.m_Mesh.AddSection(localIndices.GetCount() / 3, firstPrimitive, material);
      for (xiiUInt32 lod = 1; lod <= options.m_uiGeneratedLODs; ++lod)
      {
        auto lodVertices             = localVertices;
        auto lodIndices              = localIndices;
        auto lodOptions              = options;
        lodOptions.m_fSimplification = 1.0f - 1.0f / static_cast<float>(1U << lod);
        // LODs preserve appearance and skinning boundaries, including when the base
        // mesh was explicitly imported with aggressive simplification.
        lodOptions.m_bAggressiveSimplification = false;
        XII_SUCCEED_OR_RETURN(OptimizeMesh(lodOptions, lodVertices, lodIndices, out_pError));
        if (lodVertices.GetCount() > xiiMath::MaxValue<xiiUInt32>() / sizeof(xiiMeshPackedVertex) - vertices.GetCount() || lodIndices.GetCount() > xiiMath::MaxValue<xiiUInt32>() / sizeof(xiiUInt32) - indices.GetCount())
          return Fail(out_pError, "Generated LODs exceed native mesh storage capacity.");
        const auto offset = vertices.GetCount();
        result.m_Mesh.AddSection(lodIndices.GetCount() / 3, indices.GetCount() / 3, material, lod);
        vertices.PushBackRange(lodVertices);
        for (auto vertexIndex : lodIndices)
          indices.PushBack(offset + vertexIndex);
      }
    }
  }
  if (vertices.IsEmpty() || indices.IsEmpty())
    return Fail(out_pError, "Source file contains no renderable surface geometry.");
  auto& buffer = result.m_Mesh.GetMeshBufferDescriptor();
  buffer.AddCommonStreams();
  buffer.SetVertexData(xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(vertices.GetData()), vertices.GetCount() * sizeof(xiiMeshPackedVertex)), vertices.GetCount(), sizeof(xiiMeshPackedVertex));
  if (vertices.GetCount() <= xiiMath::MaxValue<xiiUInt16>())
  {
    xiiDynamicArray<xiiUInt16> small;
    small.Reserve(indices.GetCount());
    for (auto value : indices)
      small.PushBack(static_cast<xiiUInt16>(value));
    buffer.SetIndexData(xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(small.GetData()), small.GetCount() * sizeof(xiiUInt16)), small.GetCount(), xiiGALValueType::UInt16);
  }
  else
    buffer.SetIndexData(xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(indices.GetData()), indices.GetCount() * sizeof(xiiUInt32)), indices.GetCount(), xiiGALValueType::UInt32);
  if (options.m_bImportSkinning)
  {
    if (result.m_Mesh.m_Bones.IsEmpty())
      return Fail(out_pError, "Source file contains no skinning data.");
    result.m_Mesh.m_UsageFlags.Remove(xiiMeshResourceUsageFlags::StaticGeometry);
    result.m_Mesh.m_UsageFlags.Add(xiiMeshResourceUsageFlags::Skinned);
  }
  result.m_Mesh.ComputeBounds();
  if (result.m_Mesh.BuildMeshlets().Failed())
    return Fail(out_pError, "Could not compile native meshlets from imported geometry.");
  out_import = std::move(result);
  return XII_SUCCESS;
}
