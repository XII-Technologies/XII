/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <QAbstractItemModel>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiDragDropInfo;

/// Adapter that defines data for specific type in the xiiQtDocumentTreeModel.
///
/// Adapters are defined for a given type and define the property for child elements (needs to be array or set).
/// Furthermore they implement various model functions that will be redirected to it by the model for
/// objects of the given type.
class XII_EDITORFRAMEWORK_DLL xiiQtDocumentTreeModelAdapter : public QObject
{
  Q_OBJECT;

public:
  /// Constructor. If m_sChildProperty is empty, this type does not have children.
  xiiQtDocumentTreeModelAdapter(const xiiDocumentObjectManager* pTree, const xiiRTTI* pType, const char* szChildProperty);
  virtual const xiiRTTI*   GetType() const;
  virtual const xiiString& GetChildProperty() const;

  virtual QVariant      data(const xiiDocumentObject* pObject, int iRow, int iColumn, int iRole = Qt::DisplayRole) const = 0;
  virtual bool          setData(const xiiDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const;
  virtual Qt::ItemFlags flags(const xiiDocumentObject* pObject, int iRow, int iColumn) const;

Q_SIGNALS:
  void dataChanged(const xiiDocumentObject* pObject, QVector<int> roles);

protected:
  const xiiDocumentObjectManager* m_pTree = nullptr;
  const xiiRTTI*                  m_pType = nullptr;
  xiiString                       m_sChildProperty;
};

/// Convenience class that returns the typename as Qt::DisplayRole.
/// Use this for testing or for the document root that can't be seen and is just for defining the hierarchy.
///
/// Example:
/// xiiQtDummyAdapter(pDocument->GetObjectManager(), xiiGetStaticRTTI<xiiDocumentRoot>(), "Children");
class XII_EDITORFRAMEWORK_DLL xiiQtDummyAdapter : public xiiQtDocumentTreeModelAdapter
{
  Q_OBJECT;

public:
  xiiQtDummyAdapter(const xiiDocumentObjectManager* pTree, const xiiRTTI* pType, const char* szChildProperty);

  virtual QVariant data(const xiiDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;
};

/// Convenience class that implements getting the name via a property on the object.
class XII_EDITORFRAMEWORK_DLL xiiQtNamedAdapter : public xiiQtDocumentTreeModelAdapter
{
  Q_OBJECT;

public:
  xiiQtNamedAdapter(const xiiDocumentObjectManager* pTree, const xiiRTTI* pType, const char* szChildProperty, const char* szNameProperty);
  ~xiiQtNamedAdapter();
  virtual QVariant data(const xiiDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;

protected:
  virtual void TreePropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);

protected:
  xiiString m_sNameProperty;
};

/// Convenience class that implements setting the name via a property on the object.
class XII_EDITORFRAMEWORK_DLL xiiQtNameableAdapter : public xiiQtNamedAdapter
{
  Q_OBJECT;

public:
  xiiQtNameableAdapter(const xiiDocumentObjectManager* pTree, const xiiRTTI* pType, const char* szChildProperty, const char* szNameProperty);
  ~xiiQtNameableAdapter();
  virtual bool          setData(const xiiDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const override;
  virtual Qt::ItemFlags flags(const xiiDocumentObject* pObject, int iRow, int iColumn) const override;
};

/// Model that maps a document to a qt tree model.
///
/// Hierarchy is defined by xiiQtDocumentTreeModelAdapter that have to be added via AddAdapter.
class XII_EDITORFRAMEWORK_DLL xiiQtDocumentTreeModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  xiiQtDocumentTreeModel(const xiiDocumentObjectManager* pTree, const xiiUuid& root = xiiUuid());
  ~xiiQtDocumentTreeModel();

  const xiiDocumentObjectManager* GetDocumentTree() const { return m_pDocumentTree; }
  /// Adds an adapter. There can only be one adapter for any object type.
  /// Added adapters are taken ownership of by the model.
  void AddAdapter(xiiQtDocumentTreeModelAdapter* pAdapter);
  /// Returns the QModelIndex for the given object.
  /// Returned value is invalid if object is not mapped in model.
  QModelIndex ComputeModelIndex(const xiiDocumentObject* pObject) const;

  /// Enable drag&drop support, disabled by default.
  void SetAllowDragDrop(bool bAllow);

  static bool MoveObjects(const xiiDragDropInfo& info);

  /// Returns the ezDocumentObject that the index points to.
  const xiiDocumentObject* GetObject(const QModelIndex index) const;

public: // QAbstractItemModel
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& child) const override;

  virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;

  virtual QVariant data(const QModelIndex& index, int iRole = Qt::DisplayRole) const override;
  virtual bool     setData(const QModelIndex& index, const QVariant& value, int iRole) override;

  virtual Qt::DropActions supportedDropActions() const override;

  virtual bool        canDropMimeData(const QMimeData* pData, Qt::DropAction action, int iRow, int iColumn, const QModelIndex& parent) const override;
  virtual bool        dropMimeData(const QMimeData* pData, Qt::DropAction action, int iRow, int iColumn, const QModelIndex& parent) override;
  virtual QStringList mimeTypes() const override;
  virtual QMimeData*  mimeData(const QModelIndexList& indexes) const override;

protected:
  virtual void TreeEventHandler(const xiiDocumentObjectStructureEvent& e);

private:
  QModelIndex              ComputeParent(const xiiDocumentObject* pObject) const;
  xiiInt32                 ComputeIndex(const xiiDocumentObject* pObject) const;
  const xiiDocumentObject* GetRoot() const;
  bool                     IsUnderRoot(const xiiDocumentObject* pObject) const;

  const xiiQtDocumentTreeModelAdapter* GetAdapter(const xiiRTTI* pType) const;

protected:
  const xiiDocumentObjectManager*                              m_pDocumentTree = nullptr;
  const xiiUuid                                                m_Root;
  xiiHashTable<const xiiRTTI*, xiiQtDocumentTreeModelAdapter*> m_Adapters;
  bool                                                         m_bAllowDragDrop = false;
  xiiString                                                    m_sTargetContext = "scenetree";
};
