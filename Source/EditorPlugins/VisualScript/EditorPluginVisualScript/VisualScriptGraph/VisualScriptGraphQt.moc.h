#pragma once

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

class xiiQtVisualScriptPin : public xiiQtPin
{
public:
  xiiQtVisualScriptPin();

  virtual void SetPin(const xiiPin& pin) override;
  virtual bool UpdatePinColors(const xiiColorGammaUB* pOverwriteColor = nullptr) override;
};

class xiiQtVisualScriptConnection : public xiiQtConnection
{
public:
  xiiQtVisualScriptConnection();
};

class xiiQtVisualScriptNode : public xiiQtNode
{
public:
  xiiQtVisualScriptNode();

  virtual void UpdateState() override;
};

class xiiQtVisualScriptNodeScene : public xiiQtNodeScene
{
  Q_OBJECT

public:
  xiiQtVisualScriptNodeScene(QObject* parent = nullptr);
  ~xiiQtVisualScriptNodeScene();

  virtual void SetDocumentNodeManager(const xiiDocumentNodeManager* pManager);

private:
  void DeductedTypeChangedHandler(const xiiDocumentObject* pObject);
};
