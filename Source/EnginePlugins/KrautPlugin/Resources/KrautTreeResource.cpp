#include <KrautPlugin/KrautPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <RendererCore/Textures/Texture2DResource.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiKrautTreeResource, 1, xiiRTTIDefaultAllocator<xiiKrautTreeResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiKrautTreeResource);
// clang-format on

xiiKrautTreeResource::xiiKrautTreeResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
  m_Details.m_Bounds.SetInvalid();
}

xiiResourceLoadDesc xiiKrautTreeResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDesc res;
  res.m_State                      = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable    = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire KrautTree
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable    = 0;
    res.m_State                      = xiiResourceState::Unloaded;
  }

  return res;
}

xiiResourceLoadDesc xiiKrautTreeResource::UpdateContent(xiiStreamReader* Stream)
{
  xiiKrautTreeResourceDescriptor desc;
  xiiResourceLoadDesc            res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(desc));
}

void xiiKrautTreeResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // TODO
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiKrautTreeResource, xiiKrautTreeResourceDescriptor)
{
  m_TreeLODs.Clear();
  m_Details = descriptor.m_Details;

  xiiStringBuilder sResName, sResDesc;

  for (xiiUInt32 lodIdx = 0; lodIdx < descriptor.m_Lods.GetCount(); ++lodIdx)
  {
    const auto& lodSrc = descriptor.m_Lods[lodIdx];

    if (lodSrc.m_LodType != xiiKrautLodType::Mesh)
    {
      // ignore impostor LODs
      break;
    }

    auto& lodDst = m_TreeLODs.ExpandAndGetRef();

    lodDst.m_LodType         = lodSrc.m_LodType;
    lodDst.m_fMinLodDistance = lodSrc.m_fMinLodDistance;
    lodDst.m_fMaxLodDistance = lodSrc.m_fMaxLodDistance;

    xiiMeshResourceDescriptor md;
    auto&                     buffer = md.MeshBufferDesc();

    const xiiUInt32 uiNumVertices  = lodSrc.m_Vertices.GetCount();
    const xiiUInt32 uiNumTriangles = lodSrc.m_Triangles.GetCount();
    const xiiUInt32 uiSubMeshes    = lodSrc.m_SubMeshes.GetCount();

    buffer.AddStream(xiiGALVertexAttributeSemantic::Position, xiiGALResourceFormat::XYZFloat);                                                 // 0
    buffer.AddStream(xiiGALVertexAttributeSemantic::TexCoord0, xiiGALResourceFormat::XYFloat);                                                 // 1
    buffer.AddStream(xiiGALVertexAttributeSemantic::TexCoord1, xiiGALResourceFormat::XYFloat);                                                 // 2
    buffer.AddStream(xiiGALVertexAttributeSemantic::Normal, xiiMeshNormalPrecision::ToResourceFormatNormal(xiiMeshNormalPrecision::_16Bit));   // 3
    buffer.AddStream(xiiGALVertexAttributeSemantic::Tangent, xiiMeshNormalPrecision::ToResourceFormatTangent(xiiMeshNormalPrecision::_16Bit)); // 4
    buffer.AddStream(xiiGALVertexAttributeSemantic::Color0, xiiGALResourceFormat::XYZWFloat);                                                  // 5 TODO: better packing
    buffer.AddStream(xiiGALVertexAttributeSemantic::Color1, xiiGALResourceFormat::XYZWFloat);                                                  // 6 TODO: better packing
    buffer.AllocateStreams(uiNumVertices, xiiGALPrimitiveTopology::Triangles, uiNumTriangles);

    for (xiiUInt32 v = 0; v < uiNumVertices; ++v)
    {
      const auto& vtx = lodSrc.m_Vertices[v];

      buffer.SetVertexData<xiiVec3>(0, v, vtx.m_vPosition);
      buffer.SetVertexData<xiiVec2>(1, v, xiiVec2(vtx.m_vTexCoord.x, vtx.m_vTexCoord.y));
      buffer.SetVertexData<xiiVec2>(2, v, xiiVec2(vtx.m_vTexCoord.z, vtx.m_fAmbientOcclusion));
      xiiMeshBufferUtils::EncodeNormal(vtx.m_vNormal, buffer.GetVertexData(3, v), xiiMeshNormalPrecision::_16Bit).IgnoreResult();
      xiiMeshBufferUtils::EncodeTangent(vtx.m_vTangent, 1.0f, buffer.GetVertexData(4, v), xiiMeshNormalPrecision::_16Bit).IgnoreResult();

      xiiColor color;
      color.r = vtx.m_fBendAndFlutterStrength;
      color.g = (float)vtx.m_uiBranchLevel;
      color.b = xiiMath::ColorByteToFloat(vtx.m_uiFlutterPhase);
      color.a = xiiMath::ColorByteToFloat(vtx.m_uiColorVariation);

      buffer.SetVertexData<xiiColor>(5, v, color);

      buffer.SetVertexData<xiiVec4>(6, v, vtx.m_vBendAnchor.GetAsVec4(vtx.m_fAnchorBendStrength));
    }

    for (xiiUInt32 t = 0; t < uiNumTriangles; ++t)
    {
      const auto& tri = lodSrc.m_Triangles[t];

      buffer.SetTriangleIndices(t, tri.m_uiVertexIndex[0], tri.m_uiVertexIndex[1], tri.m_uiVertexIndex[2]);
    }

    for (xiiUInt32 sm = 0; sm < uiSubMeshes; ++sm)
    {
      const auto& subMesh = lodSrc.m_SubMeshes[sm];

      md.AddSubMesh(subMesh.m_uiNumTriangles, subMesh.m_uiFirstTriangle, subMesh.m_uiMaterialIndex);
    }

    md.ComputeBounds();

    for (xiiUInt32 mat = 0; mat < descriptor.m_Materials.GetCount(); ++mat)
    {
      md.SetMaterial(mat, descriptor.m_Materials[mat].m_sMaterial);
    }

    sResName.Format("{0}_{1}_LOD{2}", GetResourceID(), GetCurrentResourceChangeCounter(), lodIdx);

    if (GetResourceDescription().IsEmpty())
    {
      sResDesc = sResName;
    }
    else
    {
      sResDesc.Format("{0}_{1}_LOD{2}", GetResourceDescription(), GetCurrentResourceChangeCounter(), lodIdx);
    }

    lodDst.m_hMesh = xiiResourceManager::GetExistingResource<xiiMeshResource>(sResName);

    if (!lodDst.m_hMesh.IsValid())
    {
      lodDst.m_hMesh = xiiResourceManager::GetOrCreateResource<xiiMeshResource>(sResName, std::move(md), sResDesc);
    }
  }

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

//////////////////////////////////////////////////////////////////////////

void xiiKrautTreeResourceDescriptor::Save(xiiStreamWriter& ref_stream0) const
{
  xiiUInt8 uiVersion = 15;

  ref_stream0 << uiVersion;

  xiiUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  xiiCompressedStreamWriterZstd stream(&ref_stream0, xiiCompressedStreamWriterZstd::Compression::Average);
#else
  xiiStreamWriter& stream = stream0;
#endif

  ref_stream0 << uiCompressionMode;

  const xiiUInt8 uiNumLods = static_cast<xiiUInt8>(m_Lods.GetCount());
  stream << uiNumLods;

  for (xiiUInt8 lodIdx = 0; lodIdx < uiNumLods; ++lodIdx)
  {
    const auto& lod = m_Lods[lodIdx];

    stream << static_cast<xiiUInt8>(lod.m_LodType);
    stream << lod.m_fMinLodDistance;
    stream << lod.m_fMaxLodDistance;
    stream << lod.m_Vertices.GetCount();
    stream << lod.m_Triangles.GetCount();
    stream << lod.m_SubMeshes.GetCount();

    for (const auto& vtx : lod.m_Vertices)
    {
      stream << vtx.m_vPosition;
      stream << vtx.m_vTexCoord;
      stream << vtx.m_vNormal;
      stream << vtx.m_vTangent;
      stream << vtx.m_fAmbientOcclusion;
      stream << vtx.m_uiColorVariation;
      stream << vtx.m_uiBranchLevel;
      stream << vtx.m_vBendAnchor;
      stream << vtx.m_fAnchorBendStrength;
      stream << vtx.m_fBendAndFlutterStrength;
      stream << vtx.m_uiFlutterPhase;
    }

    for (const auto& tri : lod.m_Triangles)
    {
      stream << tri.m_uiVertexIndex[0];
      stream << tri.m_uiVertexIndex[1];
      stream << tri.m_uiVertexIndex[2];
    }

    for (const auto& sm : lod.m_SubMeshes)
    {
      stream << sm.m_uiFirstTriangle;
      stream << sm.m_uiNumTriangles;
      stream << sm.m_uiMaterialIndex;
    }
  }

  const xiiUInt8 uiNumMats = static_cast<xiiUInt8>(m_Materials.GetCount());
  stream << uiNumMats;

  for (const auto& mat : m_Materials)
  {
    stream << static_cast<xiiUInt8>(mat.m_MaterialType);
    stream << mat.m_sMaterial;
    stream << mat.m_VariationColor;
  }

  stream << m_Details.m_Bounds;
  stream << m_Details.m_vLeafCenter;
  stream << m_Details.m_fStaticColliderRadius;
  stream << m_Details.m_sSurfaceResource;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  stream.FinishCompressedStream().IgnoreResult();

  xiiLog::Dev("Compressed Kraut tree data from {0} KB to {1} KB ({2}%%)", xiiArgF((float)stream.GetUncompressedSize() / 1024.0f, 1), xiiArgF((float)stream.GetCompressedSize() / 1024.0f, 1), xiiArgF(100.0f * stream.GetCompressedSize() / stream.GetUncompressedSize(), 1));
#endif
}

xiiResult xiiKrautTreeResourceDescriptor::Load(xiiStreamReader& ref_stream0)
{
  xiiUInt8 uiVersion = 0;

  ref_stream0 >> uiVersion;

  if (uiVersion < 15)
    return XII_FAILURE;

  xiiUInt8 uiCompressionMode = 0;
  ref_stream0 >> uiCompressionMode;

  xiiStreamReader* pCompressor = &ref_stream0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  xiiCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(&ref_stream0);
      pCompressor = &decompressorZstd;
      break;
#else
      xiiLog::Error("Kraut tree is compressed with zstandard, but support for this compressor is not compiled in.");
      return XII_FAILURE;
#endif

    default:
      xiiLog::Error("Kraut tree is compressed with an unknown algorithm.");
      return XII_FAILURE;
  }

  xiiStreamReader& stream = *pCompressor;

  xiiUInt8 uiNumLods = 0;
  stream >> uiNumLods;

  for (xiiUInt8 lodIdx = 0; lodIdx < uiNumLods; ++lodIdx)
  {
    auto& lod = m_Lods.ExpandAndGetRef();

    xiiUInt8 lodType;
    stream >> lodType;
    lod.m_LodType = static_cast<xiiKrautLodType>(lodType);
    stream >> lod.m_fMinLodDistance;
    stream >> lod.m_fMaxLodDistance;

    xiiUInt32 numVertices, numTriangles, numSubMeshes;

    stream >> numVertices;
    stream >> numTriangles;
    stream >> numSubMeshes;

    lod.m_Vertices.SetCountUninitialized(numVertices);
    lod.m_Triangles.SetCountUninitialized(numTriangles);
    lod.m_SubMeshes.SetCount(numSubMeshes); // initialize this one because of the material handle

    for (auto& vtx : lod.m_Vertices)
    {
      stream >> vtx.m_vPosition;
      stream >> vtx.m_vTexCoord;
      stream >> vtx.m_vNormal;
      stream >> vtx.m_vTangent;
      stream >> vtx.m_fAmbientOcclusion;
      stream >> vtx.m_uiColorVariation;
      stream >> vtx.m_uiBranchLevel;
      stream >> vtx.m_vBendAnchor;
      stream >> vtx.m_fAnchorBendStrength;
      stream >> vtx.m_fBendAndFlutterStrength;
      stream >> vtx.m_uiFlutterPhase;
    }

    for (auto& tri : lod.m_Triangles)
    {
      stream >> tri.m_uiVertexIndex[0];
      stream >> tri.m_uiVertexIndex[1];
      stream >> tri.m_uiVertexIndex[2];
    }

    for (auto& sm : lod.m_SubMeshes)
    {
      stream >> sm.m_uiFirstTriangle;
      stream >> sm.m_uiNumTriangles;
      stream >> sm.m_uiMaterialIndex;
    }
  }

  xiiUInt8 uiNumMats = 0;

  stream >> uiNumMats;
  m_Materials.SetCount(uiNumMats);

  for (auto& mat : m_Materials)
  {
    xiiUInt8 matType = 0;
    stream >> matType;
    mat.m_MaterialType = static_cast<xiiKrautMaterialType>(matType);

    if (uiVersion >= 14)
    {
      stream >> mat.m_sMaterial;
    }
    else
    {
      xiiStringBuilder tmp;
      stream >> tmp;
      stream >> tmp;
    }

    stream >> mat.m_VariationColor;
  }

  stream >> m_Details.m_Bounds;
  stream >> m_Details.m_vLeafCenter;
  stream >> m_Details.m_fStaticColliderRadius;
  stream >> m_Details.m_sSurfaceResource;

  if (uiVersion == 13)
  {
    stream >> uiNumMats;

    for (xiiUInt32 i = 0; i < uiNumMats; ++i)
    {
      stream >> m_Materials[i].m_sMaterial;
    }
  }

  return XII_SUCCESS;
}
