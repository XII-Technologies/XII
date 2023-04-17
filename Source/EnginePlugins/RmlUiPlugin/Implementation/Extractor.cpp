#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RmlUiPlugin/Implementation/Extractor.h>

namespace xiiRmlUiInternal
{
  Extractor::Extractor()
  {
    m_hFallbackTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>("White.color");

    xiiGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(xiiMakeDelegate(&Extractor::EndFrame, this));
  }

  Extractor::~Extractor()
  {
    xiiGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(xiiMakeDelegate(&Extractor::EndFrame, this));

    for (auto it = m_CompiledGeometry.GetIterator(); it.IsValid(); ++it)
    {
      FreeReleasedGeometry(it.Id());
    }
  }

  void Extractor::RenderGeometry(Rml::Vertex* pVertices, int iNum_vertices, int* pIndices, int iNum_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation)
  {
    // Should never be called since we are using compiled geometry
    XII_ASSERT_NOT_IMPLEMENTED;
  }

  Rml::CompiledGeometryHandle Extractor::CompileGeometry(Rml::Vertex* pVertices, int iNum_vertices, int* pIndices, int iNum_indices, Rml::TextureHandle texture)
  {
    CompiledGeometry geometry;
    geometry.m_uiTriangleCount = iNum_indices / 3;

    // vertices
    {
      xiiDynamicArray<Vertex> vertexStorage(xiiFrameAllocator::GetCurrentAllocator());
      vertexStorage.SetCountUninitialized(iNum_vertices);

      for (xiiUInt32 i = 0; i < vertexStorage.GetCount(); ++i)
      {
        auto& srcVertex       = pVertices[i];
        auto& destVertex      = vertexStorage[i];
        destVertex.m_Position = xiiVec3(srcVertex.position.x, srcVertex.position.y, 0);
        destVertex.m_TexCoord = xiiVec2(srcVertex.tex_coord.x, srcVertex.tex_coord.y);
        destVertex.m_Color    = reinterpret_cast<const xiiColorGammaUB&>(srcVertex.colour);
      }

      xiiGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(Vertex);
      desc.m_uiTotalSize  = vertexStorage.GetCount() * desc.m_uiStructSize;
      desc.m_BufferType   = xiiGALBufferType::VertexBuffer;

      geometry.m_hVertexBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc, vertexStorage.GetByteArrayPtr());
    }

    // indices
    {
      xiiGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(xiiUInt32);
      desc.m_uiTotalSize  = iNum_indices * desc.m_uiStructSize;
      desc.m_BufferType   = xiiGALBufferType::IndexBuffer;

      geometry.m_hIndexBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(desc, xiiMakeArrayPtr(pIndices, iNum_indices).ToByteArray());
    }

    // texture
    {
      xiiTexture2DResourceHandle* phTexture = nullptr;
      if (m_Textures.TryGetValue(TextureId::FromRml(texture), phTexture))
      {
        geometry.m_hTexture = *phTexture;
      }
      else
      {
        geometry.m_hTexture = m_hFallbackTexture;
      }
    }

