#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Meshes/MeshBufferUtils.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMeshBufferResource, 1, xiiRTTIDefaultAllocator<xiiMeshBufferResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiMeshBufferResource);
// clang-format on

xiiMeshBufferResourceDescriptor::xiiMeshBufferResourceDescriptor()
{
  m_Topology      = xiiGALPrimitiveTopology::TriangleList;
  m_uiVertexSize  = 0;
  m_uiVertexCount = 0;
}

xiiMeshBufferResourceDescriptor::~xiiMeshBufferResourceDescriptor() = default;

void xiiMeshBufferResourceDescriptor::Clear()
{
  m_Topology             = xiiGALPrimitiveTopology::TriangleList;
  m_uiVertexSize         = 0;
  m_uiVertexCount        = 0;
  m_InputLayout.m_uiHash = 0;
  m_InputLayout.m_VertexStreams.Clear();
  m_VertexStreamData.Clear();
  m_IndexBufferData.Clear();
}

xiiArrayPtr<const xiiUInt8> xiiMeshBufferResourceDescriptor::GetVertexBufferData() const
{
  return m_VertexStreamData.GetArrayPtr();
}

xiiArrayPtr<const xiiUInt8> xiiMeshBufferResourceDescriptor::GetIndexBufferData() const
{
  return m_IndexBufferData.GetArrayPtr();
}

xiiDynamicArray<xiiUInt8, xiiAlignedAllocatorWrapper>& xiiMeshBufferResourceDescriptor::GetVertexBufferData()
{
  return m_VertexStreamData;
}

xiiDynamicArray<xiiUInt8, xiiAlignedAllocatorWrapper>& xiiMeshBufferResourceDescriptor::GetIndexBufferData()
{
  return m_IndexBufferData;
}

xiiUInt32 xiiMeshBufferResourceDescriptor::AddStream(xiiGALInputLayoutSemantic::Enum semantic, xiiGALResourceFormat::Enum format)
{
  XII_ASSERT_DEV(m_VertexStreamData.IsEmpty(), "This function can only be called before 'AllocateStreams' is called");

  for (xiiUInt32 i = 0; i < m_InputLayout.m_VertexStreams.GetCount(); ++i)
  {
    XII_ASSERT_DEV(m_InputLayout.m_VertexStreams[i].m_Semantic != semantic, "The given semantic {0} is already used by a previous stream", semantic);
  }

  xiiVertexStreamInfo si;

  si.m_Semantic      = semantic;
  si.m_Format        = format;
  si.m_uiOffset      = 0;
  si.m_uiElementSize = static_cast<xiiUInt16>(xiiGALTextureUtilities::GetResourceFormatProperties(format).GetElementSize());
  m_uiVertexSize += si.m_uiElementSize;

  XII_ASSERT_DEV(si.m_uiElementSize > 0, "Invalid Element Size. Format not supported?");

  if (!m_InputLayout.m_VertexStreams.IsEmpty())
    si.m_uiOffset = m_InputLayout.m_VertexStreams.PeekBack().m_uiOffset + m_InputLayout.m_VertexStreams.PeekBack().m_uiElementSize;

  m_InputLayout.m_VertexStreams.PushBack(si);

  return m_InputLayout.m_VertexStreams.GetCount() - 1;
}

void xiiMeshBufferResourceDescriptor::AddCommonStreams()
{
  AddStream(xiiGALInputLayoutSemantic::Position, xiiGALResourceFormat::RGB32Float);
  AddStream(xiiGALInputLayoutSemantic::TexCoord0, xiiMeshTexCoordPrecision::ToResourceFormat(xiiMeshTexCoordPrecision::Default));
  AddStream(xiiGALInputLayoutSemantic::Normal, xiiMeshNormalPrecision::ToResourceFormatNormal(xiiMeshNormalPrecision::Default));
  AddStream(xiiGALInputLayoutSemantic::Tangent, xiiMeshNormalPrecision::ToResourceFormatTangent(xiiMeshNormalPrecision::Default));
}

