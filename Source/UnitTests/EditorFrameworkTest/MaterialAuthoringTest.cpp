/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/VisualShader/VsCodeGenerator.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderTextSectionizer.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <QApplication>
#include <QComboBox>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

namespace
{
  // Deliberately admit malformed imported topology so compiler validation is
  // exercised independently of the Editor's interactive connection policy.
  class MaterialTopologyTestManager : public xiiVisualShaderNodeManager
  {
  public:
    xiiStatus InternalCanConnect(const xiiPin&, const xiiPin&, CanConnectResult& result) const override
    {
      result = CanConnectResult::ConnectNto1;
      return XII_SUCCESS;
    }
  };

  class MaterialTopologyTestDocument : public xiiDocument
  {
  public:
    explicit MaterialTopologyTestDocument(xiiStringView path) : xiiDocument(path, XII_DEFAULT_NEW(MaterialTopologyTestManager))
    {
      m_pDocumentInfo   = CreateDocumentInfo();
      m_pObjectAccessor = XII_DEFAULT_NEW(xiiNodeCommandAccessor, GetCommandHistory());
    }
    xiiDocumentInfo* CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }
  };
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(MaterialAuthoring);
XII_CREATE_SIMPLE_TEST(MaterialAuthoring, NativeAssetAndGraphHistory)
{
  xiiStartup::StartupCoreSystems();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  auto*                  editor = xiiQtEditorApp::GetSingleton();
  const auto             output = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  const xiiStringBuilder preferences(output, "/MaterialPreferences");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(preferences));
  editor->StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(editor->CloseProject(); QApplication::processEvents(); editor->ShutdownEditor());
  const xiiStringBuilder project(output, "/MaterialProject/xiiProject");
  if (!XII_TEST_RESULT(editor->CreateOrOpenProject(true, project)))
    return;
  xiiVisualShaderTypeRegistry::GetSingleton()->UpdateNodeData();
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Pattern material preserves typed roughness connections")
  {
    const xiiStringBuilder original(xiiFileSystem::GetSdkRootDirectory(), "/Data/Base/Materials/Common/Pattern.xiiMaterialAsset");
    const xiiStringBuilder copy(project.GetFileDirectory(), "/Pattern.xiiMaterialAsset");
    XII_TEST_RESULT(xiiOSFile::CopyFile(original, copy));
    auto* pattern = xiiDynamicCast<xiiMaterialAssetDocument*>(editor->OpenDocument(copy, xiiDocumentFlags::None));
    if (!XII_TEST_BOOL(pattern != nullptr))
      return;
    XII_SCOPE_EXIT(pattern->GetDocumentManager()->CloseDocument(pattern));
    const auto id          = pattern->GetGuid();
    auto*      nodes       = static_cast<xiiVisualShaderNodeManager*>(pattern->GetObjectManager());
    xiiUInt32  connections = 0;
    for (const auto* object : nodes->GetRootObject()->GetChildren())
      if (nodes->IsConnection(object))
        ++connections;
    XII_TEST_INT(connections, 12);
    xiiVisualShaderCodeGenerator generator;
    xiiStringBuilder             permutations;
    XII_TEST_STATUS(generator.GenerateVisualShader(nodes, permutations));
    XII_TEST_STATUS(pattern->CompileShader());
    XII_TEST_BOOL(pattern->GetShaderPropertyObject() != nullptr);
    XII_TEST_STATUS(pattern->SaveDocument());
    XII_TEST_BOOL(pattern->GetGuid() == id);
  }
  const xiiStringBuilder path(project.GetFileDirectory(), "/Inspection.xiiMaterialAsset");
  auto*                  document = xiiDynamicCast<xiiMaterialAssetDocument*>(editor->CreateDocument(path, xiiDocumentFlags::EmptyDocument | xiiDocumentFlags::RequestWindow));
  if (!XII_TEST_BOOL(document != nullptr))
    return;
  XII_SCOPE_EXIT(if (document != nullptr) document->GetDocumentManager()->CloseDocument(document));
  const auto  identity     = document->GetGuid();
  const auto* defaultLight = static_cast<const xiiEngineViewLightSettings*>(document->FindSyncObject(xiiGetStaticRTTI<xiiEngineViewLightSettings>()));
  if (!XII_TEST_BOOL(defaultLight != nullptr))
    return;
  XII_TEST_BOOL(!defaultLight->GetDirectionalLight());
  auto* accessor = document->GetObjectAccessor();
  auto* manager  = static_cast<xiiVisualShaderNodeManager*>(document->GetObjectManager());
  XII_TEST_BOOL(document->GetProperties()->GetShaderMode() == xiiMaterialShaderMode::Native);
  const auto& native = document->GetProperties()->m_NativeMaterial;
  XII_TEST_BOOL(native.m_EmissiveColor == xiiColor::Black);
  XII_TEST_FLOAT(native.m_fRoughness, 0.5f, 0);
  XII_TEST_FLOAT(native.m_fNormalScale, 1, 0);
  XII_TEST_FLOAT(native.m_fIndexOfRefraction, 1.5f, 0);
  xiiDefaultMemoryStreamStorage standardStorage;
  xiiMemoryStreamWriter         standardWriter(&standardStorage);
  XII_TEST_STATUS(document->WriteMaterialAsset(standardWriter));
  xiiMemoryStreamReader standardReader(&standardStorage);
  xiiUInt8              version = 0;
  standardReader >> version;
  XII_TEST_INT(version, 8);

  xiiUuid outputNode, valueNode, disconnected;
  accessor->StartTransaction("Author material graph");
  XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName("ShaderNode::MaterialOutput"), outputNode));
  XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName("ShaderNode::Constant1"), valueNode));
  XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName("ShaderNode::Add"), disconnected));
  XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(valueNode), "Value", 0.42f));
  XII_TEST_STATUS(xiiNodeCommands::AddAndConnectCommand(document->GetCommandHistory(), manager->GetConnectionType(), *manager->GetOutputPinByName(accessor->GetObject(valueNode), "Value"), *manager->GetInputPinByName(accessor->GetObject(outputNode), "Roughness")));
  accessor->FinishTransaction();
  const auto generate = [&]() {
    xiiVisualShaderCodeGenerator generator;
    xiiStringBuilder             permutations;
    XII_TEST_STATUS(generator.GenerateVisualShader(manager, permutations));
    return xiiString(generator.GetFinalShaderCode());
  };
  const xiiString source = generate();
  XII_TEST_BOOL(source.FindSubString("0.42") != nullptr);
  xiiGALShaderTextSectionizer sections;
  xiiGALShaderSections::GetShaderSections(source, sections);
  xiiUInt32 line = 0;
  XII_TEST_BOOL(!sections.GetSectionContent(xiiGALShaderSections::VertexShader, line).IsEmpty());
  XII_TEST_BOOL(!sections.GetSectionContent(xiiGALShaderSections::PixelShader, line).IsEmpty());
  XII_TEST_BOOL(!sections.GetSectionContent(xiiGALShaderSections::MaterialParameter, line).IsEmpty());
  XII_TEST_BOOL(!sections.GetSectionContent(xiiGALShaderSections::MaterialConstants, line).IsEmpty());
  XII_TEST_BOOL(source.FindSubString("DECLARE_CONSTANT_BUFFER_AUTO(xiiMaterialConstants)") == nullptr);
  XII_TEST_BOOL(source.FindSubString("VSE_CONSTANTS") == nullptr);
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Core Graph rejects imported cycles with node and port diagnostics")
  {
    MaterialTopologyTestDocument malformed(xiiStringBuilder(output, "/MalformedMaterialGraph.xiiGraph"));
    auto*                        commands        = malformed.GetObjectAccessor();
    auto*                        topologyManager = static_cast<MaterialTopologyTestManager*>(malformed.GetObjectManager());
    xiiUuid                      nodes[3];
    commands->StartTransaction("Import malformed topology");
    for (xiiUInt32 index = 0; index < 3; ++index)
    {
      XII_TEST_STATUS(commands->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName(index == 2 ? "ShaderNode::MaterialOutput" : "ShaderNode::Reroute"), nodes[index]));
    }
    const auto connect = [&](xiiUInt32 from, xiiUInt32 to, const char* pin) {
      XII_TEST_STATUS(xiiNodeCommands::AddAndConnectCommand(malformed.GetCommandHistory(), topologyManager->GetConnectionType(), *topologyManager->GetOutputPinByName(commands->GetObject(nodes[from]), "Value"), *topologyManager->GetInputPinByName(commands->GetObject(nodes[to]), pin)));
    };
    connect(0, 1, "Value");
    connect(1, 0, "Value");
    connect(1, 2, "Metallic");
    commands->FinishTransaction();
    xiiVisualShaderCodeGenerator generator;
    xiiStringBuilder             permutations;
    const auto                   status = generator.GenerateVisualShader(topologyManager, permutations);
    XII_TEST_BOOL(status.Failed());
    XII_TEST_BOOL(status.GetMessageString().FindSubString("cycle") != nullptr);
    XII_TEST_BOOL(status.GetMessageString().FindSubString("Value") != nullptr);
    XII_TEST_BOOL(generator.GetFinalShaderCode().IsEmpty());
  }
  accessor->StartTransaction("Remove unreachable graph node");
  XII_TEST_STATUS(accessor->RemoveObject(accessor->GetObject(disconnected)));
  accessor->FinishTransaction();
  XII_TEST_STRING(generate(), source);
  XII_TEST_STATUS(document->GetCommandHistory()->Undo());
  XII_TEST_STRING(generate(), source);
  XII_TEST_STATUS(document->GetCommandHistory()->Redo());
  XII_TEST_STRING(generate(), source);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Numeric dimensions, explicit conversion and cycle rejection")
  {
    xiiUuid vectorNode, firstMath, secondMath;
    accessor->StartTransaction("Add typed graph nodes");
    XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName("ShaderNode::Constant3"), vectorNode));
    XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName("ShaderNode::Add"), firstMath));
    XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName("ShaderNode::Reroute"), secondMath));
    const auto*                              vector = manager->GetOutputPinByName(accessor->GetObject(vectorNode), "Value");
    const auto*                              scalar = manager->GetInputPinByName(accessor->GetObject(outputNode), "Metallic");
    xiiDocumentNodeManager::CanConnectResult result;
    XII_TEST_BOOL(manager->CanConnect(manager->GetConnectionType(), *vector, *scalar, result).Failed());
    XII_TEST_STATUS(xiiNodeCommands::AddAndConnectCommand(document->GetCommandHistory(), manager->GetConnectionType(), *vector, *manager->GetInputPinByName(accessor->GetObject(firstMath), "a")));
    XII_TEST_STATUS(xiiNodeCommands::AddAndConnectCommand(document->GetCommandHistory(), manager->GetConnectionType(), *manager->GetOutputPinByName(accessor->GetObject(firstMath), "result"), *manager->GetInputPinByName(accessor->GetObject(secondMath), "Value")));
    XII_TEST_BOOL(manager->CanConnect(manager->GetConnectionType(), *manager->GetOutputPinByName(accessor->GetObject(secondMath), "Value"), *manager->GetInputPinByName(accessor->GetObject(firstMath), "b"), result).Failed());
    accessor->FinishTransaction();
    XII_TEST_STRING(generate(), source);
  }

  accessor->StartTransaction("Switch to graph authoring");
  XII_TEST_STATUS(accessor->SetValueByName(document->GetPropertyObject(), "ShaderMode", static_cast<xiiInt64>(xiiMaterialShaderMode::Custom)));
  accessor->FinishTransaction();
  XII_TEST_STATUS(document->CompileShader());
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Core Graph schedules shared dependencies once")
  {
    accessor->StartTransaction("Share a graph parameter");
    xiiUuid parameter, add;
    XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName("ShaderNode::Parameter1f"), parameter));
    XII_TEST_STATUS(accessor->SetValueByName(accessor->GetObject(parameter), "ParamName", "SharedGain"));
    XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName("ShaderNode::Add"), add));
    const auto* shared = manager->GetOutputPinByName(accessor->GetObject(parameter), "Value");
    for (const char* input : {"a", "b"})
    {
      XII_TEST_STATUS(xiiNodeCommands::AddAndConnectCommand(document->GetCommandHistory(), manager->GetConnectionType(), *shared, *manager->GetInputPinByName(accessor->GetObject(add), input)));
    }
    XII_TEST_STATUS(xiiNodeCommands::AddAndConnectCommand(document->GetCommandHistory(), manager->GetConnectionType(), *manager->GetOutputPinByName(accessor->GetObject(add), "result"), *manager->GetInputPinByName(accessor->GetObject(outputNode), "Metallic")));
    accessor->FinishTransaction();
    const auto                  generated = generate();
    xiiGALShaderTextSectionizer sharedSections;
    xiiGALShaderSections::GetShaderSections(generated, sharedSections);
    const auto  constants   = sharedSections.GetSectionContent(xiiGALShaderSections::MaterialConstants, line);
    const char* declaration = constants.FindSubString("FLOAT1(SharedGain)");
    XII_TEST_BOOL(declaration != nullptr);
    if (declaration != nullptr)
      XII_TEST_BOOL(xiiStringView(declaration + 1, constants.GetEndPointer()).FindSubString("FLOAT1(SharedGain)") == nullptr);
    XII_TEST_STATUS(document->CompileShader());
    XII_TEST_STATUS(document->GetCommandHistory()->Undo());
    XII_TEST_STRING(generate(), source);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Procedural graph compilation and unreachable textures")
  {
    accessor->StartTransaction("Author procedural surface");
    xiiUuid uv, noise, gradient, multiply;
    XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName("ShaderNode::UVTransform"), uv));
    XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName("ShaderNode::ValueNoise2D"), noise));
    XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName("ShaderNode::LinearGradient"), gradient));
    XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName("ShaderNode::Multiply"), multiply));
    const auto connect = [&](xiiUuid from, const char* outputName, xiiUuid to, const char* inputName) {
      XII_TEST_STATUS(xiiNodeCommands::AddAndConnectCommand(document->GetCommandHistory(), manager->GetConnectionType(), *manager->GetOutputPinByName(accessor->GetObject(from), outputName), *manager->GetInputPinByName(accessor->GetObject(to), inputName)));
    };
    connect(uv, "UV", noise, "Position");
    connect(noise, "Value", multiply, "a");
    connect(gradient, "Value", multiply, "b");
    connect(multiply, "result", outputNode, "Metallic");
    for (xiiUInt32 index = 0; index < 17; ++index)
    {
      xiiUuid texture;
      XII_TEST_STATUS(accessor->AddObject(nullptr, nullptr, -1, xiiRTTI::FindTypeByName("ShaderNode::BaseTexture"), texture));
    }
    accessor->FinishTransaction();
    const auto procedural = generate();
    XII_TEST_BOOL(procedural.FindSubString("xiiMaterialValueNoise2D") != nullptr);
    XII_TEST_BOOL(procedural.FindSubString("Texture2D BaseTexture") == nullptr);
    XII_TEST_STATUS(document->CompileShader());
    XII_TEST_STATUS(document->GetCommandHistory()->Undo());
    XII_TEST_STRING(generate(), source);
  }
  accessor->StartTransaction("Return to standard authoring");
  XII_TEST_STATUS(accessor->SetValueByName(document->GetPropertyObject(), "ShaderMode", static_cast<xiiInt64>(xiiMaterialShaderMode::Native)));
  accessor->FinishTransaction();
  XII_TEST_STRING(generate(), source);
  XII_TEST_BOOL(document->GetGuid() == identity);
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Independent handwritten shader parameters and native output")
  {
    const xiiStringBuilder shaderPath(project.GetFileDirectory(), "/Handwritten.xiiShader");
    xiiFileWriter          file;
    if (!XII_TEST_RESULT(file.Open(shaderPath)))
      return;
    const xiiStringView shaderSource = R"shader([PLATFORMS]
ALL
[MATERIAL_PARAMETER]
Color Tint @Default(Color(0.2, 0.4, 0.6));
float Gain @Default(0.5) @Clamp(0.0, 4.0);
[MATERIAL_CONSTANTS]
COLOR4F(Tint);
FLOAT1(Gain);
[VERTEX_SHADER]
float4 main(uint vertex : SV_VertexID) : SV_Position
{
  return float4(float2((vertex << 1u) & 2u, vertex & 2u) * 2.0 - 1.0, 0, 1);
}
[PIXEL_SHADER]
float4 main() : SV_Target0
{
  return Tint * Gain;
}
)shader";
    XII_TEST_RESULT(file.WriteBytes(shaderSource.GetStartPointer(), shaderSource.GetElementCount()));
    file.Close();
    accessor->StartTransaction("Select handwritten shader");
    XII_TEST_STATUS(accessor->SetValueByName(document->GetPropertyObject(), "ShaderMode", static_cast<xiiInt64>(xiiMaterialShaderMode::File)));
    XII_TEST_STATUS(accessor->SetValueByName(document->GetPropertyObject(), "Shader", "Handwritten.xiiShader"));
    accessor->FinishTransaction();
    const auto* parameters = document->GetShaderPropertyObject();
    if (!XII_TEST_BOOL(parameters != nullptr))
      return;
    XII_TEST_FLOAT(parameters->GetTypeAccessor().GetValue("Gain").ConvertTo<float>(), 0.5f, 0);
    accessor->StartTransaction("Override reflected shader parameter");
    XII_TEST_STATUS(accessor->SetValueByName(parameters, "Gain", 1.25f));
    accessor->FinishTransaction();
    XII_TEST_STATUS(document->CompileShader());
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    XII_TEST_STATUS(document->WriteMaterialAsset(writer));
    xiiMemoryStreamReader reader(&storage);
    xiiUInt8              format = 0, compression = 255;
    xiiString             base, surface, shader;
    reader >> format >> compression >> base >> surface >> shader;
    XII_TEST_INT(format, 8);
    XII_TEST_INT(compression, 0);
    XII_TEST_STRING(shader, "Handwritten.xiiShader");
    accessor->StartTransaction("Return to independent graph");
    XII_TEST_STATUS(accessor->SetValueByName(document->GetPropertyObject(), "ShaderMode", static_cast<xiiInt64>(xiiMaterialShaderMode::Custom)));
    accessor->FinishTransaction();
    accessor->StartTransaction("Restore handwritten shader values");
    XII_TEST_STATUS(accessor->SetValueByName(document->GetPropertyObject(), "ShaderMode", static_cast<xiiInt64>(xiiMaterialShaderMode::File)));
    accessor->FinishTransaction();
    XII_TEST_FLOAT(document->GetShaderPropertyObject()->GetTypeAccessor().GetValue("Gain").ConvertTo<float>(), 1.25f, 0);
    accessor->StartTransaction("Temporarily use standard surface");
    XII_TEST_STATUS(accessor->SetValueByName(document->GetPropertyObject(), "ShaderMode", static_cast<xiiInt64>(xiiMaterialShaderMode::Native)));
    accessor->FinishTransaction();
    accessor->StartTransaction("Restore custom shader");
    XII_TEST_STATUS(accessor->SetValueByName(document->GetPropertyObject(), "ShaderMode", static_cast<xiiInt64>(xiiMaterialShaderMode::File)));
    accessor->FinishTransaction();
    XII_TEST_FLOAT(document->GetShaderPropertyObject()->GetTypeAccessor().GetValue("Gain").ConvertTo<float>(), 1.25f, 0);
    XII_TEST_STRING(generate(), source);
  }
  XII_TEST_STATUS(document->SaveDocument());
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Production preview controls do not modify material authoring state")
  {
    auto* window = xiiQtDocumentWindow::FindWindowByDocument(document);
    if (!XII_TEST_BOOL(window != nullptr))
      return;
    QComboBox* lighting   = nullptr;
    QComboBox* diagnostic = nullptr;
    for (auto* widget : QApplication::allWidgets())
    {
      if (widget->objectName() == "MaterialPreviewLightingPreset")
        lighting = qobject_cast<QComboBox*>(widget);
      if (widget->objectName() == "MaterialPreviewDiagnosticView")
        diagnostic = qobject_cast<QComboBox*>(widget);
    }
    if (!XII_TEST_BOOL(lighting != nullptr && diagnostic != nullptr))
      return;
    XII_TEST_BOOL(!document->IsModified());
    for (int index = 0; index < 4; ++index)
    {
      lighting->setCurrentIndex(index);
      QApplication::processEvents();
      XII_TEST_BOOL(!document->IsModified());
      XII_TEST_BOOL(document->GetGuid() == identity);
    }
    diagnostic->setCurrentIndex(3);
    XII_TEST_BOOL(!document->IsModified());
  }

  document->GetDocumentManager()->CloseDocument(document);
  document = xiiDynamicCast<xiiMaterialAssetDocument*>(editor->OpenDocument(path, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(document != nullptr))
    return;
  XII_TEST_BOOL(document->GetGuid() == identity);
  accessor = document->GetObjectAccessor();
  manager  = static_cast<xiiVisualShaderNodeManager*>(document->GetObjectManager());
  XII_TEST_STRING(generate(), source);
  XII_TEST_FLOAT(document->GetShaderPropertyObject()->GetTypeAccessor().GetValue("Gain").ConvertTo<float>(), 1.25f, 0);
}
