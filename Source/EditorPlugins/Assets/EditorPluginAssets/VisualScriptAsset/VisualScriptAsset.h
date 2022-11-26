#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

struct xiiVisualScriptResourceDescriptor;
struct xiiVisualScriptInstanceActivity;

struct xiiVisualScriptActivityEvent
{
  xiiUuid                                m_ObjectGuid;
  const xiiVisualScriptInstanceActivity* m_pActivityData;
};

class xiiVisualScriptParameter : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptParameter, xiiReflectedClass);

public:
  xiiString m_sName;
  bool      m_bExpose = false;
};

class xiiVisualScriptParameterBool : public xiiVisualScriptParameter
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptParameterBool, xiiVisualScriptParameter);

public:
  bool m_DefaultValue = false;
};

class xiiVisualScriptParameterNumber : public xiiVisualScriptParameter
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptParameterNumber, xiiVisualScriptParameter);

public:
  double m_DefaultValue = 0;
};

class xiiVisualScriptParameterString : public xiiVisualScriptParameter
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptParameterString, xiiVisualScriptParameter);

public:
  xiiString m_DefaultValue;
};

class xiiVisualScriptAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptAssetProperties, xiiReflectedClass);

public:
  xiiDynamicArray<xiiVisualScriptParameterBool>   m_BoolParameters;
  xiiDynamicArray<xiiVisualScriptParameterNumber> m_NumberParameters;
  xiiDynamicArray<xiiVisualScriptParameterString> m_StringParameters;
};

class xiiVisualScriptAssetDocument : public xiiSimpleAssetDocument<xiiVisualScriptAssetProperties>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptAssetDocument, xiiSimpleAssetDocument<xiiVisualScriptAssetProperties>);

public:
  xiiVisualScriptAssetDocument(const char* szDocumentPath);

  void HandleVsActivityMsg(const xiiVisualScriptActivityMsgToEditor* pActivityMsg);
  void OnInterDocumentMessage(xiiReflectedClass* pMessage, xiiDocument* pSender) override;

  xiiEvent<const xiiVisualScriptActivityEvent&> m_ActivityEvents;
  xiiEvent<xiiReflectedClass*>                  m_InterDocumentMessages;

protected:
  virtual xiiTransformStatus InternalTransformAsset(xiiStreamWriter& stream, const char* szOutputTag, const xiiPlatformProfile* pAssetProfile, const xiiAssetFileHeader& AssetHeader, xiiBitflags<xiiTransformFlags> transformFlags) override;
  virtual void               UpdateAssetDocumentInfo(xiiAssetDocumentInfo* pInfo) const override;

  virtual void GetSupportedMimeTypesForPasting(xiiHybridArray<xiiString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(xiiAbstractObjectGraph& out_objectGraph, xiiStringBuilder& out_MimeType) const override;
  virtual bool Paste(const xiiArrayPtr<PasteInfo>& info, const xiiAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType) override;

  virtual void InternalGetMetaDataHash(const xiiDocumentObject* pObject, xiiUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(xiiAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const xiiAbstractObjectGraph& graph, bool bUndoable) override;

  xiiResult GenerateVisualScriptDescriptor(xiiVisualScriptResourceDescriptor& desc);

  void GetAllVsNodes(xiiDynamicArray<const xiiDocumentObject*>& allNodes) const;
  void HighlightConnections(const xiiVisualScriptInstanceActivity& act);
};