void xiiMeshBufferResourceDescriptor::AllocateStreams(xiiUInt32 uiNumVertices, xiiGALPrimitiveTopology::Enum topology, xiiUInt32 uiNumPrimitives, bool bZeroFill /*= false*/)
{
  XII_ASSERT_DEV(!m_InputLayout.m_VertexStreams.IsEmpty(), "You have to add streams via 'AddStream' before calling this function");

  m_Topology                         = topology;
  m_uiVertexCount                    = uiNumVertices;
  const xiiUInt32 uiVertexStreamSize = m_uiVertexSize * uiNumVertices;

  if (bZeroFill)
  {
    m_VertexStreamData.SetCount(uiVertexStreamSize);
  }
  else
  {
    m_VertexStreamData.SetCountUninitialized(uiVertexStreamSize);
  }

  if (uiNumPrimitives > 0)
  {
    // use an index buffer at all
    xiiUInt32 uiIndexBufferSize = xiiGALPrimitiveTopology::GetIndexCount(topology, uiNumPrimitives);

    if (Uses32BitIndices())
    {
      uiIndexBufferSize *= sizeof(xiiUInt32);
    }
    else
    {
      uiIndexBufferSize *= sizeof(xiiUInt16);
    }

    m_IndexBufferData.SetCountUninitialized(uiIndexBufferSize);
  }
}

