#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Meshes/DynamicMeshBufferResource.h>
#include <GraphicsCore/Meshes/MeshBufferUtils.h>
#include <GraphicsCore/RenderContext/RenderContext.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDynamicMeshBufferResource, 1, xiiRTTIDefaultAllocator<xiiDynamicMeshBufferResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiDynamicMeshBufferResource);

xiiDynamicMeshBufferResource::xiiDynamicMeshBufferResource() :
  xiiResource(DoUpdate::OnGraphicsResourceThreads, 1)
{
}

xiiDynamicMeshBufferResource::~xiiDynamicMeshBufferResource()
{
  XII_ASSERT_DEBUG(!m_VertexBuffer.IsInitialized(), "Implementation error");
  XII_ASSERT_DEBUG(!m_IndexBuffer.IsInitialized(), "Implementation error");
  XII_ASSERT_DEBUG(!m_ColorBuffer.IsInitialized(), "Implementation error");
}

xiiResourceLoadDesc xiiDynamicMeshBufferResource::UnloadData(Unload WhatToUnload)
{
  if (m_VertexBuffer.IsInitialized())
  {
    m_VertexBuffer.Deinitialize();
  }

  if (m_IndexBuffer.IsInitialized())
  {
    m_IndexBuffer.Deinitialize();
  }

  if (m_ColorBuffer.IsInitialized())
  {
    m_ColorBuffer.Deinitialize();
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
  XII_ASSERT_DEBUG(!m_VertexBuffer.IsInitialized(), "Implementation error");
  XII_ASSERT_DEBUG(!m_IndexBuffer.IsInitialized(), "Implementation error");
  XII_ASSERT_DEBUG(!m_ColorBuffer.IsInitialized(), "Implementation error");

  m_Descriptor = descriptor;

  m_VertexData.SetCountUninitialized(m_Descriptor.m_uiMaxVertices);

  {
    xiiVertexStreamInfo si;
    si.m_uiOffset      = 0;
    si.m_Format        = xiiGALResourceFormat::XYZFloat;
    si.m_Semantic      = xiiGALVertexAttributeSemantic::Position;
    si.m_uiElementSize = sizeof(xiiVec3);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format        = xiiGALResourceFormat::XYFloat;
    si.m_Semantic      = xiiGALVertexAttributeSemantic::TexCoord0;
    si.m_uiElementSize = sizeof(xiiVec2);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format        = xiiGALResourceFormat::XYZFloat;
    si.m_Semantic      = xiiGALVertexAttributeSemantic::Normal;
    si.m_uiElementSize = sizeof(xiiVec3);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format        = xiiGALResourceFormat::XYZWFloat;
    si.m_Semantic      = xiiGALVertexAttributeSemantic::Tangent;
    si.m_uiElementSize = sizeof(xiiVec4);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    if (m_Descriptor.m_bColorStream)
    {
      si.m_uiVertexBufferSlot = 1; // separate buffer
      si.m_uiOffset           = 0;
      si.m_Format             = xiiGALResourceFormat::RGBAUByteNormalized;
      si.m_Semantic           = xiiGALVertexAttributeSemantic::Color0;
      si.m_uiElementSize      = sizeof(xiiColorLinearUB);
      m_VertexDeclaration.m_VertexStreams.PushBack(si);
    }

    m_VertexDeclaration.ComputeHash();
  }

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  xiiStringBuilder           sName;
  {
    xiiGALBufferCreationDescription vertexDesc;
    vertexDesc.m_uiStructSize                = sizeof(xiiDynamicMeshVertex);
    vertexDesc.m_uiTotalSize                 = sizeof(xiiDynamicMeshVertex) * xiiMath::Max(1u, m_Descriptor.m_uiMaxVertices);
    vertexDesc.m_BufferFlags                 = xiiGALBufferUsageFlags::VertexBuffer;
    vertexDesc.m_ResourceAccess.m_bImmutable = false;

    sName.SetFormat("{0} - Dynamic Vertex Buffer", GetResourceDescription());
    m_VertexBuffer.Initialize(vertexDesc, sName);
    m_VertexBuffer.GetNewBuffer();
  }

  const xiiUInt32 uiMaxIndices = xiiGALPrimitiveTopology::GetIndexCount(m_Descriptor.m_Topology, m_Descriptor.m_uiMaxPrimitives);

  if (m_Descriptor.m_bColorStream)
  {
    m_ColorData.SetCountUninitialized(uiMaxIndices);

    xiiGALBufferCreationDescription vertexDesc;
    vertexDesc.m_uiStructSize                = sizeof(xiiColorLinearUB);
    vertexDesc.m_uiTotalSize                 = sizeof(xiiColorLinearUB) * xiiMath::Max(1u, m_Descriptor.m_uiMaxVertices);
    vertexDesc.m_BufferFlags                 = xiiGALBufferUsageFlags::VertexBuffer;
    vertexDesc.m_ResourceAccess.m_bImmutable = false;

    sName.SetFormat("{0} - Dynamic Color Buffer", GetResourceDescription());
    m_ColorBuffer.Initialize(vertexDesc, sName);
    m_ColorBuffer.GetNewBuffer();
  }

  if (m_Descriptor.m_IndexType == xiiGALIndexType::UInt)
  {
    m_Index32Data.SetCountUninitialized(uiMaxIndices);

    xiiGALBufferCreationDescription desc;
    desc.m_uiStructSize                = xiiGALIndexType::GetSize(xiiGALIndexType::UInt);
    desc.m_uiTotalSize                 = desc.m_uiStructSize * xiiMath::Max(1u, uiMaxIndices);
    desc.m_BufferFlags                 = xiiGALBufferUsageFlags::IndexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    sName.SetFormat("{0} - Dynamic Index32 Buffer", GetResourceDescription());
    m_IndexBuffer.Initialize(desc, sName);
    m_IndexBuffer.GetNewBuffer();
  }
  else if (m_Descriptor.m_IndexType == xiiGALIndexType::UShort)
  {
    m_Index16Data.SetCountUninitialized(uiMaxIndices);

    xiiGALBufferCreationDescription desc;
    desc.m_uiStructSize                = xiiGALIndexType::GetSize(xiiGALIndexType::UShort);
    desc.m_uiTotalSize                 = desc.m_uiStructSize * xiiMath::Max(1u, uiMaxIndices);
    desc.m_BufferFlags                 = xiiGALBufferUsageFlags::IndexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    sName.SetFormat("{0} - Dynamic Index16 Buffer", GetResourceDescription());
    m_IndexBuffer.Initialize(desc, sName);
    m_IndexBuffer.GetNewBuffer();
  }

  // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
  ModifyMemoryUsage().m_uiMemoryGPU = m_VertexData.GetHeapMemoryUsage() + m_Index32Data.GetHeapMemoryUsage() + m_Index16Data.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage();

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

void xiiDynamicMeshBufferResource::UpdateGpuBuffer(xiiGALCommandEncoder* pGALCommandEncoder, xiiUInt32 uiFirstVertex, xiiUInt32 uiNumVertices, xiiUInt32 uiFirstIndex, xiiUInt32 uiNumIndices, bool bCreateNewBuffer)
{
  if (m_bAccessedVB && uiNumVertices > 0)
  {
    xiiGALBufferHandle hVertexBuffer = m_VertexBuffer.GetCurrentBuffer();
    if (bCreateNewBuffer)
      hVertexBuffer = m_VertexBuffer.GetNewBuffer();

    if (uiNumVertices == xiiMath::MaxValue<xiiUInt32>())
      uiNumVertices = m_VertexData.GetCount() - uiFirstVertex;

    XII_ASSERT_DEV(uiNumVertices <= m_VertexData.GetCount(), "Can't upload {} vertices, the buffer was allocated to hold a maximum of {} vertices.", uiNumVertices, m_VertexData.GetCount());

    m_bAccessedVB = false;

    pGALCommandEncoder->UpdateBuffer(hVertexBuffer, sizeof(xiiDynamicMeshVertex) * uiFirstVertex, m_VertexData.GetArrayPtr().GetSubArray(uiFirstVertex, uiNumVertices).ToByteArray(), xiiGALUpdateMode::AheadOfTime);
  }

  if (m_bAccessedCB && uiNumVertices > 0)
  {
    xiiGALBufferHandle hColorBuffer = m_ColorBuffer.GetCurrentBuffer();
    if (bCreateNewBuffer)
      hColorBuffer = m_ColorBuffer.GetNewBuffer();

    if (uiNumVertices == xiiMath::MaxValue<xiiUInt32>())
      uiNumVertices = m_ColorData.GetCount() - uiFirstVertex;

    XII_ASSERT_DEV(uiNumVertices <= m_ColorData.GetCount(), "Can't upload {} vertices, the buffer was allocated to hold a maximum of {} vertices.", uiNumVertices, m_ColorData.GetCount());

    m_bAccessedCB = false;

    pGALCommandEncoder->UpdateBuffer(hColorBuffer, sizeof(xiiColorLinearUB) * uiFirstVertex, m_ColorData.GetArrayPtr().GetSubArray(uiFirstVertex, uiNumVertices).ToByteArray(), xiiGALUpdateMode::AheadOfTime);
  }

  if (m_bAccessedIB && uiNumIndices > 0 && m_IndexBuffer.IsInitialized())
  {
    m_bAccessedIB = false;

    if (!m_Index16Data.IsEmpty())
    {
      xiiGALBufferHandle hIndexBuffer = m_IndexBuffer.GetCurrentBuffer();
      if (bCreateNewBuffer)
        hIndexBuffer = m_IndexBuffer.GetNewBuffer();

      XII_ASSERT_DEV(uiFirstIndex < m_Index16Data.GetCount(), "Invalid first index value {}", uiFirstIndex);

      if (uiNumIndices == xiiMath::MaxValue<xiiUInt32>())
        uiNumIndices = m_Index16Data.GetCount() - uiFirstIndex;

      XII_ASSERT_DEV(uiNumIndices <= m_Index16Data.GetCount(), "Can't upload {} indices, the buffer was allocated to hold a maximum of {} indices.", uiNumIndices, m_Index16Data.GetCount());

      pGALCommandEncoder->UpdateBuffer(hIndexBuffer, sizeof(xiiUInt16) * uiFirstIndex, m_Index16Data.GetArrayPtr().GetSubArray(uiFirstIndex, uiNumIndices).ToByteArray(), xiiGALUpdateMode::AheadOfTime);
    }
    else if (!m_Index32Data.IsEmpty())
    {
      xiiGALBufferHandle hIndexBuffer = m_IndexBuffer.GetCurrentBuffer();
      if (bCreateNewBuffer)
        hIndexBuffer = m_IndexBuffer.GetNewBuffer();

      XII_ASSERT_DEV(uiFirstIndex < m_Index32Data.GetCount(), "Invalid first index value {}", uiFirstIndex);

      if (uiNumIndices == xiiMath::MaxValue<xiiUInt32>())
        uiNumIndices = m_Index32Data.GetCount() - uiFirstIndex;

      XII_ASSERT_DEV(uiNumIndices <= m_Index32Data.GetCount(), "Can't upload {} indices, the buffer was allocated to hold a maximum of {} indices.", uiNumIndices, m_Index32Data.GetCount());

      pGALCommandEncoder->UpdateBuffer(hIndexBuffer, sizeof(xiiUInt32) * uiFirstIndex, m_Index32Data.GetArrayPtr().GetSubArray(uiFirstIndex, uiNumIndices).ToByteArray(), xiiGALUpdateMode::AheadOfTime);
    }
  }
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Meshes_Implementation_DynamicMeshBufferResource);
