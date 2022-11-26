#pragma once

#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Textures/Texture2DResource.h>

class xiiSceneContext;

class xiiEditorShapeIconsExtractor : public xiiExtractor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorShapeIconsExtractor, xiiExtractor);

public:
  xiiEditorShapeIconsExtractor(const char* szName = "EditorShapeIconsExtractor");
  ~xiiEditorShapeIconsExtractor();

  virtual void Extract(
    const xiiView&                               view,
    const xiiDynamicArray<const xiiGameObject*>& visibleObjects,
    xiiExtractedRenderData&                      extractedRenderData) override;

  void             SetSceneContext(xiiSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  xiiSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  void                                           ExtractShapeIcon(const xiiGameObject* pObject, const xiiView& view, xiiExtractedRenderData& extractedRenderData, xiiRenderData::Category category);
  const xiiTypedMemberProperty<xiiColor>*        FindColorProperty(const xiiRTTI* pRtti) const;
  const xiiTypedMemberProperty<xiiColorGammaUB>* FindColorGammaProperty(const xiiRTTI* pRtti) const;
  void                                           FillShapeIconInfo();

  float            m_fSize;
  float            m_fMaxScreenSize;
  xiiSceneContext* m_pSceneContext;

  struct ShapeIconInfo
  {
    xiiTexture2DResourceHandle                     m_hTexture;
    const xiiTypedMemberProperty<xiiColor>*        m_pColorProperty;
    const xiiTypedMemberProperty<xiiColorGammaUB>* m_pColorGammaProperty;
  };

  xiiHashTable<const xiiRTTI*, ShapeIconInfo> m_ShapeIconInfos;
};
