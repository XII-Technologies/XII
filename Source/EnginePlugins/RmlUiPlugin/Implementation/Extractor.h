#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>

namespace xiiRmlUiInternal
{
  struct GeometryId : public xiiGenericId<24, 8>
  {
    using xiiGenericId::xiiGenericId;

    static GeometryId FromRml(Rml::CompiledGeometryHandle geometry) { return GeometryId(static_cast<xiiUInt32>(geometry)); }

    Rml::CompiledGeometryHandle ToRml() const { return m_Data; }
  };

  struct TextureId : public xiiGenericId<24, 8>
  {
    using xiiGenericId::xiiGenericId;

    static TextureId FromRml(Rml::TextureHandle texture) { return TextureId(static_cast<xiiUInt32>(texture)); }

    Rml::TextureHandle ToRml() const { return m_Data; }
  };

  //////////////////////////////////////////////////////////////////////////

  class Extractor final : public Rml::RenderInterface
  {
  public:
    Extractor();
    virtual ~Extractor();

    virtual void RenderGeometry(Rml::Vertex* pVertices, int iNum_vertices, int* pIndices, int iNum_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation) override;

    virtual Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex* pVertices, int iNum_vertices, int* pIndices, int iNum_indices, Rml::TextureHandle texture) override;
    virtual void                        RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry_handle, const Rml::Vector2f& translation) override;
    virtual void                        ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry_handle) override;

    virtual void EnableScissorRegion(bool bEnable) override;
    virtual void SetScissorRegion(int x, int y, int iWidth, int iHeight) override;

    virtual bool LoadTexture(Rml::TextureHandle& ref_texture_handle, Rml::Vector2i& ref_texture_dimensions, const Rml::String& sSource) override;
    virtual bool GenerateTexture(Rml::TextureHandle& ref_texture_handle, const Rml::byte* pSource, const Rml::Vector2i& source_dimensions) override;
    virtual void ReleaseTexture(Rml::TextureHandle texture_handle) override;

    virtual void SetTransform(const Rml::Matrix4f* pTransform) override;

    void BeginExtraction(const xiiVec2I32& vOffset);
    void EndExtraction();

    xiiRenderData* GetRenderData();

  private:
    void EndFrame(const xiiGALDeviceEvent& e);
    void FreeReleasedGeometry(GeometryId id);

    xiiIdTable<GeometryId, CompiledGeometry> m_CompiledGeometry;

    struct ReleasedGeometry
    {
      xiiUInt64  m_uiFrame;
      GeometryId m_Id;
    };

    xiiDeque<ReleasedGeometry> m_ReleasedCompiledGeometry;

    xiiIdTable<TextureId, xiiTexture2DResourceHandle> m_Textures;
    xiiTexture2DResourceHandle                        m_hFallbackTexture;

    xiiVec2 m_vOffset = xiiVec2::ZeroVector();

    xiiMat4      m_mTransform         = xiiMat4::IdentityMatrix();
    xiiRectFloat m_ScissorRect        = xiiRectFloat(0, 0);
    bool         m_bEnableScissorRect = false;

    xiiDynamicArray<Batch> m_Batches;
  };
} // namespace xiiRmlUiInternal
