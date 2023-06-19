#include <EditorTest/EditorTestPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorTest/SceneDocument/SceneDocumentTest.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <QMimeData>
#include <RendererCore/Lights/SphereReflectionProbeComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static xiiEditorSceneDocumentTest s_EditorSceneDocumentTest;

const char* xiiEditorSceneDocumentTest::GetTestName() const
{
  return "Scene Document Tests";
}

void xiiEditorSceneDocumentTest::SetupSubTests()
{
  AddSubTest("Layer Operations", SubTests::ST_LayerOperations);
  AddSubTest("Prefab Operations", SubTests::ST_PrefabOperations);
  AddSubTest("Component Operations", SubTests::ST_ComponentOperations);
}

xiiResult xiiEditorSceneDocumentTest::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return XII_FAILURE;

  if (SUPER::CreateAndLoadProject("SceneTestProject").Failed())
    return XII_FAILURE;

  if (xiiStatus res = xiiAssetCurator::GetSingleton()->TransformAllAssets(xiiTransformFlags::None); res.Failed())
  {
    xiiLog::Error("Asset transform failed: {}", res.m_sMessage);
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiEditorSceneDocumentTest::DeInitializeTest()
{
  m_pDoc   = nullptr;
  m_pLayer = nullptr;
  m_SceneGuid.SetInvalid();
  m_LayerGuid.SetInvalid();

  if (SUPER::DeInitializeTest().Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiTestAppRun xiiEditorSceneDocumentTest::RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_LayerOperations:
      LayerOperations();
      break;
    case SubTests::ST_PrefabOperations:
      PrefabOperations();
      break;
    case SubTests::ST_ComponentOperations:
      ComponentOperations();
      break;
  }
  return xiiTestAppRun::Quit;
}

xiiResult xiiEditorSceneDocumentTest::CreateSimpleScene(const char* szSceneName)
{
  xiiStringBuilder sName;
  sName = m_sProjectPath;
  sName.AppendPath(szSceneName);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create Document")
  {
    m_pDoc = static_cast<xiiScene2Document*>(m_pApplication->m_pEditorApp->CreateDocument(sName, xiiDocumentFlags::RequestWindow));
    if (!XII_TEST_BOOL(m_pDoc != nullptr))
      return XII_FAILURE;

    m_SceneGuid = m_pDoc->GetGuid();
    ProcessEvents();
    XII_TEST_STATUS(m_pDoc->CreateLayer("Layer1", m_LayerGuid));
    m_pLayer = xiiDynamicCast<xiiLayerDocument*>(m_pDoc->GetLayerDocument(m_LayerGuid));
    if (!XII_TEST_BOOL(m_pLayer != nullptr))
      return XII_FAILURE;
  }
  return XII_SUCCESS;
}

void xiiEditorSceneDocumentTest::CloseSimpleScene()
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Close Document")
  {
    bool           bSaved = false;
    xiiTaskGroupID id     = m_pDoc->SaveDocumentAsync(
      [&bSaved](xiiDocument* doc, xiiStatus res) {
        bSaved = true;
      },
      true);

    m_pDoc->GetDocumentManager()->CloseDocument(m_pDoc);
    XII_TEST_BOOL(xiiTaskSystem::IsTaskGroupFinished(id));
    XII_TEST_BOOL(bSaved);
    m_pDoc   = nullptr;
    m_pLayer = nullptr;
    m_SceneGuid.SetInvalid();
    m_LayerGuid.SetInvalid();
  }
}