    return m_CompiledGeometry.Insert(std::move(geometry)).ToRml();
  }

  void Extractor::RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry_handle, const Rml::Vector2f& translation)
  {
    auto& batch = m_Batches.ExpandAndGetRef();

    XII_VERIFY(m_CompiledGeometry.TryGetValue(GeometryId::FromRml(geometry_handle), batch.m_CompiledGeometry), "Invalid compiled geometry");

    xiiMat4 offsetMat;
    offsetMat.SetTranslationMatrix(m_vOffset.GetAsVec3(0));

    batch.m_Transform   = offsetMat * m_mTransform;
    batch.m_Translation = xiiVec2(translation.x, translation.y);

    batch.m_ScissorRect           = m_ScissorRect;
    batch.m_bEnableScissorRect    = m_bEnableScissorRect;
    batch.m_bTransformScissorRect = (m_bEnableScissorRect && m_mTransform.IsIdentity() == false);

    if (!batch.m_bTransformScissorRect)
    {
      batch.m_ScissorRect.x += m_vOffset.x;
      batch.m_ScissorRect.y += m_vOffset.y;
    }
  }

  void Extractor::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry_handle) { m_ReleasedCompiledGeometry.PushBack({xiiRenderWorld::GetFrameCounter(), GeometryId::FromRml(geometry_handle)}); }

  void Extractor::EnableScissorRegion(bool bEnable) { m_bEnableScissorRect = bEnable; }

  void Extractor::SetScissorRegion(int x, int y, int iWidth, int iHeight) { m_ScissorRect = xiiRectFloat(static_cast<float>(x), static_cast<float>(y), static_cast<float>(iWidth), static_cast<float>(iHeight)); }

  bool Extractor::LoadTexture(Rml::TextureHandle& ref_texture_handle, Rml::Vector2i& ref_texture_dimensions, const Rml::String& sSource)
  {
    xiiTexture2DResourceHandle hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(sSource.c_str());

    xiiResourceLock<xiiTexture2DResource> pTexture(hTexture, xiiResourceAcquireMode::BlockTillLoaded);
    if (pTexture.GetAcquireResult() == xiiResourceAcquireResult::Final)
    {
      ref_texture_handle     = m_Textures.Insert(hTexture).ToRml();
      ref_texture_dimensions = Rml::Vector2i(pTexture->GetWidth(), pTexture->GetHeight());

      return true;
    }

    return false;
  }

  bool Extractor::GenerateTexture(Rml::TextureHandle& ref_texture_handle, const Rml::byte* pSource, const Rml::Vector2i& source_dimensions)
  {
    xiiUInt32 uiWidth       = source_dimensions.x;
    xiiUInt32 uiHeight      = source_dimensions.y;
    xiiUInt32 uiSizeInBytes = uiWidth * uiHeight * 4;

    xiiUInt64 uiHash = xiiHashingUtils::xxHash64(pSource, uiSizeInBytes);

    xiiStringBuilder sTextureName;
    sTextureName.Format("RmlUiGeneratedTexture_{}x{}_{}", uiWidth, uiHeight, uiHash);

    xiiTexture2DResourceHandle hTexture = xiiResourceManager::GetExistingResource<xiiTexture2DResource>(sTextureName);

    if (!hTexture.IsValid())
    {
      xiiGALSystemMemoryDescription memoryDesc;
      memoryDesc.m_pData        = const_cast<Rml::byte*>(pSource);
      memoryDesc.m_uiRowPitch   = uiWidth * 4;
      memoryDesc.m_uiSlicePitch = uiSizeInBytes;

      xiiTexture2DResourceDescriptor desc;
      desc.m_DescGAL.m_uiWidth  = uiWidth;
      desc.m_DescGAL.m_uiHeight = uiHeight;
      desc.m_DescGAL.m_Format   = xiiGALResourceFormat::RGBAUByteNormalized;
      desc.m_InitialContent     = xiiMakeArrayPtr(&memoryDesc, 1);

      hTexture = xiiResourceManager::GetOrCreateResource<xiiTexture2DResource>(sTextureName, std::move(desc));
    }

    ref_texture_handle = m_Textures.Insert(hTexture).ToRml();
    return true;
  }

  void Extractor::ReleaseTexture(Rml::TextureHandle texture_handle) { XII_VERIFY(m_Textures.Remove(TextureId::FromRml(texture_handle)), "Invalid texture handle"); }

  void Extractor::SetTransform(const Rml::Matrix4f* pTransform)
  {
    if (pTransform != nullptr)
    {
      constexpr xiiMatrixLayout::Enum matrixLayout = std::is_same<Rml::Matrix4f, Rml::ColumnMajorMatrix4f>::value ? xiiMatrixLayout::ColumnMajor : xiiMatrixLayout::RowMajor;
      m_mTransform.SetFromArray(pTransform->data(), matrixLayout);
    }
    else
    {
      m_mTransform.SetIdentity();
    }
  }

  void Extractor::BeginExtraction(const xiiVec2I32& vOffset)
  {
    m_vOffset    = xiiVec2(static_cast<float>(vOffset.x), static_cast<float>(vOffset.y));
    m_mTransform = xiiMat4::IdentityMatrix();

    m_Batches.Clear();
  }

  void Extractor::EndExtraction() {}

  xiiRenderData* Extractor::GetRenderData()
  {
    if (m_Batches.IsEmpty() == false)
    {
      xiiRmlUiRenderData* pRenderData = XII_NEW(xiiFrameAllocator::GetCurrentAllocator(), xiiRmlUiRenderData, xiiFrameAllocator::GetCurrentAllocator());
      pRenderData->m_GlobalTransform.SetIdentity();
      pRenderData->m_GlobalBounds.SetInvalid();
      pRenderData->m_Batches = m_Batches;

      return pRenderData;
    }

    return nullptr;
  }

  void Extractor::EndFrame(const xiiGALDeviceEvent& e)
  {
    if (e.m_Type != xiiGALDeviceEvent::BeforeEndFrame)
      return;

    xiiUInt64 uiFrameCounter = xiiRenderWorld::GetFrameCounter();

    while (!m_ReleasedCompiledGeometry.IsEmpty())
    {
      auto& releasedGeometry = m_ReleasedCompiledGeometry.PeekFront();

      if (releasedGeometry.m_uiFrame >= uiFrameCounter)
        break;

      FreeReleasedGeometry(releasedGeometry.m_Id);

      m_CompiledGeometry.Remove(releasedGeometry.m_Id);
      m_ReleasedCompiledGeometry.PopFront();
    }
  }

  void Extractor::FreeReleasedGeometry(GeometryId id)
  {
    CompiledGeometry* pGeometry = nullptr;
    if (!m_CompiledGeometry.TryGetValue(id, pGeometry))
      return;

    xiiGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hVertexBuffer);
    pGeometry->m_hVertexBuffer.Invalidate();

    xiiGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hIndexBuffer);
    pGeometry->m_hIndexBuffer.Invalidate();

    pGeometry->m_hTexture.Invalidate();
  }

} // namespace xiiRmlUiInternal
