#pragma once

#include <EditorPluginScene/Scene/SceneDocument.h>

class xiiScene2Document;

class XII_EDITORPLUGINSCENE_DLL xiiLayerDocument : public xiiSceneDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLayerDocument, xiiSceneDocument);

public:
  xiiLayerDocument(const char* szDocumentPath, xiiScene2Document* pParentScene);
  ~xiiLayerDocument();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void InitializeAfterLoadingAndSaving() override;

  virtual xiiVariant GetCreateEngineMetaData() const override;
};
