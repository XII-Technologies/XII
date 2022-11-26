#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>

xiiQtJointAdapter::xiiQtJointAdapter(const xiiSkeletonAssetDocument* pDocument) :
  xiiQtNamedAdapter(pDocument->GetObjectManager(), xiiGetStaticRTTI<xiiEditableSkeletonJoint>(), "Children", "Name"), m_pDocument(pDocument)
{
}

xiiQtJointAdapter::~xiiQtJointAdapter() {}

QVariant xiiQtJointAdapter::data(const xiiDocumentObject* pObject, int row, int column, int role) const
{
  switch (role)
  {
    case Qt::DecorationRole:
    {
      QIcon icon = xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveY.png"); // Giv ICon Plxii!
      return icon;
    }
    break;
  }
  return xiiQtNamedAdapter::data(pObject, row, column, role);
}
