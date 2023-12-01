#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Meshes/DynamicMeshBufferResource.h>
#include <GraphicsCore/Meshes/MeshBufferUtils.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicMeshBufferResource, 1, xiiRTTIDefaultAllocator<xiiDynamicMeshBufferResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiDynamicMeshBufferResource);
// clang-format on

xiiDynamicMeshBufferResource::xiiDynamicMeshBufferResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiDynamicMeshBufferResource::~xiiDynamicMeshBufferResource()
{
  XII_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  XII_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
  XII_ASSERT_DEBUG(m_hColorBuffer.IsInvalidated(), "Implementation error");
}

xiiResourceLoadDesc xiiDynamicMeshBufferResource::UnloadData(Unload WhatToUnload)
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

  if (!m_hColorBuffer.IsInvalidated())
  {
    xiiGALDevice::GetDefaultDevice()->DestroyBuffer(m_hColorBuffer);
    m_hColorBuffer.Invalidate();
  }

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDesc xiiDynamicMeshBufferResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_REPORT_FAILURE("This resource type does not support loading data from file.");

  return xiiResourceLoadDesc();
}

void xiiDynamicMeshBufferResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiDynamicMeshBufferResource) + m_VertexData.GetHeapMemoryUsage() + m_Index16Data.GetHeapMemoryUsage() + m_Index32Data.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiDynamicMeshBufferResource, xiiDynamicMeshBufferResourceDescriptor)
{
  XII_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  XII_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
  XII_ASSERT_DEBUG(m_hColorBuffer.IsInvalidated(), "Implementation error");

  m_Descriptor = descriptor;

  m_VertexData.SetCountUninitialized(m_Descriptor.m_uiMaxVertices);

  {
    xiiVertexStreamInfo si;
    si.m_uiOffset      = 0;
    si.m_Format        = xiiGALTextureFormat::RGB32Float;
    si.m_Semantic      = xiiGALInputLayoutSemantic::Position;
    si.m_uiElementSize = sizeof(xiiVec3);
    m_InputLayout.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format        = xiiGALTextureFormat::RG32Float;
    si.m_Semantic      = xiiGALInputLayoutSemantic::TexCoord0;
    si.m_uiElementSize = sizeof(xiiVec2);
    m_InputLayout.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format        = xiiGALTextureFormat::RGB32Float;
    si.m_Semantic      = xiiGALInputLayoutSemantic::Normal;
    si.m_uiElementSize = sizeof(xiiVec3);
    m_InputLayout.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format        = xiiGALTextureFormat::RGBA32Float;
    si.m_Semantic      = xiiGALInputLayoutSemantic::Tangent;
    si.m_uiElementSize = sizeof(xiiVec4);
    m_InputLayout.m_VertexStreams.PushBack(si);

    if (m_Descriptor.m_bColorStream)
    {
      si.m_uiVertexBufferSlot = 1; // separate buffer
      si.m_uiOffset           = 0;
      si.m_Format             = xiiGALTextureFormat::RGBA8UNormalized;
      si.m_Semantic           = xiiGALInputLayoutSemantic::Color0;
      si.m_uiElementSize      = sizeof(xiiColorLinearUB);
      m_InputLayout.m_VertexStreams.PushBack(si);
    }

    m_InputLayout.ComputeHash();
  }

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiElementByteStride = sizeof(xiiDynamicMeshVertex);
    desc.m_uiSize              = desc.m_uiElementByteStride * m_Descriptor.m_uiMaxVertices;
    desc.m_BindFlags.Add(xiiGALBindFlags::VertexBuffer);
    m_hVertexBuffer = pDevice->CreateBuffer(desc);
  }

  // xiiStringBuilder sName;
  // sName.Format("{0} - Dynamic Vertex Buffer", GetResourceDescription());
  // pDevice->GetBuffer(m_hVertexBuffer)->SetDebugName(sName);

  const xiiUInt32 uiMaxIndices = xiiGALPrimitiveTopology::VerticesPerPrimitive(m_Descriptor.m_Topology) * m_Descriptor.m_uiMaxPrimitives;

  if (m_Descriptor.m_bColorStream)
  {
    m_ColorData.SetCountUninitialized(uiMaxIndices);

    xiiGALBufferCreationDescription desc;
    desc.m_uiElementByteStride = sizeof(xiiColorLinearUB);
    desc.m_uiSize              = desc.m_uiElementByteStride * m_Descriptor.m_uiMaxVertices;
    desc.m_BindFlags.Add(xiiGALBindFlags::VertexBuffer);
    m_hColorBuffer = pDevice->CreateBuffer(desc);

    // sName.Format("{0} - Dynamic Color Buffer", GetResourceDescription());
    // pDevice->GetBuffer(m_hColorBuffer)->SetDebugName(sName);
  }

  if (m_Descriptor.m_IndexType == xiiGALValueType::UInt32)
  {
    m_Index32Data.SetCountUninitialized(uiMaxIndices);

    xiiGALBufferCreationDescription desc;
    desc.m_uiElementByteStride = xiiGALValueType::GetSize(xiiGALValueType::UInt32);
    desc.m_uiSize              = desc.m_uiElementByteStride * uiMaxIndices;
    desc.m_BindFlags.Add(xiiGALBindFlags::IndexBuffer);
    m_hIndexBuffer = pDevice->CreateBuffer(desc);

    // sName.Format("{0} - Dynamic Index32 Buffer", GetResourceDescription());
    // pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);
  }
  else if (m_Descriptor.m_IndexType == xiiGALValueType::UInt16)
  {
    m_Index16Data.SetCountUninitialized(uiMaxIndices);

    xiiGALBufferCreationDescription desc;
    desc.m_uiElementByteStride = xiiGALValueType::GetSize(xiiGALValueType::UInt16);
    desc.m_uiSize              = desc.m_uiElementByteStride * uiMaxIndices;
    desc.m_BindFlags.Add(xiiGALBindFlags::IndexBuffer);
    m_hIndexBuffer = pDevice->CreateBuffer(desc);

    // sName.Format("{0} - Dynamic Index16 Buffer", GetResourceDescription());
    // pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);
  }

  // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
  ModifyMemoryUsage().m_uiMemoryGPU = m_VertexData.GetHeapMemoryUsage() + m_Index32Data.GetHeapMemoryUsage() + m_Index16Data.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage();

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

