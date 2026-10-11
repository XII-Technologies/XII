/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureAsset/TextureContext.h>
#include <EnginePluginAssets/TextureAsset/TextureView.h>


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureContext, 1, xiiRTTIDefaultAllocator<xiiTextureContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture 2D;Render Target;Substance Package"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTextureContext::xiiTextureContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
}

void xiiTextureContext::HandleMessage(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiDocumentConfigMsgToEngine>())
  {
    const auto& config = *static_cast<const xiiDocumentConfigMsgToEngine*>(pMsg);
    if (config.m_sWhatToDo == "SetChannelMode")
    {
      m_iChannelMode    = config.m_iValue;
      m_fAlphaThreshold = config.m_fValue;
    }
    else if (config.m_sWhatToDo == "SetLodLevel")
    {
      m_iLodLevel = config.m_iValue;
    }
    else if (config.m_sWhatToDo == "SetTexture" && !config.m_sValue.IsEmpty())
    {
      m_hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(config.m_sValue);
    }
  }
  xiiEngineProcessDocumentContext::HandleMessage(pMsg);
}

void xiiTextureContext::OnInitialize()
{
  xiiStringBuilder textureGuid;
  xiiConversionUtils::ToString(GetDocumentGuid(), textureGuid);
  m_hTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>(textureGuid);
}

xiiEngineProcessViewContext* xiiTextureContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiTextureViewContext, this);
}

void xiiTextureContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}
