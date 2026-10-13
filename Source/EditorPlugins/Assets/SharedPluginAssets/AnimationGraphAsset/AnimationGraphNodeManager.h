/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <SharedPluginAssets/SharedPluginAssetsDLL.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class XII_SHAREDPLUGINASSETS_DLL xiiAnimationGraphNodeManager : public xiiDocumentNodeManager
{
public:
  void         GetNodeCreationTemplates(xiiDynamicArray<xiiNodeCreationTemplate>& out_templates) const override;
  virtual bool InternalIsNode(const xiiDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const override;

  virtual xiiStatus InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const override;

private:
  virtual bool                                    InternalIsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const override;
  mutable xiiHybridArray<xiiNodePropertyValue, 6> m_TemplateProperties;
};
