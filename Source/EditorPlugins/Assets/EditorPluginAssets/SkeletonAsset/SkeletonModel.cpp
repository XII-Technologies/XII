#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>

xiiQtJointAdapter::xiiQtJointAdapter(const xiiSkeletonAssetDocument* pDocument) :
  xiiQtNamedAdapter(pDocument->GetObjectManager(), xiiGetStaticRTTI<xiiEditableSkeletonJoint>(), "Children", "Name"), m_pDocument(pDocument)
{
}

xiiQtJointAdapter::~xiiQtJointAdapter() = default;

QVariant xiiQtJointAdapter::data(const xiiDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  switch (iRole)
  {
    case Qt::DecorationRole:
    {
      QIcon icon = xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveY.svg"); // Giv ICon Plez!
      return icon;
    }
    break;
  }
  return xiiQtNamedAdapter::data(pObject, iRow, iColumn, iRole);
}