void xiiMeshBufferResourceDescriptor::AllocateStreamsFromGeometry(const xiiGeometry& geom, xiiGALPrimitiveTopology::Enum topology)
{
  xiiLogBlock _("Allocate Streams From Geometry");

  XII_ASSERT_DEV(topology == xiiGALPrimitiveTopology::PointList || topology == xiiGALPrimitiveTopology::LineList || topology == xiiGALPrimitiveTopology::TriangleList, "Only three pimitive types are currently supported");

  // Index Buffer Generation
  xiiDynamicArray<xiiUInt32> Indices;

  if (topology == xiiGALPrimitiveTopology::PointList)
  {
    // Leaving indices empty disables indexed rendering.
  }
  else if (topology == xiiGALPrimitiveTopology::LineList)
  {
    Indices.Reserve(geom.GetLines().GetCount() * 2);

    for (xiiUInt32 p = 0; p < geom.GetLines().GetCount(); ++p)
    {
      Indices.PushBack(geom.GetLines()[p].m_uiStartVertex);
      Indices.PushBack(geom.GetLines()[p].m_uiEndVertex);
    }
  }
  else if (topology == xiiGALPrimitiveTopology::TriangleList)
  {
    Indices.Reserve(geom.GetPolygons().GetCount() * 6);

    for (xiiUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      for (xiiUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
      {
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[0]);
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 1]);
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 2]);
      }
    }
  }

  AllocateStreams(geom.GetVertices().GetCount(), topology, Indices.GetCount() / (topology + 1));

  // Fill vertex buffer.
  for (xiiUInt32 s = 0; s < m_InputLayout.m_VertexStreams.GetCount(); ++s)
  {
    const xiiVertexStreamInfo& si = m_InputLayout.m_VertexStreams[s];
    switch (si.m_Semantic)
    {
      case xiiGALInputLayoutSemantic::Position:
      {
        if (si.m_Format == xiiGALResourceFormat::RGB32Float)
        {
          for (xiiUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<xiiVec3>(s, v, geom.GetVertices()[v].m_vPosition);
          }
        }
        else
        {
          xiiLog::Error("Position stream with format '{0}' is not supported.", (int)si.m_Format);
        }
      }
      break;

      case xiiGALInputLayoutSemantic::Normal:
      {
        for (xiiUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
        {
          if (xiiMeshBufferUtils::EncodeNormal(geom.GetVertices()[v].m_vNormal, GetVertexData(s, v), si.m_Format).Failed())
          {
            xiiLog::Error("Normal stream with format '{0}' is not supported.", (int)si.m_Format);
            break;
          }
        }
      }
      break;

      case xiiGALInputLayoutSemantic::Tangent:
      {
        for (xiiUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
        {
          if (xiiMeshBufferUtils::EncodeTangent(geom.GetVertices()[v].m_vTangent, geom.GetVertices()[v].m_fBiTangentSign, GetVertexData(s, v), si.m_Format).Failed())
          {
            xiiLog::Error("Tangent stream with format '{0}' is not supported.", (int)si.m_Format);
            break;
          }
        }
      }
      break;

      case xiiGALInputLayoutSemantic::Color0:
      case xiiGALInputLayoutSemantic::Color1:
      {
        if (si.m_Format == xiiGALResourceFormat::RGBA8UNormalized)
        {
          for (xiiUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<xiiColorLinearUB>(s, v, geom.GetVertices()[v].m_Color);
          }
        }
        else
        {
          xiiLog::Error("Color stream with format '{0}' is not supported.", (int)si.m_Format);
        }
      }
      break;

      case xiiGALInputLayoutSemantic::TexCoord0:
      case xiiGALInputLayoutSemantic::TexCoord1:
      {
        for (xiiUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
        {
          if (xiiMeshBufferUtils::EncodeTexCoord(geom.GetVertices()[v].m_vTexCoord, GetVertexData(s, v), si.m_Format).Failed())
          {
            xiiLog::Error("UV stream with format '{0}' is not supported.", (int)si.m_Format);
            break;
          }
        }
      }
      break;

      case xiiGALInputLayoutSemantic::BoneIndices0:
      {
        // if a bone index array is available, move the custom index into it

        if (si.m_Format == xiiGALResourceFormat::RGBA8UInt)
        {
          for (xiiUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            xiiVec4U16 boneIndices = geom.GetVertices()[v].m_BoneIndices;
            xiiVec4U8  storage(static_cast<xiiUInt8>(boneIndices.x), static_cast<xiiUInt8>(boneIndices.y), static_cast<xiiUInt8>(boneIndices.z), static_cast<xiiUInt8>(boneIndices.w));
            SetVertexData<xiiVec4U8>(s, v, storage);
          }
        }
        else if (si.m_Format == xiiGALResourceFormat::RGBA16UInt)
        {
          for (xiiUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<xiiVec4U16>(s, v, geom.GetVertices()[v].m_BoneIndices);
          }
        }
      }
      break;

      case xiiGALInputLayoutSemantic::BoneWeights0:
      {
        // if a bone weight array is available, set it to fully use the first bone

        if (si.m_Format == xiiGALResourceFormat::RGBA8UNormalized)
        {
          for (xiiUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<xiiColorLinearUB>(s, v, geom.GetVertices()[v].m_BoneWeights);
          }
        }

        if (si.m_Format == xiiGALResourceFormat::RGBA32Float)
        {
          for (xiiUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<xiiVec4>(s, v, xiiColor(geom.GetVertices()[v].m_BoneWeights).GetAsVec4());
          }
        }
      }
      break;

      case xiiGALInputLayoutSemantic::BoneIndices1:
      case xiiGALInputLayoutSemantic::BoneWeights1:
        // Don't error out for these semantics as they may be used by the user (e.g. breakable mesh construction)
        break;

      default:
      {
        xiiLog::Error("Streams semantic '{0}' is not supported.", (int)si.m_Semantic);
      }
      break;
    }
  }

  // Fill index buffer.
  if (topology == xiiGALPrimitiveTopology::PointList)
  {
    for (xiiUInt32 t = 0; t < Indices.GetCount(); t += 1)
    {
      SetPointIndices(t, Indices[t]);
    }
  }
  else if (topology == xiiGALPrimitiveTopology::TriangleList)
  {
    for (xiiUInt32 t = 0; t < Indices.GetCount(); t += 3)
    {
      SetTriangleIndices(t / 3, Indices[t], Indices[t + 1], Indices[t + 2]);
    }
  }
  else if (topology == xiiGALPrimitiveTopology::LineList)
  {
    for (xiiUInt32 t = 0; t < Indices.GetCount(); t += 2)
    {
      SetLineIndices(t / 2, Indices[t], Indices[t + 1]);
    }
  }
}

void xiiMeshBufferResourceDescriptor::SetPointIndices(xiiUInt32 uiPoint, xiiUInt32 uiVertex0)
{
  XII_ASSERT_DEBUG(m_Topology == xiiGALPrimitiveTopology::PointList, "Wrong topology");

  if (Uses32BitIndices())
  {
    xiiUInt32* pIndices = reinterpret_cast<xiiUInt32*>(&m_IndexBufferData[uiPoint * sizeof(xiiUInt32) * 1]);
    pIndices[0]         = uiVertex0;
  }
  else
  {
    xiiUInt16* pIndices = reinterpret_cast<xiiUInt16*>(&m_IndexBufferData[uiPoint * sizeof(xiiUInt16) * 1]);
    pIndices[0]         = static_cast<xiiUInt16>(uiVertex0);
  }
}

void xiiMeshBufferResourceDescriptor::SetLineIndices(xiiUInt32 uiLine, xiiUInt32 uiVertex0, xiiUInt32 uiVertex1)
{
  XII_ASSERT_DEBUG(m_Topology == xiiGALPrimitiveTopology::LineList, "Wrong topology");

  if (Uses32BitIndices())
  {
    xiiUInt32* pIndices = reinterpret_cast<xiiUInt32*>(&m_IndexBufferData[uiLine * sizeof(xiiUInt32) * 2]);
    pIndices[0]         = uiVertex0;
    pIndices[1]         = uiVertex1;
  }
  else
  {
    xiiUInt16* pIndices = reinterpret_cast<xiiUInt16*>(&m_IndexBufferData[uiLine * sizeof(xiiUInt16) * 2]);
    pIndices[0]         = static_cast<xiiUInt16>(uiVertex0);
    pIndices[1]         = static_cast<xiiUInt16>(uiVertex1);
  }
}

void xiiMeshBufferResourceDescriptor::SetTriangleIndices(xiiUInt32 uiTriangle, xiiUInt32 uiVertex0, xiiUInt32 uiVertex1, xiiUInt32 uiVertex2)
{
  XII_ASSERT_DEBUG(m_Topology == xiiGALPrimitiveTopology::TriangleList, "Wrong topology");
  XII_ASSERT_DEBUG(uiVertex0 < m_uiVertexCount && uiVertex1 < m_uiVertexCount && uiVertex2 < m_uiVertexCount, "Vertex indices out of range.");

  if (Uses32BitIndices())
  {
    xiiUInt32* pIndices = reinterpret_cast<xiiUInt32*>(&m_IndexBufferData[uiTriangle * sizeof(xiiUInt32) * 3]);
    pIndices[0]         = uiVertex0;
    pIndices[1]         = uiVertex1;
    pIndices[2]         = uiVertex2;
  }
  else
  {
    xiiUInt16* pIndices = reinterpret_cast<xiiUInt16*>(&m_IndexBufferData[uiTriangle * sizeof(xiiUInt16) * 3]);
    pIndices[0]         = static_cast<xiiUInt16>(uiVertex0);
    pIndices[1]         = static_cast<xiiUInt16>(uiVertex1);
    pIndices[2]         = static_cast<xiiUInt16>(uiVertex2);
  }
}

xiiUInt32 xiiMeshBufferResourceDescriptor::GetPrimitiveCount() const
{
  const xiiUInt32 divider = m_Topology + 1;

  if (!m_IndexBufferData.IsEmpty())
  {
    if (Uses32BitIndices())
      return (m_IndexBufferData.GetCount() / sizeof(xiiUInt32)) / divider;
    else
      return (m_IndexBufferData.GetCount() / sizeof(xiiUInt16)) / divider;
  }
  else
  {
    return m_uiVertexCount / divider;
  }
}

xiiBoundingBoxSphere xiiMeshBufferResourceDescriptor::ComputeBounds() const
{
  xiiBoundingBoxSphere bounds = xiiBoundingBoxSphere::MakeInvalid();

  for (xiiUInt32 i = 0; i < m_InputLayout.m_VertexStreams.GetCount(); ++i)
  {
    if (m_InputLayout.m_VertexStreams[i].m_Semantic == xiiGALInputLayoutSemantic::Position)
    {
      XII_ASSERT_DEBUG(m_InputLayout.m_VertexStreams[i].m_Format == xiiGALResourceFormat::RGB32Float, "Position format is not usable");

      const xiiUInt32 offset = m_InputLayout.m_VertexStreams[i].m_uiOffset;

      if (!m_VertexStreamData.IsEmpty() && m_uiVertexCount > 0)
      {
        bounds = xiiBoundingBoxSphere::MakeFromPoints(reinterpret_cast<const xiiVec3*>(&m_VertexStreamData[offset]), m_uiVertexCount, m_uiVertexSize);
      }

      return bounds;
    }
  }

  if (!bounds.IsValid())
  {
    bounds = xiiBoundingBoxSphere::MakeFromCenterExtents(xiiVec3::MakeZero(), xiiVec3(0.1f), 0.1f);
  }

  return bounds;
}

xiiResult xiiMeshBufferResourceDescriptor::RecomputeNormals()
{
  if (m_Topology != xiiGALPrimitiveTopology::TriangleList)
    return XII_FAILURE; // normals not needed

  const xiiUInt32            uiVertexSize  = m_uiVertexSize;
  const xiiUInt8*            pPositions    = nullptr;
  xiiUInt8*                  pNormals      = nullptr;
  xiiGALResourceFormat::Enum normalsFormat = xiiGALResourceFormat::RGB32Float;

  for (xiiUInt32 i = 0; i < m_InputLayout.m_VertexStreams.GetCount(); ++i)
  {
    if (m_InputLayout.m_VertexStreams[i].m_Semantic == xiiGALInputLayoutSemantic::Position && m_InputLayout.m_VertexStreams[i].m_Format == xiiGALResourceFormat::RGB32Float)
    {
      pPositions = GetVertexData(i, 0).GetPtr();
    }

    if (m_InputLayout.m_VertexStreams[i].m_Semantic == xiiGALInputLayoutSemantic::Normal)
    {
      normalsFormat = m_InputLayout.m_VertexStreams[i].m_Format;
      pNormals      = GetVertexData(i, 0).GetPtr();
    }
  }

  if (pPositions == nullptr || pNormals == nullptr)
    return XII_FAILURE; // there are no normals that could be recomputed

  xiiDynamicArray<xiiVec3> newNormals;
  newNormals.SetCountUninitialized(m_uiVertexCount);

  for (auto& n : newNormals)
  {
    n.SetZero();
  }

  xiiResult res = XII_SUCCESS;

  const xiiUInt16* pIndices16    = reinterpret_cast<const xiiUInt16*>(m_IndexBufferData.GetData());
  const xiiUInt32* pIndices32    = reinterpret_cast<const xiiUInt32*>(m_IndexBufferData.GetData());
  const bool       bUseIndices32 = Uses32BitIndices();

  // Compute unnormalized triangle normals and add them to all vertices.
  // This way large triangles have an higher influence on the vertex normal.
  for (xiiUInt32 triIdx = 0; triIdx < GetPrimitiveCount(); ++triIdx)
  {
    const xiiUInt32 v0 = bUseIndices32 ? pIndices32[triIdx * 3 + 0] : pIndices16[triIdx * 3 + 0];
    const xiiUInt32 v1 = bUseIndices32 ? pIndices32[triIdx * 3 + 1] : pIndices16[triIdx * 3 + 1];
    const xiiUInt32 v2 = bUseIndices32 ? pIndices32[triIdx * 3 + 2] : pIndices16[triIdx * 3 + 2];

    const xiiVec3 p0 = *reinterpret_cast<const xiiVec3*>(pPositions + xiiMath::SafeMultiply64(uiVertexSize, v0));
    const xiiVec3 p1 = *reinterpret_cast<const xiiVec3*>(pPositions + xiiMath::SafeMultiply64(uiVertexSize, v1));
    const xiiVec3 p2 = *reinterpret_cast<const xiiVec3*>(pPositions + xiiMath::SafeMultiply64(uiVertexSize, v2));

    const xiiVec3 d01 = p1 - p0;
    const xiiVec3 d02 = p2 - p0;

    const xiiVec3 triNormal = d01.CrossRH(d02);

    if (triNormal.IsValid())
    {
      newNormals[v0] += triNormal;
      newNormals[v1] += triNormal;
      newNormals[v2] += triNormal;
    }
  }

  for (xiiUInt32 i = 0; i < newNormals.GetCount(); ++i)
  {
    // normalize the new normal
    if (newNormals[i].NormalizeIfNotZero(xiiVec3::MakeAxisX()).Failed())
      res = XII_FAILURE;

    // then encode it in the target format precision and write it back to the buffer
    XII_SUCCEED_OR_RETURN(xiiMeshBufferUtils::EncodeNormal(newNormals[i], xiiByteArrayPtr(pNormals + xiiMath::SafeMultiply64(uiVertexSize, i), sizeof(xiiVec3)), normalsFormat));
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiMeshBufferResource::xiiMeshBufferResource() :
  xiiResource(DoUpdate::OnGraphicsResourceThreads, 1)
{
}

xiiMeshBufferResource::~xiiMeshBufferResource()
{
  XII_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  XII_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
}

xiiResourceLoadDesc xiiMeshBufferResource::UnloadData(Unload WhatToUnload)
{
  if (!m_hVertexBuffer.IsInvalidated())
  {
    xiiGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexBuffer);
    m_hVertexBuffer.Invalidate();
  }

  if (!m_hIndexBuffer.IsInvalidated())
  {
    xiiGALDevice::GetDefaultDevice()->DestroyBuffer(m_hIndexBuffer);
    m_hIndexBuffer.Invalidate();
  }

  m_uiPrimitiveCount = 0;

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDesc xiiMeshBufferResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_REPORT_FAILURE("This resource type does not support loading data from file.");

  return xiiResourceLoadDesc();
}

void xiiMeshBufferResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiMeshBufferResource);
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiMeshBufferResource, xiiMeshBufferResourceDescriptor)
{
  XII_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  XII_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");

  m_InputLayout = descriptor.GetInputLayout();
  m_InputLayout.ComputeHash();

  m_uiPrimitiveCount = descriptor.GetPrimitiveCount();
  m_Topology         = descriptor.GetTopology();

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  m_hVertexBuffer = xiiGALDeviceUtilities::CreateVertexBuffer(pDevice, descriptor.GetVertexDataSize(), descriptor.GetVertexCount(), descriptor.GetVertexBufferData().GetArrayPtr());

  xiiStringBuilder sName;
  sName.SetFormat("{0} Vertex Buffer", GetResourceDescription());
  pDevice->GetBuffer(m_hVertexBuffer)->SetDebugName(sName);

  if (descriptor.HasIndexBuffer())
  {
    const xiiUInt32 uiIndexCount = xiiGALPrimitiveTopology::GetIndexCount(m_Topology, m_uiPrimitiveCount);
    m_hIndexBuffer               = xiiGALDeviceUtilities::CreateIndexBuffer(pDevice, descriptor.Uses32BitIndices() ? xiiGALDeviceUtilities::IndexType::UInt : xiiGALDeviceUtilities::IndexType::UShort, uiIndexCount, descriptor.GetIndexBufferData());

    sName.SetFormat("{0} Index Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);

    // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
    ModifyMemoryUsage().m_uiMemoryGPU = descriptor.GetVertexBufferData().GetCount() + descriptor.GetIndexBufferData().GetCount();
  }
  else
  {
    // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
    ModifyMemoryUsage().m_uiMemoryGPU = descriptor.GetVertexBufferData().GetCount();
  }

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  m_Bounds = descriptor.ComputeBounds();

  return res;
}

void xiiInputLayoutInfo::ComputeHash()
{
  m_uiHash = 0;

  for (const auto& vs : m_VertexStreams)
  {
    m_uiHash += vs.CalculateHash();

    XII_ASSERT_DEBUG(m_uiHash != 0, "Invalid Hash Value");
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_MeshBufferResource);
