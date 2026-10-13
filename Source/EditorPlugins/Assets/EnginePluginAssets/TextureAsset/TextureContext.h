/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

class XII_ENGINEPLUGINASSETS_DLL xiiTextureContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureContext, xiiEngineProcessDocumentContext);

public:
  xiiTextureContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

  const xiiTexture2DResourceHandle& GetTexture() const { return m_hTexture; }
  xiiInt32                          GetLodLevel() const { return m_iLodLevel; }
  xiiInt32                          GetChannelMode() const { return m_iChannelMode; }
  float                             GetAlphaThreshold() const { return m_fAlphaThreshold; }

protected:
  virtual xiiStatus OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;

private:
  xiiTexture2DResourceHandle m_hTexture;
  xiiInt32                   m_iLodLevel       = -1;
  xiiInt32                   m_iChannelMode    = 0;
  float                      m_fAlphaThreshold = 0.5f;
};