void xiiDynamicMeshBufferResource::UpdateGpuBuffer(xiiGALCommandEncoder* pGALCommandEncoder, xiiUInt32 uiFirstVertex, xiiUInt32 uiNumVertices, xiiUInt32 uiFirstIndex, xiiUInt32 uiNumIndices, xiiBitflags<xiiGALMapFlags> mapFlags /*= xiiGALMapFlags::Discard*/)
{
  if (m_bAccessedVB && uiNumVertices > 0)
  {
    if (uiNumVertices == xiiMath::MaxValue<xiiUInt32>())
      uiNumVertices = m_VertexData.GetCount() - uiFirstVertex;

    XII_ASSERT_DEV(uiNumVertices <= m_VertexData.GetCount(), "Can't upload {} vertices, the buffer was allocated to hold a maximum of {} vertices.", uiNumVertices, m_VertexData.GetCount());

    m_bAccessedVB = false;

    pGALCommandEncoder->UpdateBuffer(m_hVertexBuffer, sizeof(xiiDynamicMeshVertex) * uiFirstVertex, m_VertexData.GetArrayPtr().GetSubArray(uiFirstVertex, uiNumVertices).ToByteArray(), mapFlags);
  }

  if (m_bAccessedCB && uiNumVertices > 0)
  {
    if (uiNumVertices == xiiMath::MaxValue<xiiUInt32>())
      uiNumVertices = m_ColorData.GetCount() - uiFirstVertex;

    XII_ASSERT_DEV(uiNumVertices <= m_ColorData.GetCount(), "Can't upload {} vertices, the buffer was allocated to hold a maximum of {} vertices.", uiNumVertices, m_ColorData.GetCount());

    m_bAccessedCB = false;

    pGALCommandEncoder->UpdateBuffer(m_hColorBuffer, sizeof(xiiColorLinearUB) * uiFirstVertex, m_ColorData.GetArrayPtr().GetSubArray(uiFirstVertex, uiNumVertices).ToByteArray(), mapFlags);
  }

  if (m_bAccessedIB && uiNumIndices > 0 && !m_hIndexBuffer.IsInvalidated())
  {
    m_bAccessedIB = false;

    if (!m_Index16Data.IsEmpty())
    {
      XII_ASSERT_DEV(uiFirstIndex < m_Index16Data.GetCount(), "Invalid first index value {}", uiFirstIndex);

      if (uiNumIndices == xiiMath::MaxValue<xiiUInt32>())
        uiNumIndices = m_Index16Data.GetCount() - uiFirstIndex;

      XII_ASSERT_DEV(uiNumIndices <= m_Index16Data.GetCount(), "Can't upload {} indices, the buffer was allocated to hold a maximum of {} indices.", uiNumIndices, m_Index16Data.GetCount());

      pGALCommandEncoder->UpdateBuffer(m_hIndexBuffer, sizeof(xiiUInt16) * uiFirstIndex, m_Index16Data.GetArrayPtr().GetSubArray(uiFirstIndex, uiNumIndices).ToByteArray(), mapFlags);
    }
    else if (!m_Index32Data.IsEmpty())
    {
      XII_ASSERT_DEV(uiFirstIndex < m_Index32Data.GetCount(), "Invalid first index value {}", uiFirstIndex);

      if (uiNumIndices == xiiMath::MaxValue<xiiUInt32>())
        uiNumIndices = m_Index32Data.GetCount() - uiFirstIndex;

      XII_ASSERT_DEV(uiNumIndices <= m_Index32Data.GetCount(), "Can't upload {} indices, the buffer was allocated to hold a maximum of {} indices.", uiNumIndices, m_Index32Data.GetCount());

      pGALCommandEncoder->UpdateBuffer(m_hIndexBuffer, sizeof(xiiUInt32) * uiFirstIndex, m_Index32Data.GetArrayPtr().GetSubArray(uiFirstIndex, uiNumIndices).ToByteArray(), mapFlags);
    }
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_DynamicMeshBufferResource);
