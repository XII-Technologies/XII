/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Dialogs/AssetProfilesDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Platform/PlatformDescription.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
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

        if (pObject->GetGuid() == m_pDialog->m_OriginalActiveProfile)
        {
          name += " (active)";
        }
        else if (pObject->GetGuid() == m_pDialog->m_ActiveProfile)
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
  const xiiStatus status = ApplyAllChanges();
  if (status.Failed())
  {
    xiiQtUiServices::MessageBoxStatus(status, "Could not save asset profiles.");
    return;
  }
  accept();
}

void xiiQtAssetProfilesDlg::on_ButtonCancel_clicked()
{
  reject();
}

void xiiQtAssetProfilesDlg::OnItemDoubleClicked(QModelIndex idx)
{
  if (!idx.isValid())
    return;
  const auto  source = Tree->GetProxyFilterModel()->mapToSource(idx);
  const auto* model  = static_cast<const xiiQtDocumentTreeModel*>(source.model());
  const auto* object = model->GetObject(source);
  if (object == nullptr || m_ActiveProfile == object->GetGuid())
    return;
  m_ActiveProfile = object->GetGuid();

  QVector<int> roles;
  roles.push_back(Qt::DisplayRole);
  Tree->model()->dataChanged(Tree->model()->index(0, 0), Tree->model()->index(Tree->model()->rowCount() - 1, 0), roles);
}

bool xiiQtAssetProfilesDlg::CheckProfileNameUniqueness(const char* szName, xiiUuid exclude)
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
    if (pObject->GetGuid() != exclude && pObject->GetTypeAccessor().GetValue("Name").ConvertTo<xiiString>().IsEqual_NoCase(szName))
    {
      xiiQtUiServices::GetSingleton()->MessageBoxInformation("A profile with this name already exists.");
      return false;
    }
  }

  return true;
}

bool xiiQtAssetProfilesDlg::DetermineNewProfileName(QWidget* parent, xiiString& result, xiiUuid exclude)
{
  while (true)
  {
    bool ok = false;
    result  = QInputDialog::getText(parent, "Profile Name", "New Name:", QLineEdit::Normal, "", &ok).toUtf8().data();

    if (!ok)
      return false;

    if (CheckProfileNameUniqueness(result, exclude))
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

  NativeToObject(&profile);

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

  if (m_ActiveProfile == sel[0]->GetGuid())
    m_ActiveProfile = m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]->GetGuid();

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
  if (!DetermineNewProfileName(this, sProfileName, sel[0]->GetGuid()))
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
  for (xiiUInt32 i = 0; i < xiiAssetCurator::GetSingleton()->GetNumAssetProfiles(); ++i)
  {
    auto* pProfile = xiiAssetCurator::GetSingleton()->GetAssetProfile(i);

    const auto id = NativeToObject(pProfile);
    if (i == xiiAssetCurator::GetSingleton()->GetActiveAssetProfileIndex())
      m_ActiveProfile = m_OriginalActiveProfile = id;
  }
}

xiiStatus xiiQtAssetProfilesDlg::ApplyAllChanges()
{
  xiiHybridArray<xiiPlatformProfile*, 8> profiles;
  XII_SCOPE_EXIT(for (auto* profile : profiles) XII_DEFAULT_DELETE(profile));
  xiiUInt32   activeIndex = 0;
  const auto& objects     = m_pDocument->GetObjectManager()->GetRootObject()->GetChildren();
  for (xiiUInt32 i = 0; i < objects.GetCount(); ++i)
  {
    xiiPlatformProfile* profile = XII_DEFAULT_NEW(xiiPlatformProfile);
    profiles.PushBack(profile);
    ObjectToNative(objects[i]->GetGuid(), profile);
    if (objects[i]->GetGuid() == m_ActiveProfile)
      activeIndex = i;
  }
  return xiiAssetCurator::GetSingleton()->ApplyAssetProfiles({profiles.GetData(), profiles.GetCount()}, activeIndex);
}