void xiiEditorSceneDocumentTest::LayerOperations()
{
  xiiStringBuilder sName;
  sName = m_sProjectPath;
  sName.AppendPath("LayerOperations.xiiScene");

  xiiScene2Document*                     pDoc          = nullptr;
  xiiEventSubscriptionID                 layerEventsID = 0;
  xiiHybridArray<xiiScene2LayerEvent, 2> expectedEvents;
  xiiUuid                                sceneGuid;
  xiiUuid                                layer1Guid;
  xiiLayerDocument*                      pLayer1 = nullptr;

  auto TestLayerEvents = [&expectedEvents](const xiiScene2LayerEvent& e) {
    if (XII_TEST_BOOL(!expectedEvents.IsEmpty()))
    {
      // If we pass in an invalid guid it's considered fine as we might not know the ID, e.g. when creating a layer.
      XII_TEST_BOOL(!expectedEvents[0].m_layerGuid.IsValid() || expectedEvents[0].m_layerGuid == e.m_layerGuid);
      XII_TEST_BOOL(expectedEvents[0].m_Type == e.m_Type);
      expectedEvents.RemoveAtAndCopy(0);
    }
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create Document")
  {
    pDoc = static_cast<xiiScene2Document*>(m_pApplication->m_pEditorApp->CreateDocument(sName, xiiDocumentFlags::RequestWindow));
    if (!XII_TEST_BOOL(pDoc != nullptr))
      return;

    sceneGuid     = pDoc->GetGuid();
    layerEventsID = pDoc->m_LayerEvents.AddEventHandler(TestLayerEvents);
    ProcessEvents();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create Layer")
  {
    XII_TEST_BOOL(pDoc->GetActiveLayer() == sceneGuid);
    XII_TEST_BOOL(pDoc->IsLayerVisible(sceneGuid));
    XII_TEST_BOOL(pDoc->IsLayerLoaded(sceneGuid));

    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerAdded, xiiUuid()});
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerLoaded, xiiUuid()});
    XII_TEST_STATUS(pDoc->CreateLayer("Layer1", layer1Guid));

    expectedEvents.PushBack({xiiScene2LayerEvent::Type::ActiveLayerChanged, layer1Guid});
    XII_TEST_STATUS(pDoc->SetActiveLayer(layer1Guid));
    XII_TEST_BOOL(pDoc->GetActiveLayer() == layer1Guid);
    XII_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    XII_TEST_BOOL(pDoc->IsLayerLoaded(layer1Guid));
    pLayer1 = xiiDynamicCast<xiiLayerDocument*>(pDoc->GetLayerDocument(layer1Guid));
    XII_TEST_BOOL(pLayer1 != nullptr);

    xiiHybridArray<xiiSceneDocument*, 2> layers;
    pDoc->GetLoadedLayers(layers);
    XII_TEST_INT(layers.GetCount(), 2);
    XII_TEST_BOOL(layers.Contains(pLayer1));
    XII_TEST_BOOL(layers.Contains(pDoc));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Undo/Redo Layer Creation")
  {
    // Undo / redo
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::ActiveLayerChanged, sceneGuid});
    XII_TEST_STATUS(pDoc->SetActiveLayer(sceneGuid));
    // Initial scene setup exists in the scene undo stack
    const xiiUInt32 uiInitialUndoStackSize = pDoc->GetCommandHistory()->GetUndoStackSize();
    XII_TEST_BOOL(uiInitialUndoStackSize >= 1);
    XII_TEST_INT(pDoc->GetSceneCommandHistory()->GetUndoStackSize(), uiInitialUndoStackSize);
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerRemoved, layer1Guid});
    XII_TEST_STATUS(pDoc->GetCommandHistory()->Undo(1));
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerAdded, layer1Guid});
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerLoaded, layer1Guid});
    XII_TEST_STATUS(pDoc->GetCommandHistory()->Redo(1));

    pLayer1 = xiiDynamicCast<xiiLayerDocument*>(pDoc->GetLayerDocument(layer1Guid));
    XII_TEST_BOOL(pLayer1 != nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Save and Close Document")
  {
    bool           bSaved = false;
    xiiTaskGroupID id     = pDoc->SaveDocumentAsync(
      [&bSaved](xiiDocument* doc, xiiStatus res) {
        bSaved = true;
      },
      true);

    pDoc->m_LayerEvents.RemoveEventHandler(layerEventsID);
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    XII_TEST_BOOL(xiiTaskSystem::IsTaskGroupFinished(id));
    XII_TEST_BOOL(bSaved);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reload Document")
  {
    pDoc = static_cast<xiiScene2Document*>(m_pApplication->m_pEditorApp->OpenDocument(sName, xiiDocumentFlags::RequestWindow));
    if (!XII_TEST_BOOL(pDoc != nullptr))
      return;
    layerEventsID = pDoc->m_LayerEvents.AddEventHandler(TestLayerEvents);
    ProcessEvents();

    XII_TEST_BOOL(pDoc->GetActiveLayer() == sceneGuid);
    XII_TEST_BOOL(pDoc->IsLayerVisible(sceneGuid));
    XII_TEST_BOOL(pDoc->IsLayerLoaded(sceneGuid));

    pLayer1 = xiiDynamicCast<xiiLayerDocument*>(pDoc->GetLayerDocument(layer1Guid));
    xiiHybridArray<xiiSceneDocument*, 2> layers;
    pDoc->GetLoadedLayers(layers);
    XII_TEST_INT(layers.GetCount(), 2);
    XII_TEST_BOOL(layers.Contains(pLayer1));
    XII_TEST_BOOL(layers.Contains(pDoc));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Toggle Layer Visibility")
  {
    XII_TEST_BOOL(pDoc->GetActiveLayer() == sceneGuid);
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::ActiveLayerChanged, layer1Guid});
    XII_TEST_STATUS(pDoc->SetActiveLayer(layer1Guid));
    XII_TEST_BOOL(pDoc->GetActiveLayer() == layer1Guid);
    XII_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    XII_TEST_BOOL(pDoc->IsLayerLoaded(layer1Guid));

    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerInvisible, layer1Guid});
    XII_TEST_STATUS(pDoc->SetLayerVisible(layer1Guid, false));
    XII_TEST_BOOL(!pDoc->IsLayerVisible(layer1Guid));
    ProcessEvents();
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerVisible, layer1Guid});
    XII_TEST_STATUS(pDoc->SetLayerVisible(layer1Guid, true));
    XII_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    ProcessEvents();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Toggle Layer Loaded")
  {
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerInvisible, layer1Guid});
    XII_TEST_STATUS(pDoc->SetLayerVisible(layer1Guid, false));

    expectedEvents.PushBack({xiiScene2LayerEvent::Type::ActiveLayerChanged, sceneGuid});
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    XII_TEST_STATUS(pDoc->SetLayerLoaded(layer1Guid, false));

    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerLoaded, layer1Guid});
    XII_TEST_STATUS(pDoc->SetLayerLoaded(layer1Guid, true));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Delete Layer")
  {
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::ActiveLayerChanged, layer1Guid});
    XII_TEST_STATUS(pDoc->SetActiveLayer(layer1Guid));

    expectedEvents.PushBack({xiiScene2LayerEvent::Type::ActiveLayerChanged, sceneGuid});
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerRemoved, layer1Guid});
    XII_TEST_STATUS(pDoc->DeleteLayer(layer1Guid));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Undo/Redo Layer Deletion")
  {
    XII_TEST_INT(pDoc->GetCommandHistory()->GetUndoStackSize(), 1);
    XII_TEST_INT(pDoc->GetSceneCommandHistory()->GetUndoStackSize(), 1);
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerAdded, layer1Guid});
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerLoaded, layer1Guid});
    XII_TEST_STATUS(pDoc->GetCommandHistory()->Undo(1));
    XII_TEST_BOOL(pDoc->IsLayerVisible(layer1Guid));
    XII_TEST_BOOL(pDoc->IsLayerLoaded(layer1Guid));

    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerUnloaded, layer1Guid});
    expectedEvents.PushBack({xiiScene2LayerEvent::Type::LayerRemoved, layer1Guid});
    XII_TEST_STATUS(pDoc->GetCommandHistory()->Redo(1));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Close Document")
  {
    bool           bSaved = false;
    xiiTaskGroupID id     = pDoc->SaveDocumentAsync(
      [&bSaved](xiiDocument* doc, xiiStatus res) {
        bSaved = true;
      },
      true);

    pDoc->m_LayerEvents.RemoveEventHandler(layerEventsID);
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
    XII_TEST_BOOL(xiiTaskSystem::IsTaskGroupFinished(id));
    XII_TEST_BOOL(bSaved);
  }
}


