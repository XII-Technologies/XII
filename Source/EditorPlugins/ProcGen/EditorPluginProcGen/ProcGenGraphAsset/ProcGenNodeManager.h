#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class xiiProcGenPin : public xiiPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProcGenPin, xiiPin);

public:
  using xiiPin::xiiPin;
};

class xiiProcGenNodeManager : public xiiDocumentNodeManager
{
public:
  virtual bool        InternalIsNode(const xiiDocumentObject* pObject) const override;
  virtual void        InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& node) override;
  virtual void        GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& Types) const override;
  virtual const char* GetTypeCategory(const xiiRTTI* pRtti) const override;

  virtual xiiStatus InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_Result) const override;
};
