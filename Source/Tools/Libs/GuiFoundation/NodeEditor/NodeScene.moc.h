#pragma once

#include <Foundation/Containers/Map.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class xiiQtNode;
class xiiQtPin;
class xiiQtConnection;
struct xiiSelectionManagerEvent;

class XII_GUIFOUNDATION_DLL xiiQtNodeScene : public QGraphicsScene
{
  Q_OBJECT
public:
  enum Type
  {
    Node = QGraphicsItem::UserType + 1,
    Pin,
    Connection
  };

  explicit xiiQtNodeScene(QObject* pParent = nullptr);
  ~xiiQtNodeScene();

  virtual void                  InitScene(const xiiDocumentNodeManager* pManager);
  const xiiDocumentNodeManager* GetDocumentNodeManager() const;
  const xiiDocument*            GetDocument() const;

  static xiiRttiMappedObjectFactory<xiiQtNode>&       GetNodeFactory();
  static xiiRttiMappedObjectFactory<xiiQtPin>&        GetPinFactory();
  static xiiRttiMappedObjectFactory<xiiQtConnection>& GetConnectionFactory();
  static xiiVec2                                      GetLastMouseInteractionPos() { return s_vLastMouseInteraction; }

  struct ConnectionStyle
  {
    using StorageType = xiiUInt32;

    enum Enum
    {
      BezierCurve,
      StraightLine,

      Default = BezierCurve
    };
  };

  void                     SetConnectionStyle(xiiEnum<ConnectionStyle> style);
  xiiEnum<ConnectionStyle> GetConnectionStyle() const { return m_ConnectionStyle; }

  struct ConnectionDecorationFlags
  {
    using StorageType = xiiUInt32;

    enum Enum
    {
      DirectionArrows = XII_BIT(0), ///< Draw an arrow to indicate the connection's direction. Only works with straight lines atm.

      Default = 0
    };

    struct Bits
    {
      StorageType DirectionArrows : 1;
    };
  };

  void                                   SetConnectionDecorationFlags(xiiBitflags<ConnectionDecorationFlags> flags);
  xiiBitflags<ConnectionDecorationFlags> GetConnectionDecorationFlags() const { return m_ConnectionDecorationFlags; }

protected:
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent) override;
  virtual void keyPressEvent(QKeyEvent* event) override;

private:
  void Clear();
  void CreateQtNode(const xiiDocumentObject* pObject);
  void DeleteQtNode(const xiiDocumentObject* pObject);
  void CreateQtConnection(const xiiDocumentObject* pObject);
  void DeleteQtConnection(const xiiDocumentObject* pObject);
  void RecreateQtPins(const xiiDocumentObject* pObject);
  void CreateNodeObject(const xiiRTTI* pRtti);
  void NodeEventsHandler(const xiiDocumentNodeManagerEvent& e);
  void PropertyEventsHandler(const xiiDocumentObjectPropertyEvent& e);
  void SelectionEventsHandler(const xiiSelectionManagerEvent& e);
  void GetSelectedNodes(xiiDeque<xiiQtNode*>& selection) const;
  void MarkupConnectablePins(xiiQtPin* pSourcePin);
  void ResetConnectablePinMarkup();
  void OpenSearchMenu(QPoint screenPos);

protected:
  virtual xiiStatus RemoveNode(xiiQtNode* pNode);
  virtual void      RemoveSelectedNodesAction();
  virtual void      ConnectPinsAction(const xiiPin& sourcePin, const xiiPin& targetPin);
  virtual void      DisconnectPinsAction(xiiQtConnection* pConnection);
  virtual void      DisconnectPinsAction(xiiQtPin* pPin);

private Q_SLOTS:
  void OnMenuItemTriggered(const QString& sName, const QVariant& variant);
  void OnSelectionChanged();

private:
  static xiiRttiMappedObjectFactory<xiiQtNode>       s_NodeFactory;
  static xiiRttiMappedObjectFactory<xiiQtPin>        s_PinFactory;
  static xiiRttiMappedObjectFactory<xiiQtConnection> s_ConnectionFactory;

protected:
  const xiiDocumentNodeManager* m_pManager = nullptr;

  xiiMap<const xiiDocumentObject*, xiiQtNode*>       m_Nodes;
  xiiMap<const xiiDocumentObject*, xiiQtConnection*> m_Connections;

private:
  bool                                   m_bIgnoreSelectionChange = false;
  xiiQtPin*                              m_pStartPin              = nullptr;
  xiiQtConnection*                       m_pTempConnection        = nullptr;
  xiiDeque<const xiiDocumentObject*>     m_Selection;
  xiiVec2                                m_vMousePos = xiiVec2::MakeZero();
  QString                                m_sContextMenuSearchText;
  xiiDynamicArray<const xiiQtPin*>       m_ConnectablePins;
  xiiEnum<ConnectionStyle>               m_ConnectionStyle;
  xiiBitflags<ConnectionDecorationFlags> m_ConnectionDecorationFlags;

  static xiiVec2 s_vLastMouseInteraction;
};

XII_DECLARE_FLAGS_OPERATORS(xiiQtNodeScene::ConnectionDecorationFlags);
