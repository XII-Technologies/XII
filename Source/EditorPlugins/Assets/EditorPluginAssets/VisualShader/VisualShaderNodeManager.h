/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

struct xiiVisualShaderPinDescriptor;

class XII_EDITORPLUGINASSETS_DLL xiiVisualShaderPin : public xiiPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualShaderPin, xiiPin);

public:
  xiiVisualShaderPin(Type type, const xiiVisualShaderPinDescriptor* pDescriptor, const xiiDocumentObject* pObject);

  const xiiRTTI*                      GetDataType() const;
  const xiiString&                    GetTooltip() const;
  static bool                         CanConvert(const xiiVisualShaderPin& source, const xiiVisualShaderPin& target);
  const xiiVisualShaderPinDescriptor* GetDescriptor() const { return m_pDescriptor; }

private:
  const xiiVisualShaderPinDescriptor* m_pDescriptor;
};

class XII_EDITORPLUGINASSETS_DLL xiiVisualShaderNodeManager : public xiiDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const xiiDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetNodeCreationTemplates(xiiDynamicArray<xiiNodeCreationTemplate>& out_templates) const override;

  virtual xiiStatus InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const override;

private:
  virtual xiiStatus InternalCanAdd(const xiiRTTI* pRtti, const xiiDocumentObject* pParent, xiiStringView sParentProperty, const xiiVariant& index) const override;

  xiiUInt32 CountNodesOfType(xiiVisualShaderNodeType::Enum type) const;
};