void xiiEditorSceneDocumentTest::PrefabOperations()
{
  if (CreateSimpleScene("PrefabOperations.xiiScene").Failed())
    return;

  const xiiDocumentObject* pPrefab1  = nullptr;
  const xiiDocumentObject* pPrefab2  = nullptr;
  auto                     pAccessor = m_pDoc->GetObjectAccessor();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Drag&Drop Prefabs")
  {
    const char* szSpherePrefab = "{ a3ce5d3d-be5e-4bda-8820-b1ce3b3d33fd }";
    pPrefab1                   = DropAsset(m_pDoc, szSpherePrefab);
    XII_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab1->GetGuid()));
    XII_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab1->GetGuid()));
    pPrefab2 = DropAsset(m_pDoc, szSpherePrefab, true);
    XII_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab2->GetGuid()));
    XII_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab2->GetGuid()));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create Nodes and check default state")
  {
    const char*              szSphereMesh = "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }";
    const xiiDocumentObject* pSphere1     = DropAsset(m_pDoc, szSphereMesh);
    const xiiDocumentObject* pSphere2     = DropAsset(m_pDoc, szSphereMesh);

    pAccessor->StartTransaction("Modify objects");
    XII_TEST_STATUS(pAccessor->SetValue(pSphere1, "Name", "Sphere1"));
    XII_TEST_STATUS(pAccessor->SetValue(pSphere1, "LocalPosition", xiiVec3(1.0f, 0.0f, 0.0f)));
    XII_TEST_STATUS(pAccessor->SetValue(pSphere1, "LocalRotation", xiiQuat(1.0f, 0.0f, 0.0f, 0.0f)));
    XII_TEST_STATUS(pAccessor->SetValue(pSphere1, "LocalScaling", xiiVec3(1.0f, 2.0f, 3.0f)));
    XII_TEST_STATUS(pAccessor->InsertValue(pSphere1, "Tags", "SkyLight", -1));
    const xiiDocumentObject* pMeshComponent = pAccessor->GetObject(pAccessor->Get<xiiVariantArray>(pSphere1, "Components")[0].Get<xiiUuid>());
    XII_TEST_STATUS(pAccessor->InsertValue(pMeshComponent, "Materials", "{ d615cd66-0904-00ca-81f9-768ff4fc24ee }", 0));

    xiiUuid pSphereRef;
    XII_TEST_STATUS(pAccessor->AddObject(pSphere1, "Components", -1, xiiGetStaticRTTI<xiiSphereReflectionProbeComponent>(), pSphereRef));

    pAccessor->FinishTransaction();

    {
      // Check that modifications above changed properties from their default state.
      xiiHybridArray<xiiPropertySelection, 1> selection;
      selection.PushBack({pSphere1, xiiVariant()});
      xiiDefaultObjectState defaultState(pAccessor, selection);
      XII_TEST_STRING(defaultState.GetStateProviderName(), "Attribute");

      XII_TEST_BOOL(!defaultState.IsDefaultValue("Name"));
      XII_TEST_BOOL(!defaultState.IsDefaultValue("LocalPosition"));
      XII_TEST_BOOL(!defaultState.IsDefaultValue("LocalRotation"));
      XII_TEST_BOOL(!defaultState.IsDefaultValue("LocalScaling"));
      XII_TEST_BOOL(!defaultState.IsDefaultValue("Tags"));
      XII_TEST_BOOL(!defaultState.IsDefaultValue("Components"));

      // Does default state match that of pSphere2 which is unmodified?
      auto MatchesDefaultValue = [&](xiiDefaultObjectState& defaultState, const char* szProperty) {
        xiiVariant defaultValue = defaultState.GetDefaultValue(szProperty);
        xiiVariant sphere2value;
        XII_TEST_STATUS(pAccessor->GetValue(pSphere2, szProperty, sphere2value));
        XII_TEST_BOOL(defaultValue == sphere2value);
      };

      MatchesDefaultValue(defaultState, "Name");
      MatchesDefaultValue(defaultState, "LocalPosition");
      MatchesDefaultValue(defaultState, "LocalRotation");
      MatchesDefaultValue(defaultState, "LocalScaling");
      MatchesDefaultValue(defaultState, "Tags");
    }

    {
      // pSphere2 should be unmodified except for the component array.
      xiiHybridArray<xiiPropertySelection, 1> selection;
      selection.PushBack({pSphere2, xiiVariant()});
      xiiDefaultObjectState defaultState(pAccessor, selection);
      XII_TEST_BOOL(defaultState.IsDefaultValue("Name"));
      XII_TEST_BOOL(defaultState.IsDefaultValue("LocalPosition"));
      XII_TEST_BOOL(defaultState.IsDefaultValue("LocalRotation"));
      XII_TEST_BOOL(defaultState.IsDefaultValue("LocalScaling"));
      XII_TEST_BOOL(defaultState.IsDefaultValue("Tags"));
      XII_TEST_BOOL(!defaultState.IsDefaultValue("Components"));
    }

    {
      // Multi-selection should not be default if one in the selection is not.
      xiiHybridArray<xiiPropertySelection, 1> selection;
      selection.PushBack({pSphere1, xiiVariant()});
      selection.PushBack({pSphere2, xiiVariant()});
      xiiDefaultObjectState defaultState(pAccessor, selection);
      XII_TEST_BOOL(!defaultState.IsDefaultValue("Name"));
      XII_TEST_BOOL(!defaultState.IsDefaultValue("LocalPosition"));
      XII_TEST_BOOL(!defaultState.IsDefaultValue("LocalRotation"));
      XII_TEST_BOOL(!defaultState.IsDefaultValue("LocalScaling"));
      XII_TEST_BOOL(!defaultState.IsDefaultValue("Tags"));
      XII_TEST_BOOL(!defaultState.IsDefaultValue("Components"));
    }

    {
      // Default state object array
      xiiHybridArray<xiiPropertySelection, 1> selection;
      selection.PushBack({pSphere1, xiiVariant()});
      xiiDefaultContainerState defaultState(pAccessor, selection, "Components");
      XII_TEST_STRING(defaultState.GetStateProviderName(), "Attribute");
      XII_TEST_BOOL(defaultState.GetDefaultContainer() == xiiVariantArray());
      XII_TEST_BOOL(defaultState.GetDefaultElement(0) == xiiUuid());
      XII_TEST_BOOL(!defaultState.IsDefaultContainer());
      // We currently do not supporting reverting an index of a non-value type container. Thus, they are always the default state.
      XII_TEST_BOOL(defaultState.IsDefaultElement(0));
      XII_TEST_BOOL(defaultState.IsDefaultElement(1));
    }

    {
      // Default state value array
      xiiHybridArray<xiiPropertySelection, 1> selection;
      selection.PushBack({pMeshComponent, xiiVariant()});
      xiiDefaultContainerState defaultState(pAccessor, selection, "Materials");
      XII_TEST_STRING(defaultState.GetStateProviderName(), "Attribute");
      XII_TEST_BOOL(defaultState.GetDefaultContainer() == xiiVariantArray());
      XII_TEST_BOOL(defaultState.GetDefaultElement(0) == "");
      XII_TEST_BOOL(!defaultState.IsDefaultContainer());
      XII_TEST_BOOL(!defaultState.IsDefaultElement(0));

      xiiDefaultObjectState defaultObjectState(pAccessor, selection);
      XII_TEST_STRING(defaultObjectState.GetStateProviderName(), "Attribute");
      XII_TEST_BOOL(defaultObjectState.GetDefaultValue("Materials") == xiiVariantArray());
      XII_TEST_BOOL(!defaultObjectState.IsDefaultValue("Materials"));
    }

    xiiDeque<const xiiDocumentObject*> selection;
    selection.PushBack(pSphere1);
    selection.PushBack(pSphere2);
    m_pDoc->GetSelectionManager()->SetSelection(selection);
  }

  xiiUuid                  prefabGuid;
  const xiiDocumentObject* pPrefab3 = nullptr;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create Prefab from Selection")
  {
    //ProcessEvents(999999999);

    xiiStringBuilder sPrefabName;
    sPrefabName = m_sProjectPath;
    sPrefabName.AppendPath("Spheres.xiiPrefab");
    XII_TEST_BOOL(m_pDoc->CreatePrefabDocumentFromSelection(sPrefabName, xiiGetStaticRTTI<xiiGameObject>(), {}, {}, [](xiiAbstractObjectGraph& graph, xiiDynamicArray<xiiAbstractObjectNode*>&) { /* Do nothing */ }).Succeeded());
    m_pDoc->ScheduleSendObjectSelection();
    pPrefab3 = m_pDoc->GetSelectionManager()->GetCurrentObject();
    XII_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid()));
    XII_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab3->GetGuid(), &prefabGuid));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move Prefabs to Layer")
  {
    XII_TEST_BOOL(pPrefab1->GetDocumentObjectManager() == m_pDoc->GetSceneObjectManager());
    XII_TEST_BOOL(pPrefab2->GetDocumentObjectManager() == m_pDoc->GetSceneObjectManager());
    XII_TEST_BOOL(pPrefab3->GetDocumentObjectManager() == m_pDoc->GetSceneObjectManager());

    // Copy & paste should retain the order in the tree view, not the selection array so we push the elements in a random order here.
    xiiDeque<const xiiDocumentObject*> assets;
    assets.PushBack(pPrefab3);
    assets.PushBack(pPrefab1);
    assets.PushBack(pPrefab2);
    xiiDeque<const xiiDocumentObject*> newObjects;

    MoveObjectsToLayer(m_pDoc, assets, m_LayerGuid, newObjects);

    XII_TEST_BOOL(m_pDoc->GetActiveLayer() == m_SceneGuid);
    XII_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab1->GetGuid()) == nullptr);
    XII_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab2->GetGuid()) == nullptr);
    XII_TEST_BOOL(m_pDoc->GetObjectManager()->GetObject(pPrefab3->GetGuid()) == nullptr);

    XII_TEST_INT(newObjects.GetCount(), assets.GetCount());
    pPrefab1 = newObjects[0];
    pPrefab2 = newObjects[1];
    pPrefab3 = newObjects[2];

    XII_TEST_STATUS(m_pDoc->SetActiveLayer(m_LayerGuid));

    XII_TEST_BOOL(pPrefab1->GetDocumentObjectManager() == m_pLayer->GetObjectManager());
    XII_TEST_BOOL(pPrefab2->GetDocumentObjectManager() == m_pLayer->GetObjectManager());
    XII_TEST_BOOL(pPrefab3->GetDocumentObjectManager() == m_pLayer->GetObjectManager());

    XII_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab1->GetGuid()));
    XII_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab1->GetGuid()));
    XII_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab2->GetGuid()));
    XII_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab2->GetGuid()));
    XII_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid()));
    xiiUuid prefabGuidOut;
    XII_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab3->GetGuid(), &prefabGuidOut));
    XII_TEST_BOOL(prefabGuid == prefabGuidOut);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Change Prefab Type")
  {
    xiiVariant                         oldIndex = pPrefab1->GetPropertyIndex();
    xiiDeque<const xiiDocumentObject*> selection;
    {
      selection.PushBack(pPrefab1);
      m_pDoc->ConvertToEditorPrefab(selection);
      pPrefab1 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      XII_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab1->GetGuid()));
      XII_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab1->GetGuid()));
      XII_TEST_BOOL(oldIndex == pPrefab1->GetPropertyIndex());
    }

    {
      oldIndex = pPrefab2->GetPropertyIndex();
      selection.Clear();
      selection.PushBack(pPrefab2);
      m_pDoc->ConvertToEnginePrefab(selection);
      pPrefab2 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      XII_TEST_BOOL(!m_pDoc->IsObjectEditorPrefab(pPrefab2->GetGuid()));
      XII_TEST_BOOL(m_pDoc->IsObjectEnginePrefab(pPrefab2->GetGuid()));
      XII_TEST_BOOL(oldIndex == pPrefab2->GetPropertyIndex());
    }

    {
      oldIndex = pPrefab3->GetPropertyIndex();
      selection.Clear();
      selection.PushBack(pPrefab3);
      m_pDoc->ConvertToEditorPrefab(selection);
      pPrefab3 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      XII_TEST_BOOL(!m_pDoc->IsObjectEnginePrefab(pPrefab3->GetGuid()));
      xiiUuid prefabGuidOut;
      XII_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid(), &prefabGuidOut));
      XII_TEST_BOOL(prefabGuid == prefabGuidOut);
      XII_TEST_BOOL(oldIndex == pPrefab3->GetPropertyIndex());
    }
  }

  auto IsObjectDefault = [&](const xiiDocumentObject* pChild) {
    xiiHybridArray<xiiPropertySelection, 1> selection;
    selection.PushBack({pChild, xiiVariant()});
    xiiDefaultObjectState defaultState(pAccessor, selection);
    // The root node of the prefab is not actually part of the prefab in the sense that it is just the container and does not actually exist in the prefab itself.
    const char* szExpectedProvider = pChild == pPrefab3 ? "Attribute" : "Prefab";
    XII_TEST_STRING(defaultState.GetStateProviderName(), szExpectedProvider);

    xiiHybridArray<xiiAbstractProperty*, 32> properties;
    pChild->GetType()->GetAllProperties(properties);
    for (xiiAbstractProperty* pProp : properties)
    {
      if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Hidden | xiiPropertyFlags::ReadOnly))
        continue;

      XII_TEST_BOOL(defaultState.IsDefaultValue(pProp));
    }
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Modify Editor Prefab")
  {
    xiiVariant                 oldIndex  = pPrefab3->GetPropertyIndex();
    const xiiAbstractProperty* pProp     = pPrefab3->GetType()->FindPropertyByName("Children");
    const xiiAbstractProperty* pCompProp = pPrefab3->GetType()->FindPropertyByName("Components");

    //ProcessEvents(999999999);

    CheckHierarchy(pAccessor, pPrefab3, IsObjectDefault);

    //ProcessEvents(999999999);
    {
      m_pDoc->UpdatePrefabs();
      // Update prefabs replaces object instances with new ones with the same IDs so the old ones are in the undo history now.
      pPrefab3 = pAccessor->GetObject(pPrefab3->GetGuid());
    }

    {
      // Remove part of the prefab
      xiiHybridArray<xiiVariant, 16> values;
      XII_TEST_STATUS(pAccessor->GetValues(pPrefab3, pProp, values));
      XII_TEST_INT(values.GetCount(), 2);
      const xiiDocumentObject* pChild0 = pAccessor->GetObject(values[0].Get<xiiUuid>());
      const xiiDocumentObject* pChild1 = pAccessor->GetObject(values[1].Get<xiiUuid>());
      pAccessor->StartTransaction("Delete child0");
      pAccessor->RemoveObject(pChild1);
      pAccessor->FinishTransaction();
      XII_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 1);
    }

    {
      m_pDoc->UpdatePrefabs();
      pPrefab3 = pAccessor->GetObject(pPrefab3->GetGuid());
    }

    {
      // Revert prefab
      xiiDeque<const xiiDocumentObject*> selection;
      selection.PushBack(pPrefab3);
      m_pDoc->RevertPrefabs(selection);

      pPrefab3 = m_pDoc->GetSelectionManager()->GetCurrentObject();
      xiiUuid prefabGuidOut;
      XII_TEST_BOOL(m_pDoc->IsObjectEditorPrefab(pPrefab3->GetGuid(), &prefabGuidOut));
      XII_TEST_BOOL(prefabGuid == prefabGuidOut);
      XII_TEST_BOOL(oldIndex == pPrefab3->GetPropertyIndex());
      XII_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 2);
    }

    {
      // Modify the prefab
      xiiHybridArray<xiiVariant, 16> values;
      XII_TEST_STATUS(pAccessor->GetValues(pPrefab3, pProp, values));
      XII_TEST_INT(values.GetCount(), 2);
      const xiiDocumentObject* pChild0 = pAccessor->GetObject(values[0].Get<xiiUuid>());
      const xiiDocumentObject* pChild1 = pAccessor->GetObject(values[1].Get<xiiUuid>());

      pAccessor->StartTransaction("Modify Prefab");
      xiiUuid compGuid;
      XII_TEST_STATUS(pAccessor->AddObject(pChild0, "Components", -1, xiiRTTI::FindTypeByName("xiiBeamComponent"), compGuid));
      const xiiDocumentObject* pComp = pAccessor->GetObject(compGuid);

      const xiiDocumentObject* pChild1Comp = pAccessor->GetChildObject(pChild1, "Components", 0);
      XII_TEST_STATUS(pAccessor->RemoveObject(pChild1Comp));
      pAccessor->FinishTransaction();

      XII_TEST_INT(pAccessor->GetCount(pPrefab3, pProp), 2);
      XII_TEST_INT(pAccessor->GetCount(pChild0, pCompProp), 3);
      XII_TEST_INT(pAccessor->GetCount(pChild1, pCompProp), 0);

      // Check default states
      {
        xiiHybridArray<xiiPropertySelection, 1> selection;
        selection.PushBack({pChild0, xiiVariant()});
        xiiDefaultContainerState defaultObjectState(pAccessor, selection, "Components");
        XII_TEST_STRING(defaultObjectState.GetStateProviderName(), "Prefab");
        XII_TEST_BOOL(!defaultObjectState.IsDefaultContainer());
      }

      {
        xiiHybridArray<xiiPropertySelection, 1> selection;
        selection.PushBack({pChild1, xiiVariant()});
        xiiDefaultContainerState defaultObjectState(pAccessor, selection, "Components");
        XII_TEST_STRING(defaultObjectState.GetStateProviderName(), "Prefab");
        XII_TEST_BOOL(!defaultObjectState.IsDefaultContainer());
      }

      {
        xiiHybridArray<xiiPropertySelection, 1> selection;
        selection.PushBack({pComp, xiiVariant()});
        xiiDefaultObjectState defaultObjectState(pAccessor, selection);
        XII_TEST_STRING(defaultObjectState.GetStateProviderName(), "Attribute");
      }
    }

    {
      m_pDoc->UpdatePrefabs();
      pPrefab3 = pAccessor->GetObject(pPrefab3->GetGuid());
    }

    {
      // Revert via default state
      const xiiDocumentObject* pChild1 = pAccessor->GetChildObject(pPrefab3, "Children", 0);
      const xiiDocumentObject* pChild2 = pAccessor->GetChildObject(pPrefab3, "Children", 1);
      {
        xiiHybridArray<xiiPropertySelection, 1> selection;
        selection.PushBack({pChild1, xiiVariant()});
        selection.PushBack({pChild2, xiiVariant()});
        xiiDefaultContainerState defaultState(pAccessor, selection, "Components");

        pAccessor->StartTransaction("Revert children");
        defaultState.RevertContainer();
        pAccessor->FinishTransaction();
      }
    }

    {
      // Verify prefab was reverted
      xiiHybridArray<xiiVariant, 16> values;
      XII_TEST_STATUS(pAccessor->GetValues(pPrefab3, pProp, values));
      XII_TEST_INT(values.GetCount(), 2);
      const xiiDocumentObject* pChild0 = pAccessor->GetObject(values[0].Get<xiiUuid>());
      const xiiDocumentObject* pChild1 = pAccessor->GetObject(values[1].Get<xiiUuid>());

      values.Clear();
      XII_TEST_STATUS(pAccessor->GetValues(pChild0, "Components", values));
      XII_TEST_INT(values.GetCount(), 2);

      XII_TEST_STATUS(pAccessor->GetValues(pChild1, "Components", values));
      XII_TEST_INT(values.GetCount(), 1);

      CheckHierarchy(pAccessor, pPrefab3, IsObjectDefault);
    }
  }

  ProcessEvents(10);
  CloseSimpleScene();
}

