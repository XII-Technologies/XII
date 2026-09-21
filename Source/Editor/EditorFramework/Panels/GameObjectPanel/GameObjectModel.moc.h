/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class xiiSceneDocument;


/// Custom delegate for game objects, used in xiiQtGameObjectWidget.
///
/// Renders additional icons to display stats.
class XII_EDITORFRAMEWORK_DLL xiiQtGameObjectDelegate : public xiiQtItemDelegate
{
  Q_OBJECT
public:
  xiiQtGameObjectDelegate(QObject* pParent, xiiGameObjectDocument* pDocument);
  virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const override;
  virtual bool helpEvent(QHelpEvent* pEvent, QAbstractItemView* pView, const QStyleOptionViewItem& option, const QModelIndex& index) override;
  static QRect GetHiddenIconRect(const QStyleOptionViewItem& opt);
  static QRect GetActiveParentIconRect(const QStyleOptionViewItem& opt);

  xiiGameObjectDocument* m_pDocument = nullptr;
};

class XII_EDITORFRAMEWORK_DLL xiiQtGameObjectAdapter : public xiiQtNameableAdapter
{
  Q_OBJECT;

public:
  enum UserRoles
  {
    HiddenRole       = Qt::UserRole + 0,
    ActiveParentRole = Qt::UserRole + 1,
  };

  xiiQtGameObjectAdapter(xiiDocumentObjectManager* pObjectManager, xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>* pObjectMetaData = nullptr, xiiObjectMetaData<xiiUuid, xiiGameObjectMetaData>* pGameObjectMetaData = nullptr);
  ~xiiQtGameObjectAdapter();
  virtual QVariant data(const xiiDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;
  virtual bool     setData(const xiiDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const override;

public:
  void DocumentObjectMetaDataEventHandler(const xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>::EventData& e);
  void GameObjectMetaDataEventHandler(const xiiObjectMetaData<xiiUuid, xiiGameObjectMetaData>::EventData& e);

protected:
  xiiDocumentObjectManager*                              m_pObjectManager      = nullptr;
  xiiGameObjectDocument*                                 m_pGameObjectDocument = nullptr;
  xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>* m_pObjectMetaData     = nullptr;
  xiiObjectMetaData<xiiUuid, xiiGameObjectMetaData>*     m_pGameObjectMetaData = nullptr;
  xiiEventSubscriptionID                                 m_GameObjectMetaDataSubscription;
  xiiEventSubscriptionID                                 m_DocumentObjectMetaDataSubscription;
};

class XII_EDITORFRAMEWORK_DLL xiiQtGameObjectModel : public xiiQtDocumentTreeModel
{
  Q_OBJECT

public:
  xiiQtGameObjectModel(const xiiDocumentObjectManager* pObjectManager, const xiiUuid& root = xiiUuid());
  ~xiiQtGameObjectModel();
};
