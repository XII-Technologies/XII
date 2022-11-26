#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/Status.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

struct xiiGameObjectDocumentEvent;

class xiiTypeScriptAssetDocumentManager : public xiiAssetDocumentManager
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTypeScriptAssetDocumentManager, xiiAssetDocumentManager);

public:
  xiiTypeScriptAssetDocumentManager();
  ~xiiTypeScriptAssetDocumentManager();

  xiiTypeScriptTranspiler& GetTranspiler() { return m_Transpiler; }

  void      SetupProjectForTypeScript(bool bForce);
  xiiResult GenerateScriptCompendium(xiiBitflags<xiiTransformFlags> transformFlags);

  virtual xiiStatus GetAdditionalOutputs(xiiDynamicArray<xiiString>& files) override;

private:
  void OnDocumentManagerEvent(const xiiDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    const char*              szDocumentTypeName,
    const char*              szPath,
    bool                     bCreateNewDocument,
    xiiDocument*&            out_pDocument,
    const xiiDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  void ToolsProjectEventHandler(const xiiToolsProjectEvent& e);
  void GameObjectDocumentEventHandler(const xiiGameObjectDocumentEvent& e);

  static void ModifyTsBeforeTranspilation(xiiStringBuilder& source);

  void InitializeTranspiler();
  void ShutdownTranspiler();

  bool                    m_bTranspilerLoaded = false;
  bool                    m_bProjectSetUp     = false;
  xiiTypeScriptTranspiler m_Transpiler;

  xiiAssetDocumentTypeDescriptor m_DocTypeDesc;

  xiiMap<xiiString, xiiTimestamp> m_CheckedTsFiles;
};

//////////////////////////////////////////////////////////////////////////

class xiiTypeScriptPreferences : public xiiPreferences
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTypeScriptPreferences, xiiPreferences);

public:
  xiiTypeScriptPreferences();

  bool m_bAutoUpdateScriptsForSimulation  = true;
  bool m_bAutoUpdateScriptsForPlayTheGame = true;
};
