/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Dialogs/AssetProfilesDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Platform/PlatformDescription.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class xiiAssetProfilesObjectManager : public xiiDocumentObjectManager
{
public:
  virtual void GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const override { ref_types.PushBack(xiiGetStaticRTTI<xiiPlatformProfile>()); }
};

class xiiAssetProfilesDocument : public xiiDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAssetProfilesDocument, xiiDocument);

public:
  xiiAssetProfilesDocument(xiiStringView sDocumentPath) :
    xiiDocument(sDocumentPath, XII_DEFAULT_NEW(xiiAssetProfilesObjectManager))
  {
  }

public:
  virtual xiiDocumentInfo* CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }
};

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAssetProfilesDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiQtAssetConfigAdapter : public xiiQtNameableAdapter
{
public:
  xiiQtAssetConfigAdapter(const xiiQtAssetProfilesDlg* pDialog, const xiiDocumentObjectManager* pTree, const xiiRTTI* pType) :
    xiiQtNameableAdapter(pTree, pType, "", "Name")
  {
    m_pDialog = pDialog;
  }

  virtual QVariant data(const xiiDocumentObject* pObject, int iRow, int iColumn, int iRole) const override
  {
    if (iColumn == 0)
    {
      if (iRole == Qt::DecorationRole)
      {
        const xiiString sTargetPlatform = pObject->GetTypeAccessor().GetValue("TargetPlatform").ConvertTo<xiiString>();

        const xiiStringBuilder sIconName(":Platforms/Icons/Platform", sTargetPlatform, ".svg");

        return xiiQtUiServices::GetSingleton()->GetCachedIconResource(sIconName);
      }

      if (iRole == Qt::DisplayRole)
      {
        QString name = xiiQtNameableAdapter::data(pObject, iRow, iColumn, iRole).toString();

        if (iRow == xiiAssetCurator::GetSingleton()->GetActiveAssetProfileIndex())
        {
          name += " (active)";
        }
        else if (iRow == m_pDialog->m_uiActiveConfig)
        {
          name += " (switch to)";
        }

        return name;
      }
    }

    return xiiQtNameableAdapter::data(pObject, iRow, iColumn, iRole);
  }

private:
  const xiiQtAssetProfilesDlg* m_pDialog = nullptr;
};

xiiQtAssetProfilesDlg::xiiQtAssetProfilesDlg(QWidget* pParent) :
  QDialog(pParent)
{
  setupUi(this);

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  // do not allow to delete or rename the first item
  DeleteButton->setEnabled(false);
  RenameButton->setEnabled(false);

  {
    auto& platEnum = xiiDynamicStringEnum::CreateDynamicEnum("TargetPlatformNames");
    platEnum.Clear();

    for (auto pDesc = xiiPlatformDescription::GetFirstInstance(); pDesc != nullptr; pDesc = pDesc->GetNextInstance())
    {
      platEnum.AddValidValue(pDesc->GetName(), true);
    }
  }

  m_pDocument = XII_DEFAULT_NEW(xiiAssetProfilesDocument, "<none>");
  m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtAssetProfilesDlg::SelectionEventHandler, this));

  std::unique_ptr<xiiQtDocumentTreeModel> pModel(new xiiQtDocumentTreeModel(m_pDocument->GetObjectManager()));
  pModel->AddAdapter(new xiiQtDummyAdapter(m_pDocument->GetObjectManager(), xiiGetStaticRTTI<xiiDocumentRoot>(), "Children"));
  pModel->AddAdapter(new xiiQtAssetConfigAdapter(this, m_pDocument->GetObjectManager(), xiiPlatformProfile::GetStaticRTTI()));

  Tree->Initialize(m_pDocument, std::move(pModel));
  Tree->SetAllowDragDrop(false);
  Tree->SetAllowDeleteObjects(false);
  Tree->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  Tree->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

  connect(Tree, &QTreeView::doubleClicked, this, &xiiQtAssetProfilesDlg::OnItemDoubleClicked);

  AllAssetProfilesToObject();

  Properties->SetDocument(m_pDocument);

  auto& rootChildArray = m_pDocument->GetObjectManager()->GetRootObject()->GetChildren();

  if (!rootChildArray.IsEmpty())
  {
    m_pDocument->GetSelectionManager()->SetSelection(rootChildArray[xiiAssetCurator::GetSingleton()->GetActiveAssetProfileIndex()]);
  }
}

xiiQtAssetProfilesDlg::~xiiQtAssetProfilesDlg()
{
  delete Tree;
  Tree = nullptr;

  delete Properties;
  Properties = nullptr;

  XII_DEFAULT_DELETE(m_pDocument);
}

