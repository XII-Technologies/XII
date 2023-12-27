#pragma once

#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

struct xiiVisualShaderPinDescriptor;

class xiiVisualShaderPin : public xiiPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualShaderPin, xiiPin);

public:
  xiiVisualShaderPin(Type type, const xiiVisualShaderPinDescriptor* pDescriptor, const xiiDocumentObject* pObject);

  const xiiRTTI*                      GetDataType() const;
  const xiiString&                    GetTooltip() const;
  const xiiVisualShaderPinDescriptor* GetDescriptor() const { return m_pDescriptor; }

private:
  const xiiVisualShaderPinDescriptor* m_pDescriptor;
};

class xiiVisualShaderNodeManager : public xiiDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const xiiDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const override;

  virtual xiiStatus     InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const override;
  virtual xiiStringView GetTypeCategory(const xiiRTTI* pRtti) const override;

private:
  virtual xiiStatus InternalCanAdd(
    const xiiRTTI*           pRtti,
    const xiiDocumentObject* pParent,
    xiiStringView            sParentProperty,
    const xiiVariant&        index) const override;

  xiiUInt32 CountNodesOfType(xiiVisualShaderNodeType::Enum type) const;
};
