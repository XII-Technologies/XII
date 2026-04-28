/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginScene/Actions/LayerActions.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QToolTip>

xiiQtLayerAdapter::xiiQtLayerAdapter(xiiScene2Document* pDocument) :
  xiiQtDocumentTreeModelAdapter(pDocument->GetSceneObjectManager(), xiiGetStaticRTTI<xiiSceneLayer>(), nullptr)
{
  m_pSceneDocument = pDocument;
  m_pSceneDocument->m_LayerEvents.AddEventHandler(
    xiiMakeDelegate(&xiiQtLayerAdapter::LayerEventHandler, this), m_LayerEventUnsubscriber);

  xiiDocument::s_EventsAny.AddEventHandler(xiiMakeDelegate(&xiiQtLayerAdapter::DocumentEventHander, this), m_DocumentEventUnsubscriber);
}

xiiQtLayerAdapter::~xiiQtLayerAdapter()
{
  m_LayerEventUnsubscriber.Unsubscribe();
  m_DocumentEventUnsubscriber.Unsubscribe();
}

QVariant xiiQtLayerAdapter::data(const xiiDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  switch (iRole)
  {
    case UserRoles::LayerGuid:
    {
      xiiObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      xiiUuid                layerGuid = pAccessor->GetByName<xiiUuid>(pObject, "Layer");
      return QVariant::fromValue(layerGuid);
    }
    break;
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    {
      xiiObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      xiiUuid                layerGuid = pAccessor->GetByName<xiiUuid>(pObject, "Layer");
      // Use curator to get name in case the layer is unloaded and there is no document to query.
      const xiiAssetCurator::xiiLockedSubAsset subAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
      if (subAsset.isValid())
      {
        if (iRole == Qt::ToolTipRole)
        {
          return xiiMakeQString(subAsset->m_pAssetInfo->m_Path.GetAbsolutePath());
        }
        xiiStringBuilder sName   = subAsset->GetName();
        QString          sQtName = QString::fromUtf8(sName.GetData());
        if (xiiSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid))
        {
          if (pLayer->IsModified())
          {
            sQtName += "*";
          }
        }
        return sQtName;
      }
      else
      {
        return QStringLiteral("Layer guid not found");
      }
    }
    break;

    case Qt::DecorationRole:
    {
      return xiiQtUiServices::GetCachedIconResource(":/EditorPluginScene/Icons/Layer.svg");
    }
    break;
    case Qt::ForegroundRole:
    {
      xiiObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      xiiUuid                layerGuid = pAccessor->GetByName<xiiUuid>(pObject, "Layer");
      if (!m_pSceneDocument->IsLayerLoaded(layerGuid))
      {
        return QVariant();
      }
    }
    break;
    case Qt::FontRole:
    {
      QFont                  font;
      xiiObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      xiiUuid                layerGuid = pAccessor->GetByName<xiiUuid>(pObject, "Layer");
      if (m_pSceneDocument->GetActiveLayer() == layerGuid)
        font.setBold(true);
      return font;
    }
    break;
  }

  return QVariant();
}