xiiUuid xiiQtAssetProfilesDlg::NativeToObject(xiiPlatformProfile* pProfile)
{
  const xiiRTTI* pType = pProfile->GetDynamicRTTI();
  // Write properties to graph.
  xiiAbstractObjectGraph  graph;
  xiiRttiConverterContext context;
  xiiRttiConverterWriter  conv(&graph, &context, true, true);

  const xiiUuid guid = xiiUuid::MakeUuid();
  context.RegisterObject(guid, pType, pProfile);
  xiiAbstractObjectNode* pNode = conv.AddObjectToGraph(pType, pProfile, "root");

  // Read from graph and write into matching document object.
  auto               pRoot   = m_pDocument->GetObjectManager()->GetRootObject();
  xiiDocumentObject* pObject = m_pDocument->GetObjectManager()->CreateObject(pType);
  m_pDocument->GetObjectManager()->AddObject(pObject, pRoot, "Children", -1);

  xiiDocumentObjectConverterReader objectConverter(&graph, m_pDocument->GetObjectManager(), xiiDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  objectConverter.ApplyPropertiesToObject(pNode, pObject);

  return pObject->GetGuid();
}

void xiiQtAssetProfilesDlg::ObjectToNative(xiiUuid objectGuid, xiiPlatformProfile* pProfile)
{
  xiiDocumentObject* pObject = m_pDocument->GetObjectManager()->GetObject(objectGuid);
  const xiiRTTI*     pType   = pObject->GetTypeAccessor().GetType();

  // Write object to graph.
  xiiAbstractObjectGraph graph;
  auto                   filter = [](const xiiDocumentObject*, const xiiAbstractProperty* pProp) -> bool {
    if (pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
      return false;
    return true;
  };
  xiiDocumentObjectConverterWriter objectConverter(&graph, m_pDocument->GetObjectManager(), filter);
  xiiAbstractObjectNode*           pNode = objectConverter.AddObjectToGraph(pObject, "root");

  // Read from graph and write to native object.
  xiiRttiConverterContext context;
  xiiRttiConverterReader  conv(&graph, &context);

  conv.ApplyPropertiesToObject(pNode, pType, pProfile);
}


void xiiQtAssetProfilesDlg::SelectionEventHandler(const xiiSelectionManagerEvent& e)
{
  const auto& selection = m_pDocument->GetSelectionManager()->GetSelection();

  const bool bAllowModification = !selection.IsEmpty() && (selection[0] != m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  DeleteButton->setEnabled(bAllowModification);
  RenameButton->setEnabled(bAllowModification);
}

void xiiQtAssetProfilesDlg::on_ButtonOk_clicked()
{
  ApplyAllChanges();

  // SaveAssetProfileRuntimeConfig
  for (xiiUInt32 i = 0; i < xiiAssetCurator::GetSingleton()->GetNumAssetProfiles(); ++i)
  {
    xiiStringBuilder sProfileRuntimeDataFile;

    xiiPlatformProfile* pProfile = xiiAssetCurator::GetSingleton()->GetAssetProfile(i);

    sProfileRuntimeDataFile.Set(":project/RuntimeConfigs/", pProfile->GetConfigName(), ".xiiProfile");

    pProfile->SaveForRuntime(sProfileRuntimeDataFile).IgnoreResult();
  }

  accept();

  xiiAssetCurator::GetSingleton()->SaveAssetProfiles().IgnoreResult();
}

void xiiQtAssetProfilesDlg::on_ButtonCancel_clicked()
{
  m_uiActiveConfig = xiiAssetCurator::GetSingleton()->GetActiveAssetProfileIndex();
  reject();
}

void xiiQtAssetProfilesDlg::OnItemDoubleClicked(QModelIndex idx)
{
  if (m_uiActiveConfig == idx.row())
    return;

  const QModelIndex oldIdx = Tree->model()->index(m_uiActiveConfig, 0);

  m_uiActiveConfig = idx.row();

  QVector<int> roles;
  roles.push_back(Qt::DisplayRole);
  Tree->model()->dataChanged(idx, idx, roles);
  Tree->model()->dataChanged(oldIdx, oldIdx, roles);
}

bool xiiQtAssetProfilesDlg::CheckProfileNameUniqueness(const char* szName)
{
  if (xiiStringUtils::IsNullOrEmpty(szName))
  {
    xiiQtUiServices::GetSingleton()->MessageBoxInformation("Empty strings are not allowed as profile names.");
    return false;
  }

  if (!xiiStringUtils::IsValidIdentifierName(szName))
  {
    xiiQtUiServices::GetSingleton()->MessageBoxInformation("Profile names may only contain characters, digits and underscores.");
    return false;
  }

  const auto& objects = m_pDocument->GetObjectManager()->GetRootObject()->GetChildren();
  for (const xiiDocumentObject* pObject : objects)
  {
    if (pObject->GetTypeAccessor().GetValue("Name").ConvertTo<xiiString>().IsEqual_NoCase(szName))
    {
      xiiQtUiServices::GetSingleton()->MessageBoxInformation("A profile with this name already exists.");
      return false;
    }
  }

  return true;
}

bool xiiQtAssetProfilesDlg::DetermineNewProfileName(QWidget* parent, xiiString& result)
{
  while (true)
  {
    bool ok = false;
    result  = QInputDialog::getText(parent, "Profile Name", "New Name:", QLineEdit::Normal, "", &ok).toUtf8().data();

    if (!ok)
      return false;

    if (CheckProfileNameUniqueness(result))
      return true;
  }
}

void xiiQtAssetProfilesDlg::on_AddButton_clicked()
{
  xiiString sProfileName;
  if (!DetermineNewProfileName(this, sProfileName))
    return;

  xiiPlatformProfile profile;
  profile.SetConfigName(sProfileName);
  profile.AddMissingConfigs();

  auto& binding      = m_ProfileBindings[NativeToObject(&profile)];
  binding.m_pProfile = nullptr;
  binding.m_State    = Binding::State::Added;

  // select the new profile
  m_pDocument->GetSelectionManager()->SetSelection(m_pDocument->GetObjectManager()->GetRootObject()->GetChildren().PeekBack());
}

void xiiQtAssetProfilesDlg::on_DeleteButton_clicked()
{
  const auto& sel = m_pDocument->GetSelectionManager()->GetSelection();
  if (sel.IsEmpty())
    return;

  // do not allow to delete the first object
  if (sel[0] == m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0])
    return;

  if (xiiQtUiServices::GetSingleton()->MessageBoxQuestion(xiiFmt("Delete the selected profile?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
    return;

  m_ProfileBindings[sel[0]->GetGuid()].m_State = Binding::State::Deleted;

  m_pDocument->GetCommandHistory()->StartTransaction("Delete Profile");

  xiiRemoveObjectCommand cmd;
  cmd.m_Object = sel[0]->GetGuid();

  m_pDocument->GetCommandHistory()->AddCommand(cmd).AssertSuccess();

  m_pDocument->GetCommandHistory()->FinishTransaction();
}

void xiiQtAssetProfilesDlg::on_RenameButton_clicked()
{
  const auto& sel = m_pDocument->GetSelectionManager()->GetSelection();
  if (sel.IsEmpty())
    return;

  // do not allow to rename the first object
  if (sel[0] == m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0])
    return;

  xiiString sProfileName;
  if (!DetermineNewProfileName(this, sProfileName))
    return;

  m_pDocument->GetCommandHistory()->StartTransaction("Rename Profile");

  xiiSetObjectPropertyCommand cmd;
  cmd.m_Object    = sel[0]->GetGuid();
  cmd.m_sProperty = "Name";
  cmd.m_NewValue  = sProfileName;

  m_pDocument->GetCommandHistory()->AddCommand(cmd).AssertSuccess();

  m_pDocument->GetCommandHistory()->FinishTransaction();
}

void xiiQtAssetProfilesDlg::on_SwitchToButton_clicked()
{
  const auto& sel = Tree->selectionModel()->selectedRows();
  if (sel.isEmpty())
    return;

  OnItemDoubleClicked(sel[0]);
}

void xiiQtAssetProfilesDlg::AllAssetProfilesToObject()
{
  m_uiActiveConfig = xiiAssetCurator::GetSingleton()->GetActiveAssetProfileIndex();

  m_ProfileBindings.Clear();

  for (xiiUInt32 i = 0; i < xiiAssetCurator::GetSingleton()->GetNumAssetProfiles(); ++i)
  {
    auto* pProfile = xiiAssetCurator::GetSingleton()->GetAssetProfile(i);

    m_ProfileBindings[NativeToObject(pProfile)].m_pProfile = pProfile;
  }
}

void xiiQtAssetProfilesDlg::PropertyChangedEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  const xiiUuid guid = e.m_pObject->GetGuid();
  XII_ASSERT_DEV(m_ProfileBindings.Contains(guid), "Object GUID is not in the known list!");

  ObjectToNative(guid, m_ProfileBindings[guid].m_pProfile);
}

void xiiQtAssetProfilesDlg::ApplyAllChanges()
{
  for (auto it = m_ProfileBindings.GetIterator(); it.IsValid(); ++it)
  {
    const auto& binding = it.Value();

    xiiPlatformProfile* pProfile = binding.m_pProfile;

    if (binding.m_State == Binding::State::Deleted)
    {
      xiiAssetCurator::GetSingleton()->DeleteAssetProfile(pProfile).IgnoreResult();
      continue;
    }

    if (binding.m_State == Binding::State::Added)
    {
      // create a new profile object and synchronize the state directly into that
      pProfile = xiiAssetCurator::GetSingleton()->CreateAssetProfile();
    }

    ObjectToNative(it.Key(), pProfile);
  }
}
