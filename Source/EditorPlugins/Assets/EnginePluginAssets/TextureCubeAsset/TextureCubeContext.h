/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>

class XII_ENGINEPLUGINASSETS_DLL xiiTextureCubeContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTextureCubeContext, xiiEngineProcessDocumentContext);

public:
  xiiTextureCubeContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

  const xiiTextureCubeResourceHandle& GetTexture() const { return m_hTexture; }
  xiiInt32                            GetLodLevel() const { return m_iLodLevel; }
  xiiInt32                            GetChannelMode() const { return m_iChannelMode; }
  float                               GetAlphaThreshold() const { return m_fAlphaThreshold; }

protected:
  virtual xiiStatus OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;

private:
  xiiTextureCubeResourceHandle m_hTexture;
  xiiInt32                     m_iLodLevel       = -1;
  xiiInt32                     m_iChannelMode    = 0;
  float                        m_fAlphaThreshold = 0.5f;
};
