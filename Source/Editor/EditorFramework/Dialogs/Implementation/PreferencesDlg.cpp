#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/PreferencesDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class xiiPreferencesObjectManager : public xiiDocumentObjectManager
{
public:
  virtual void GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& ref_types) const override
  {
    for (auto pRtti : m_KnownTypes)
    {
      ref_types.PushBack(pRtti);
    }
  }

  xiiHybridArray<const xiiRTTI*, 16> m_KnownTypes;
};


class xiiPreferencesDocument : public xiiDocument
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPreferencesDocument, xiiDocument);

public:
  xiiPreferencesDocument(xiiStringView sDocumentPath) :
    xiiDocument(sDocumentPath, XII_DEFAULT_NEW(xiiPreferencesObjectManager))
  {
  }

public:
  virtual xiiDocumentInfo* CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }
};

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPreferencesDocument, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiQtPreferencesDlg::xiiQtPreferencesDlg(QWidget* pParent) :
  QDialog(pParent)
{
  setupUi(this);

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  m_pDocument = XII_DEFAULT_NEW(xiiPreferencesDocument, "<none>");

  // if this is set, all properties are applied immediately
  // m_pDocument->GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtPreferencesDlg::PropertyChangedEventHandler, this));
  std::unique_ptr<xiiQtDocumentTreeModel> pModel(new xiiQtDocumentTreeModel(m_pDocument->GetObjectManager()));
  pModel->AddAdapter(new xiiQtDummyAdapter(m_pDocument->GetObjectManager(), xiiGetStaticRTTI<xiiDocumentRoot>(), "Children"));
  pModel->AddAdapter(new xiiQtNamedAdapter(m_pDocument->GetObjectManager(), xiiPreferences::GetStaticRTTI(), "", "Name"));

  Tree->Initialize(m_pDocument, std::move(pModel));
  Tree->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  Tree->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

  RegisterAllPreferenceTypes();
  AllPreferencesToObject();

  Properties->SetDocument(m_pDocument);

  m_pDocument->GetSelectionManager()->SetSelection(m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
}

xiiQtPreferencesDlg::~xiiQtPreferencesDlg()
{
  delete Tree;
  Tree = nullptr;

  delete Properties;
  Properties = nullptr;

  XII_DEFAULT_DELETE(m_pDocument);
}

xiiUuid xiiQtPreferencesDlg::NativeToObject(xiiPreferences* pPreferences)
{
  const xiiRTTI* pType = pPreferences->GetDynamicRTTI();
  // Write properties to graph.
  xiiAbstractObjectGraph  graph;
  xiiRttiConverterContext context;
  xiiRttiConverterWriter  conv(&graph, &context, true, true);

  const xiiUuid guid = xiiUuid::MakeUuid();
  context.RegisterObject(guid, pType, pPreferences);
  xiiAbstractObjectNode* pNode = conv.AddObjectToGraph(pType, pPreferences, "root");

  // Read from graph and write into matching document object.
  auto               pRoot   = m_pDocument->GetObjectManager()->GetRootObject();
  xiiDocumentObject* pObject = m_pDocument->GetObjectManager()->CreateObject(pType);
  m_pDocument->GetObjectManager()->AddObject(pObject, pRoot, "Children", -1);

  xiiDocumentObjectConverterReader objectConverter(&graph, m_pDocument->GetObjectManager(), xiiDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  objectConverter.ApplyPropertiesToObject(pNode, pObject);

  return pObject->GetGuid();
}

void xiiQtPreferencesDlg::ObjectToNative(xiiUuid objectGuid, const xiiDocument* pPrefDocument)
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

  xiiPreferences* pPreferences = xiiPreferences::QueryPreferences(pType, pPrefDocument);
  conv.ApplyPropertiesToObject(pNode, pType, pPreferences);

  pPreferences->TriggerPreferencesChangedEvent();
}

void xiiQtPreferencesDlg::on_ButtonOk_clicked()
{
  ApplyAllChanges();
  accept();
}

void xiiQtPreferencesDlg::RegisterAllPreferenceTypes()
{
  xiiPreferencesObjectManager* pManager = static_cast<xiiPreferencesObjectManager*>(m_pDocument->GetObjectManager());

  xiiHybridArray<xiiPreferences*, 16> AllPrefs;
  xiiPreferences::GatherAllPreferences(AllPrefs);

  for (auto pref : AllPrefs)
  {
    pManager->m_KnownTypes.PushBack(pref->GetDynamicRTTI());
  }
}

void xiiQtPreferencesDlg::AllPreferencesToObject()
{
  xiiHybridArray<xiiPreferences*, 16> AllPrefs;
  xiiPreferences::GatherAllPreferences(AllPrefs);

  xiiHybridArray<const xiiAbstractProperty*, 32> properties;

  xiiMap<xiiString, xiiPreferences*> appPref;
  xiiMap<xiiString, xiiPreferences*> projPref;
  xiiMap<xiiString, xiiPreferences*> docPref;

  for (auto pref : AllPrefs)
  {
    bool noVisibleProperties = true;

    // ignore all objects that have no visible properties
    pref->GetDynamicRTTI()->GetAllProperties(properties);
    for (const xiiAbstractProperty* prop : properties)
    {
      if (prop->GetAttributeByType<xiiHiddenAttribute>() != nullptr)
        continue;

      noVisibleProperties = false;
      break;
    }

    if (noVisibleProperties)
      continue;

    switch (pref->GetDomain())
    {
      case xiiPreferences::Domain::Application:
        appPref[pref->GetName()] = pref;
        break;
      case xiiPreferences::Domain::Project:
        projPref[pref->GetName()] = pref;
        break;
      case xiiPreferences::Domain::Document:
        docPref[pref->GetName()] = pref;
        break;
    }
  }

  // create the objects in a certain order

  for (auto it = appPref.GetIterator(); it.IsValid(); ++it)
  {
    m_DocumentBinding[NativeToObject(it.Value())] = it.Value()->GetDocumentAssociation();
  }

  for (auto it = projPref.GetIterator(); it.IsValid(); ++it)
  {
    m_DocumentBinding[NativeToObject(it.Value())] = it.Value()->GetDocumentAssociation();
  }

  for (auto it = docPref.GetIterator(); it.IsValid(); ++it)
  {
    m_DocumentBinding[NativeToObject(it.Value())] = it.Value()->GetDocumentAssociation();
  }
}

void xiiQtPreferencesDlg::PropertyChangedEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  const xiiUuid guid = e.m_pObject->GetGuid();
  XII_ASSERT_DEV(m_DocumentBinding.Contains(guid), "Object GUID is not in the known list!");

  ObjectToNative(guid, m_DocumentBinding[guid]);
}

void xiiQtPreferencesDlg::ApplyAllChanges()
{
  for (auto it = m_DocumentBinding.GetIterator(); it.IsValid(); ++it)
  {
    ObjectToNative(it.Key(), it.Value());
  }
}
