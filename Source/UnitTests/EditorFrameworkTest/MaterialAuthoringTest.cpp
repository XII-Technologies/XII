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
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

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
[SHADER]
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiMaterialConstants)
{
  COLOR4F(Tint);
  FLOAT1(Gain);
}
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
