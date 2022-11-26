#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>
#include <Foundation/Communication/Event.h>

class xiiTypeScriptAssetDocument;

struct xiiTypeScriptAssetDocumentEvent
{
  enum class Type
  {
    None,
    ScriptCreated,
    ScriptOpened,
    ScriptTransformed,
  };

  Type                        m_Type      = Type::None;
  xiiTypeScriptAssetDocument* m_pDocument = nullptr;
};

class xiiTypeScriptAssetDocument : public xiiSimpleAssetDocument<xiiTypeScriptAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTypeScriptAssetDocument, xiiSimpleAssetDocument<xiiTypeScriptAssetProperties>);

public:
  xiiTypeScriptAssetDocument(const char* szDocumentPath);

  void EditScript();

  const xiiEvent<const xiiTypeScriptAssetDocumentEvent&>& GetEvent() const { return m_Events; }

protected:
  void      CreateComponentFile(const char* szFile);
  void      CreateTsConfigFiles();
  xiiResult CreateTsConfigFile(const char* szDirectory);

  virtual void UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;

  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;

  xiiStatus ValidateScriptCode();
  xiiStatus AutoGenerateVariablesCode();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  xiiEvent<const xiiTypeScriptAssetDocumentEvent&> m_Events;
};
