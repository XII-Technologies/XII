/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <QAbstractItemModel>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiSkeletonAssetDocument;

class xiiQtJointAdapter : public xiiQtNamedAdapter
{
  Q_OBJECT;

public:
  xiiQtJointAdapter(const xiiSkeletonAssetDocument* pDocument);
  ~xiiQtJointAdapter();
  virtual QVariant data(const xiiDocumentObject* pObject, int iRow, int iColumn, int iRole) const override;

private:
  const xiiSkeletonAssetDocument* m_pDocument;
};
