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
  virtual QVariant data(const xiiDocumentObject* pObject, int row, int column, int role) const override;

private:
  const xiiSkeletonAssetDocument* m_pDocument;
};