void xiiEditorSceneDocumentTest::ComponentOperations()
{
  if (CreateSimpleScene("ComponentOperations.xiiScene").Failed())
    return;

  auto pAccessor = m_pDoc->GetObjectAccessor();

  const xiiDocumentObject* pRoot = CreateGameObject(m_pDoc);

  xiiDeque<const xiiDocumentObject*> selection;
  selection.PushBack(pRoot);
  m_pDoc->GetSelectionManager()->SetSelection(selection);

  auto CreateComponent = [&](const xiiRTTI* pType, const xiiDocumentObject* pParent) -> const xiiDocumentObject* {
    xiiUuid compGuid;
    XII_TEST_STATUS(pAccessor->AddObject(pParent, "Components", -1, pType, compGuid));
    return pAccessor->GetObject(compGuid);
  };

  auto IsObjectDefault = [&](const xiiDocumentObject* pChild) {
    xiiHybridArray<xiiPropertySelection, 1> selection;
    selection.PushBack({pChild, xiiVariant()});
    xiiDefaultObjectState defaultState(pAccessor, selection);

    xiiHybridArray<xiiAbstractProperty*, 32> properties;
    pChild->GetType()->GetAllProperties(properties);
    for (xiiAbstractProperty* pProp : properties)
    {
      if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Hidden | xiiPropertyFlags::ReadOnly))
        continue;

      xiiVariant defaultValue = defaultState.GetDefaultValue(pProp);
      XII_TEST_BOOL(xiiDefaultStateProvider::DoesVariantMatchProperty(defaultValue, pProp));
      xiiVariant currentValue;
      XII_TEST_STATUS(pAccessor->GetValue(pChild, pProp, currentValue));
      XII_TEST_BOOL(xiiDefaultStateProvider::DoesVariantMatchProperty(currentValue, pProp));
      XII_TEST_BOOL(defaultValue == currentValue);
      XII_TEST_BOOL(defaultState.IsDefaultValue(pProp));
    }
  };

  xiiDynamicArray<const xiiRTTI*> componentTypes;
  xiiRTTI::GetAllTypesDerivedFrom(xiiGetStaticRTTI<xiiComponent>(), componentTypes, true);

  xiiSet<const xiiRTTI*> blacklist;
  // The scene already has one and the code asserts otherwise. There needs to be a general way of preventing two settings components from existing at the same time.
  blacklist.Insert(xiiRTTI::FindTypeByName("xiiSkyLightComponent"));

  pAccessor->StartTransaction("Modify objects");

  for (auto pType : componentTypes)
  {
    if (pType->GetTypeFlags().IsSet(xiiTypeFlags::Abstract) || blacklist.Contains(pType))
      continue;

    auto pComp = CreateComponent(pType, pRoot);

    CheckHierarchy(pAccessor, pComp, IsObjectDefault);
  }

  pAccessor->FinishTransaction();

  ProcessEvents(10);
  CloseSimpleScene();
}


void xiiEditorSceneDocumentTest::CheckHierarchy(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pRoot, xiiDelegate<void(const xiiDocumentObject* pChild)> functor)
{
  xiiDeque<const xiiDocumentObject*> objects;
  objects.PushBack(pRoot);
  while (!objects.IsEmpty())
  {
    const xiiDocumentObject* pCurrent = objects[0];
    objects.PopFront();
    {
      functor(pCurrent);
    }

    for (auto pChild : pCurrent->GetChildren())
    {
      objects.PushBack(pChild);
    }
  }
}
