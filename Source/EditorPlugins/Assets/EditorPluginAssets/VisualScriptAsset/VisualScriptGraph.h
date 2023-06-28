#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

struct xiiVisualScriptPinDescriptor;

class xiiVisualScriptPin_Legacy : public xiiPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptPin_Legacy, xiiPin);

public:
  xiiVisualScriptPin_Legacy(Type type, const xiiVisualScriptPinDescriptor* pDescriptor, const xiiDocumentObject* pObject);

  const xiiString&                    GetTooltip() const;
  const xiiVisualScriptPinDescriptor* GetDescriptor() const { return m_pDescriptor; }

private:
  const xiiVisualScriptPinDescriptor* m_pDescriptor;
};

class xiiVisualScriptConnection_Legacy : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptConnection_Legacy, xiiReflectedClass);
};

class xiiVisualScriptNodeManager_Legacy : public xiiDocumentNodeManager
{
public:
  virtual bool           InternalIsNode(const xiiDocumentObject* pObject) const override;
  virtual void           InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void           GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const override;
  virtual const xiiRTTI* GetConnectionType() const override;
  virtual const char*    GetTypeCategory(const xiiRTTI* pRtti) const override;

  virtual xiiStatus InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_result) const override;
};
