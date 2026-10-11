/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/TextureCubeAsset/TextureCubeContext.h>
#include <EnginePluginAssets/TextureCubeAsset/TextureCubeView.h>


// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTextureCubeContext, 1, xiiRTTIDefaultAllocator<xiiTextureCubeContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Texture Cube"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTextureCubeContext::xiiTextureCubeContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
}

void xiiTextureCubeContext::HandleMessage(const xiiEditorEngineDocumentMsg* pMsg)
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
  }
  xiiEngineProcessDocumentContext::HandleMessage(pMsg);
}

void xiiTextureCubeContext::OnInitialize()
{
  xiiStringBuilder textureGuid;
  xiiConversionUtils::ToString(GetDocumentGuid(), textureGuid);
  m_hTexture = xiiResourceManager::LoadResource<xiiTextureCubeResource>(textureGuid);
}

xiiEngineProcessViewContext* xiiTextureCubeContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiTextureCubeViewContext, this);
}

void xiiTextureCubeContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}
