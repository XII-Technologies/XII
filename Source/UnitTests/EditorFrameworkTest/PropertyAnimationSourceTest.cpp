/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetManager.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

namespace
{
  // Create fixtures through the real document implementation without creating Qt windows or an asset-curator project.
  class PropertyAnimationSourceManager : public xiiAssetDocumentManager
  {
  public:
    PropertyAnimationSourceManager()
    {
      m_Type.m_sDocumentTypeName  = "PropertyAnim";
      m_Type.m_sFileExtension     = "xiiPropertyAnimAsset";
      m_Type.m_pDocumentType      = xiiGetStaticRTTI<xiiPropertyAnimAssetDocument>();
      m_Type.m_pManager           = this;
      m_Type.m_AssetDocumentFlags = xiiAssetDocumentFlags::DisableTransform;
    }

    ~PropertyAnimationSourceManager() override { CloseAllDocumentsOfManager(); }

  private:
    xiiStatus InternalCreateDocument(xiiStringView, xiiStringView path, bool, xiiDocument*& out_document, const xiiDocumentObject*) override
    {
      out_document = new xiiPropertyAnimAssetDocument(path);
      return XII_SUCCESS;
    }
    void                           InternalGetSupportedDocumentTypes(xiiDynamicArray<const xiiDocumentTypeDescriptor*>& types) const override { types.PushBack(&m_Type); }
    bool                           GeneratesProfileSpecificAssets() const override { return false; }
    xiiAssetDocumentTypeDescriptor m_Type;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(EditorDocuments, PropertyAnimationSource)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Legacy channel migration preserves identity and bindings")
  {
    XII_TEST_BOOL(xiiRTTI::FindTypeByName("xiiPropertyAnimTarget") == nullptr);
    // Documents carry a type graph. Without it GraphVersioning assumes the currently registered type version.
    xiiAbstractObjectGraph types;
    auto*                  type = types.AddNode(xiiUuid::MakeUuid(), "xiiReflectedTypeDescriptor", 1);
    type->AddProperty("TypeName", "xiiPropertyAnimationTrack");
    type->AddProperty("ParentTypeName", "xiiReflectedClass");
    type->AddProperty("TypeVersion", xiiUInt32(1));
    const char* channels[] = {"Number", "VectorX", "VectorY", "VectorZ", "VectorW", "RotationX", "RotationY", "RotationZ", "Color"};
    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(channels); ++i)
    {
      const auto             id = xiiUuid::MakeUuid();
      xiiAbstractObjectGraph graph;
      auto*                  node = graph.AddNode(id, "xiiPropertyAnimationTrack", 1, "Track");
      node->AddProperty("Target", xiiStringBuilder("xiiPropertyAnimTarget::", channels[i]).GetData());
      node->AddProperty("ObjectPath", "Root/Child");
      node->AddProperty("ComponentType", "xiiMeshComponent");
      node->AddProperty("Property", "TintColor");

      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         writer(&storage);
      xiiAbstractGraphBinarySerializer::Write(writer, &graph, &types);
      xiiAbstractObjectGraph loaded, loadedTypes;
      xiiMemoryStreamReader  reader(&storage);
      xiiAbstractGraphBinarySerializer::Read(reader, &loaded, &loadedTypes, true);
      const auto* migrated = loaded.GetNode(id);
      if (!XII_TEST_BOOL(migrated != nullptr))
        continue;
      XII_TEST_INT(migrated->GetTypeVersion(), 2);
      XII_TEST_STRING(migrated->FindProperty("Target")->m_Value.Get<xiiString>(), xiiStringBuilder("xiiPropertyAnimationChannel::", channels[i]));
      xiiPropertyAnimationTrack track;
      xiiRttiConverterContext   context;
      xiiRttiConverterReader    converter(&loaded, &context);
      converter.ApplyPropertiesToObject(migrated, xiiGetStaticRTTI<xiiPropertyAnimationTrack>(), &track);
      XII_TEST_INT(track.m_Target.GetValue(), i);
      XII_TEST_STRING(track.m_sObjectSearchSequence, "Root/Child");
      XII_TEST_STRING(track.m_sComponentType, "xiiMeshComponent");
      XII_TEST_STRING(track.m_sPropertyPath, "TintColor");
    }