bool xiiQtLayerAdapter::setData(const xiiDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const
{
  return false;
}

void xiiQtLayerAdapter::LayerEventHandler(const xiiScene2LayerEvent& e)
{
  switch (e.m_Type)
  {
    case xiiScene2LayerEvent::Type::LayerUnloaded:
    case xiiScene2LayerEvent::Type::LayerLoaded:
    {
      QVector<int> v;
      v.push_back(Qt::DisplayRole);
      v.push_back(Qt::ForegroundRole);
      Q_EMIT dataChanged(m_pSceneDocument->GetLayerObject(e.m_layerGuid), v);
    }
    break;
    case xiiScene2LayerEvent::Type::ActiveLayerChanged:
    {
      QVector<int> v;
      v.push_back(Qt::FontRole);
      if (auto pObject = m_pSceneDocument->GetLayerObject(m_CurrentActiveLayer))
      {
        Q_EMIT dataChanged(pObject, v);
      }
      Q_EMIT dataChanged(m_pSceneDocument->GetLayerObject(e.m_layerGuid), v);
      m_CurrentActiveLayer = e.m_layerGuid;
    }
    default:
      break;
  }
}

void xiiQtLayerAdapter::DocumentEventHander(const xiiDocumentEvent& e)
{
  if (e.m_Type == xiiDocumentEvent::Type::DocumentSaved || e.m_Type == xiiDocumentEvent::Type::ModifiedChanged)
  {
    const xiiDocumentObject* pLayerObj = m_pSceneDocument->GetLayerObject(e.m_pDocument->GetGuid());
    if (pLayerObj)
    {
      QVector<int> v;
      v.push_back(Qt::DisplayRole);
      v.push_back(Qt::ForegroundRole);
      Q_EMIT dataChanged(pLayerObj, v);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

xiiQtLayerDelegate::xiiQtLayerDelegate(QObject* pParent, xiiScene2Document* pDocument) :
  xiiQtItemDelegate(pParent), m_pDocument(pDocument)
{
}

bool xiiQtLayerDelegate::mousePressEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  const QRect visibleRect = GetVisibleIconRect(option);
  const QRect loadedRect  = GetLoadedIconRect(option);
  if (pEvent->button() == Qt::MouseButton::LeftButton && (visibleRect.contains(pEvent->position().toPoint()) || loadedRect.contains(pEvent->position().toPoint())))
  {
    m_bPressed = true;
    pEvent->accept();
    return true;
  }
  return xiiQtItemDelegate::mousePressEvent(pEvent, option, index);
}

bool xiiQtLayerDelegate::mouseReleaseEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (m_bPressed)
  {
    const QRect visibleRect = GetVisibleIconRect(option);
    const QRect loadedRect  = GetLoadedIconRect(option);
    if (visibleRect.contains(pEvent->position().toPoint()))
    {
      const xiiUuid layerGuid = index.data(xiiQtLayerAdapter::UserRoles::LayerGuid).value<xiiUuid>();
      const bool    bVisible  = !m_pDocument->IsLayerVisible(layerGuid);
      m_pDocument->SetLayerVisible(layerGuid, bVisible).LogFailure();
    }
    else if (loadedRect.contains(pEvent->position().toPoint()))
    {
      const xiiUuid layerGuid = index.data(xiiQtLayerAdapter::UserRoles::LayerGuid).value<xiiUuid>();
      if (layerGuid != m_pDocument->GetGuid())
      {
        xiiLayerAction::ToggleLayerLoaded(m_pDocument, layerGuid);
      }
    }
    m_bPressed = false;
    pEvent->accept();
    return true;
  }
  return xiiQtItemDelegate::mouseReleaseEvent(pEvent, option, index);
}

bool xiiQtLayerDelegate::mouseMoveEvent(QMouseEvent* pEvent, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (m_bPressed)
  {
    return true;
  }
  return xiiQtItemDelegate::mouseMoveEvent(pEvent, option, index);
}

void xiiQtLayerDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  xiiQtItemDelegate::paint(pPainter, opt, index);

  {
    const xiiUuid layerGuid = index.data(xiiQtLayerAdapter::UserRoles::LayerGuid).value<xiiUuid>();
    if (layerGuid.IsValid())
    {
      {
        const QRect thumbnailRect = GetVisibleIconRect(opt);
        const bool  bVisible      = m_pDocument->IsLayerVisible(layerGuid);

        if (bVisible)
        {
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/ObjectsVisible.svg").paint(pPainter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
        }
        else
        {
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/ObjectsHidden.svg").paint(pPainter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
        }
      }

      if (layerGuid != m_pDocument->GetGuid())
      {
        const QRect thumbnailRect = GetLoadedIconRect(opt);
        const bool  bLoaded       = m_pDocument->IsLayerLoaded(layerGuid);

        if (bLoaded)
        {
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginScene/Icons/LayerLoaded.svg").paint(pPainter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
        }
        else
        {
          xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginScene/Icons/LayerUnloaded.svg").paint(pPainter, thumbnailRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
        }
      }
    }
  }
}

QSize xiiQtLayerDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  return xiiQtItemDelegate::sizeHint(opt, index);
}

bool xiiQtLayerDelegate::helpEvent(QHelpEvent* pEvent, QAbstractItemView* pView, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  const xiiUuid layerGuid = index.data(xiiQtLayerAdapter::UserRoles::LayerGuid).value<xiiUuid>();
  if (layerGuid.IsValid())
  {
    const QRect visibleRect = GetVisibleIconRect(option);
    const QRect loadedRect  = GetLoadedIconRect(option);
    if (visibleRect.contains(pEvent->pos()))
    {
      const bool bVisible = m_pDocument->IsLayerVisible(layerGuid);
      QToolTip::showText(pEvent->globalPos(), bVisible ? "Hide Layer" : "Show Layer", pView);
      return true;
    }
    else if (loadedRect.contains(pEvent->pos()))
    {
      const bool bLoaded = m_pDocument->IsLayerLoaded(layerGuid);
      QToolTip::showText(pEvent->globalPos(), bLoaded ? "Unload Layer" : "Load Layer", pView);
      return true;
    }
  }
  return xiiQtItemDelegate::helpEvent(pEvent, pView, option, index);
}

QRect xiiQtLayerDelegate::GetVisibleIconRect(const QStyleOptionViewItem& opt)
{
  return opt.rect.adjusted(opt.rect.width() - opt.rect.height(), 0, 0, 0);
}

QRect xiiQtLayerDelegate::GetLoadedIconRect(const QStyleOptionViewItem& opt)
{
  return opt.rect.adjusted(opt.rect.width() - opt.rect.height() * 2, 0, -opt.rect.height(), 0);
}

//////////////////////////////////////////////////////////////////////////

xiiQtLayerModel::xiiQtLayerModel(xiiScene2Document* pDocument) :
  xiiQtDocumentTreeModel(pDocument->GetSceneObjectManager(), pDocument->GetSettingsObject()->GetGuid()), m_pDocument(pDocument)
{
  m_sTargetContext = "layertree";
}
