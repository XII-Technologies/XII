/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>

xiiQtAssetStatusIndicator::xiiQtAssetStatusIndicator(xiiAssetDocument* pDoc, QWidget* pParent) :
  QFrame(pParent)
{
  m_pAsset = pDoc;

  setContentsMargins(0, 0, 0, 0);
  setLayout(new QHBoxLayout());

  layout()->setContentsMargins(0, 0, 0, 0);

  m_pLabel = new QPushButton();
  m_pLabel->setFlat(true);
  connect(m_pLabel, &QPushButton::clicked, this, &xiiQtAssetStatusIndicator::onClick);

  layout()->addWidget(m_pLabel);

  m_pAsset->m_EventsOne.AddEventHandler(xiiMakeDelegate(&xiiQtAssetStatusIndicator::DocumentEventHandler, this));
  xiiAssetCurator::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtAssetStatusIndicator::AssetEventHandler, this));

  UpdateDisplay();
}

xiiQtAssetStatusIndicator::~xiiQtAssetStatusIndicator()
{
  m_pAsset->m_EventsOne.RemoveEventHandler(xiiMakeDelegate(&xiiQtAssetStatusIndicator::DocumentEventHandler, this));
  xiiAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtAssetStatusIndicator::AssetEventHandler, this));
}

void xiiQtAssetStatusIndicator::DocumentEventHandler(const xiiDocumentEvent& e)
{
  if (e.m_Type == xiiDocumentEvent::Type::ModifiedChanged)
  {
    UpdateDisplay();
  }
}

void xiiQtAssetStatusIndicator::AssetEventHandler(const xiiAssetCuratorEvent& e)
{
  if (e.m_AssetGuid == m_pAsset->GetGuid())
  {
    if (e.m_Type == xiiAssetCuratorEvent::Type::AssetUpdated)
    {
      UpdateDisplay();
    }
  }
}

void xiiQtAssetStatusIndicator::UpdateDisplay()
{
  // states:
  // all good
  // document modified - but live preview
  // document modified - no preview
  // saved - waiting for background transform
  // saved - needs manual transform (bg off)
  // transform error

  if (m_pAsset->IsModified())
  {
    auto       flags            = m_pAsset->GetAssetFlags();
    const bool bTransformOnSave = flags.IsSet(xiiAssetDocumentFlags::AutoTransformOnSave);
    const bool bBgRunning       = xiiAssetProcessor::GetSingleton()->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Running;

    // no flag for live preview available (ignore)

    if (bTransformOnSave || bBgRunning)
    {
      m_pLabel->setText("Asset Modified: Click to Save");
      m_Action = Action::Save;
    }
    else
    {
      m_pLabel->setText("Asset Modified: Click to Transform");
      m_Action = Action::Transform;
    }

    m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/Attention.svg"));
  }
  else
  {
    auto assetInfo = xiiAssetCurator::GetSingleton()->GetSubAsset(m_pAsset->GetGuid());
    switch (assetInfo->m_pAssetInfo->m_TransformState)
    {
      case xiiAssetInfo::TransformState::UpToDate:
      case xiiAssetInfo::TransformState::NeedsThumbnail:
      {
        m_pLabel->setText("Asset State: All Good");
        m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/AssetOk.svg"));
        m_Action = Action::None;
        break;
      }

      case xiiAssetInfo::TransformState::NeedsImport:
      case xiiAssetInfo::TransformState::NeedsTransform:
      {
        const bool bBgRunning = xiiAssetProcessor::GetSingleton()->GetProcessTaskState() == xiiAssetProcessor::ProcessTaskState::Running;

        if (bBgRunning)
        {
          m_pLabel->setText("Waiting for Transform: Click to Force");
          m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/AssetNeedsTransform.svg"));
        }
        else
        {
          m_pLabel->setText("Asset Changed: Click to Transform");
          m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/Attention.svg"));
        }

        m_Action = Action::Transform;
        break;
      }

      case xiiAssetInfo::TransformState::TransformError:
      case xiiAssetInfo::TransformState::MissingTransformDependency:
      case xiiAssetInfo::TransformState::MissingThumbnailDependency:
      case xiiAssetInfo::TransformState::MissingPackageDependency:
      case xiiAssetInfo::TransformState::CircularDependency:
        m_pLabel->setText("Asset Error: Click for Details");
        m_pLabel->setIcon(QIcon(":/EditorFramework/Icons/AssetFailedTransform.svg"));
        m_Action = Action::ShowErrors;
        break;

      default:
        break;
    }
  }
}

void xiiQtAssetStatusIndicator::onClick(bool)
{
  switch (m_Action)
  {
    case Action::Save:
      m_pAsset->SaveDocument().IgnoreResult();
      break;

    case Action::None:
      // also transform in this case
      [[fallthrough]];

    case Action::Transform:
      m_pAsset->TransformAsset(xiiTransformFlags::TriggeredManually | xiiTransformFlags::ForceTransform);
      break;

    case Action::ShowErrors:
    {
      auto assetInfo = xiiAssetCurator::GetSingleton()->GetSubAsset(m_pAsset->GetGuid());

      xiiStringBuilder output;
      output.Set("Asset transform failed.\n\n");

      if (!assetInfo->m_pAssetInfo->m_LogEntries.IsEmpty())
      {
        output.Append("Errors:\n\n");

        for (const xiiLogEntry& logEntry : assetInfo->m_pAssetInfo->m_LogEntries)
        {
          output.AppendFormat("{}\n", logEntry.m_sMsg);
        }
      }

      auto getNiceName = [](const xiiString& sDep) -> xiiStringBuilder {
        if (xiiConversionUtils::IsStringUuid(sDep))
        {
          xiiUuid guid         = xiiConversionUtils::ConvertStringToUuid(sDep);
          auto    assetInfoDep = xiiAssetCurator::GetSingleton()->GetSubAsset(guid);
          if (assetInfoDep)
          {
            return assetInfoDep->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
          }

          xiiUInt64 uiLow;
          xiiUInt64 uiHigh;
          guid.GetValues(uiLow, uiHigh);
          xiiStringBuilder sTmp;
          sTmp.SetFormat("{} - u4{{},{}}", sDep, uiLow, uiHigh);

          return sTmp;
        }

        return sDep;
      };

      xiiSet<xiiString> missingDeps;

      if (!assetInfo->m_pAssetInfo->m_MissingTransformDeps.IsEmpty())
      {
        for (const xiiString& dep : assetInfo->m_pAssetInfo->m_MissingTransformDeps)
        {
          missingDeps.Insert(getNiceName(dep));
        }
      }

      if (!assetInfo->m_pAssetInfo->m_MissingPackageDeps.IsEmpty())
      {
        for (const xiiString& dep : assetInfo->m_pAssetInfo->m_MissingPackageDeps)
        {
          missingDeps.Insert(getNiceName(dep));
        }
      }

      if (!assetInfo->m_pAssetInfo->m_MissingThumbnailDeps.IsEmpty())
      {
        for (const xiiString& dep : assetInfo->m_pAssetInfo->m_MissingThumbnailDeps)
        {
          missingDeps.Insert(getNiceName(dep));
        }
      }

      if (!missingDeps.IsEmpty())
      {
        output.Append("Missing Dependencies:\n\n");

        for (const xiiString& dep : missingDeps)
        {
          output.AppendFormat("{}\n", dep);
        }
      }

      xiiQtUiServices::GetSingleton()->MessageBoxInformation(output);

      break;
    }
  }
}