    xiiAbstractObjectGraph graph;
    auto*                  numeric = graph.AddNode(xiiUuid::MakeUuid(), "xiiPropertyAnimationTrack", 1);
    numeric->AddProperty("Target", xiiInt64(xiiPropertyAnimationChannel::Color));
    xiiGraphVersioning::GetSingleton()->PatchGraph(&graph, &types);
    XII_TEST_INT(numeric->FindProperty("Target")->m_Value.Get<xiiInt64>(), xiiPropertyAnimationChannel::Color);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Source editing, undo and disk round trip without engine IPC")
  {
    xiiStringBuilder path = xiiTestFramework::GetInstance()->GetAbsOutputPath();
    path.AppendPath("PropertyAnimationSource");
    XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(path));
    XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(path, "PropertyAnimationSource", ":", xiiDataDirUsage::AllowWrites));
    XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("PropertyAnimationSource"));
    path.AppendPath("Source.xiiPropertyAnimAsset");
    PropertyAnimationSourceManager manager;
    xiiDocument*                   base = nullptr;
    XII_TEST_STATUS(manager.CreateDocument("PropertyAnim", path, base, xiiDocumentFlags::EmptyDocument));
    if (!XII_TEST_BOOL(base != nullptr))
      return;
    auto*      document   = static_cast<xiiPropertyAnimAssetDocument*>(base);
    const auto documentId = document->GetGuid();
    XII_TEST_BOOL(document->GetEditorEngineConnection() == nullptr);
    XII_TEST_BOOL(document->GetEngineStatus() == xiiAssetDocument::EngineStatus::Unsupported);
    auto* accessor = document->GetObjectAccessor();
    accessor->StartTransaction("Create source tracks");
    xiiUuid scalar, color;
    XII_TEST_STATUS(accessor->AddObjectByName(document->GetPropertyObject(), "Tracks", -1, xiiGetStaticRTTI<xiiPropertyAnimationTrack>(), scalar));
    XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(scalar), "ObjectPath", "Root/Child"));
    XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(scalar), "Property", "LocalScaling"));
    XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(scalar), "Target", xiiInt64(xiiPropertyAnimationChannel::VectorZ)));
    XII_TEST_STATUS(accessor->AddObjectByName(document->GetPropertyObject(), "Tracks", -1, xiiGetStaticRTTI<xiiPropertyAnimationTrack>(), color));
    XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(color), "Target", xiiInt64(xiiPropertyAnimationChannel::Color)));
    accessor->FinishTransaction();
    const auto curvePoint = document->InsertCurveCpAt(scalar, 240, 7.5);
    document->InsertGradientColorCpAt(color, 120, xiiColorGammaUB(17, 34, 51));
    document->InsertGradientAlphaCpAt(color, 360, 127);
    document->InsertGradientIntensityCpAt(color, 480, 2.5f);
    const auto eventPoint = document->InsertEventTrackCpAt(720, "SourceEvent");
    XII_TEST_STATUS(document->GetCommandHistory()->Undo());
    XII_TEST_INT(document->GetProperties()->m_EventTrack.m_ControlPoints.GetCount(), 0);
    XII_TEST_STATUS(document->GetCommandHistory()->Redo());
    XII_TEST_BOOL(accessor->GetObject(eventPoint) != nullptr);
    document->SetAnimationDurationTicks(960);
    XII_TEST_STATUS(document->SaveDocument());
    manager.CloseDocument(document);

    xiiPropertyAnimAssetDocumentManager                 productionManager;
    xiiHybridArray<const xiiDocumentTypeDescriptor*, 4> descriptors;
    productionManager.GetSupportedDocumentTypes(descriptors);
    XII_TEST_INT(descriptors.GetCount(), 1);
    const auto* descriptor = static_cast<const xiiAssetDocumentTypeDescriptor*>(descriptors[0]);
    XII_TEST_BOOL(!descriptor->m_bCanCreate);
    XII_TEST_BOOL(descriptor->m_AssetDocumentFlags.IsSet(xiiAssetDocumentFlags::DisableTransform));
    XII_TEST_BOOL(descriptor->m_sResourceFileExtension.IsEmpty());
    XII_TEST_BOOL(descriptor->m_CompatibleTypes.IsEmpty());
    XII_TEST_BOOL(productionManager.GetAssetTableEntry(nullptr, {}, nullptr).IsEmpty());
    base = nullptr;
    XII_TEST_BOOL(productionManager.CreateDocument("PropertyAnim", path, base, xiiDocumentFlags::EmptyDocument).Failed());
    XII_TEST_BOOL(base == nullptr);
    XII_TEST_STATUS(productionManager.OpenDocument("PropertyAnim", path, base, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(base != nullptr))
      return;
    document = static_cast<xiiPropertyAnimAssetDocument*>(base);
    XII_TEST_BOOL(document->GetGuid() == documentId);
    XII_TEST_INT(document->GetUnknownObjectTypeInstances(), 0);
    XII_TEST_BOOL(document->GetObjectAccessor()->GetObject(scalar) != nullptr);
    XII_TEST_BOOL(document->GetObjectAccessor()->GetObject(curvePoint) != nullptr);
    XII_TEST_BOOL(document->GetObjectAccessor()->GetObject(eventPoint) != nullptr);
    const auto* data = document->GetProperties();
    if (!XII_TEST_INT(data->m_Tracks.GetCount(), 2))
      return;
    XII_TEST_INT(data->m_uiCurveDuration, 960);
    XII_TEST_STRING(data->m_Tracks[0]->m_sObjectSearchSequence, "Root/Child");
    XII_TEST_INT(data->m_Tracks[0]->m_Target, xiiPropertyAnimationChannel::VectorZ);
    XII_TEST_DOUBLE(data->m_Tracks[0]->m_FloatCurve.m_ControlPoints[0].m_fValue, 7.5, 0);
    XII_TEST_INT(data->m_Tracks[1]->m_ColorGradient.m_ColorCPs[0].m_Red, 17);
    XII_TEST_INT(data->m_Tracks[1]->m_ColorGradient.m_AlphaCPs[0].m_Alpha, 127);
    XII_TEST_FLOAT(data->m_Tracks[1]->m_ColorGradient.m_IntensityCPs[0].m_fIntensity, 2.5f, 0);
    XII_TEST_INT(data->m_EventTrack.m_ControlPoints[0].m_iTick, 720);
    XII_TEST_STRING(data->m_EventTrack.m_ControlPoints[0].GetEventName(), "SourceEvent");
    // Malformed legacy FPS values must not divide by zero or produce a zero frame step.
    for (xiiUInt32 fps : {0U, 10000U})
    {
      auto* currentAccessor = document->GetObjectAccessor();
      currentAccessor->StartTransaction("Invalid legacy FPS");
      XII_TEST_STATUS(currentAccessor->SetValueByName(document->GetPropertyObject(), "FPS", fps));
      currentAccessor->FinishTransaction();
      document->SetScrubberPosition(240);
      XII_TEST_BOOL(document->GetScrubberPosition() <= document->GetAnimationDurationTicks());
    }
    productionManager.CloseDocument(document);
  }
}
