/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <QAbstractItemModel>
#include <QIcon>

class xiiPropertyAnimAssetDocument;
struct xiiDocumentObjectPropertyEvent;
struct xiiDocumentObjectStructureEvent;
class xiiPropertyAnimationTrack;

struct xiiQtPropertyAnimModelTreeEntry
{
  xiiString                  m_sPathToItem;
  xiiInt32                   m_iParent       = -1;
  xiiUInt16                  m_uiOwnRowIndex = 0;
  xiiPropertyAnimationTrack* m_pTrack        = nullptr;
  xiiInt32                   m_iTrackIdx     = -1;
  xiiString                  m_sDisplay;
  xiiDynamicArray<xiiInt32>  m_Children;
  QIcon                      m_Icon;

  bool operator==(const xiiQtPropertyAnimModelTreeEntry& rhs) const
  {
    return (m_iParent == rhs.m_iParent) && (m_uiOwnRowIndex == rhs.m_uiOwnRowIndex) && (m_pTrack == rhs.m_pTrack) && (m_iTrackIdx == rhs.m_iTrackIdx) && (m_sDisplay == rhs.m_sDisplay) && (m_Children == rhs.m_Children);
  }
};

class xiiQtPropertyAnimModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  xiiQtPropertyAnimModel(xiiPropertyAnimAssetDocument* pDocument, QObject* pParent);
  ~xiiQtPropertyAnimModel();

  enum UserRoles
  {
    TrackPtr = Qt::UserRole + 1,
    TreeItem = Qt::UserRole + 2,
    TrackIdx = Qt::UserRole + 3,
    Path     = Qt::UserRole + 4,
  };

  const xiiDeque<xiiQtPropertyAnimModelTreeEntry>& GetAllEntries() const { return m_AllEntries[m_iInUse]; }

private Q_SLOTS:
  void onBuildMappingTriggered();

public: // QAbstractItemModel interface
  virtual QVariant      data(const QModelIndex& index, int iRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QModelIndex   index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex   parent(const QModelIndex& index) const override;
  virtual int           rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int           columnCount(const QModelIndex& parent = QModelIndex()) const override;

private:
  void DocumentStructureEventHandler(const xiiDocumentObjectStructureEvent& e);
  void DocumentPropertyEventHandler(const xiiDocumentObjectPropertyEvent& e);
  void TriggerBuildMapping();
  void BuildMapping();
  void BuildMapping(xiiInt32 iToUse);
  void BuildMapping(xiiInt32 iToUse, xiiInt32 iTrackIdx, xiiPropertyAnimationTrack* pTrack, xiiDynamicArray<xiiInt32>& treeItems, xiiInt32 iParentEntry, const char* szPath);

  bool                                      m_bBuildMappingQueued = false;
  xiiInt32                                  m_iInUse              = 0;
  xiiDynamicArray<xiiInt32>                 m_TopLevelEntries[2];
  xiiDeque<xiiQtPropertyAnimModelTreeEntry> m_AllEntries[2];

  xiiPropertyAnimAssetDocument* m_pAssetDoc = nullptr;
};
