/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFrameworkTest/RenderPipelineSettingsEditorTest.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderPipelineSettings.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QApplication>
#include <QElapsedTimer>
#include <QMenu>
#include <QPointer>
#include <QToolButton>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

void TestRenderPipelineSettingsWindow(xiiQtEditorApp& editor, xiiStringView projectDirectory)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Production pipeline window selects pipeline properties when graph selection clears")
  {
    xiiStringBuilder path(projectDirectory, "/Settings.xiiRenderPipelineAsset");
    auto*            document = xiiDynamicCast<xiiRenderPipelineAssetDocument*>(editor.CreateDocument(path, xiiDocumentFlags::RequestWindow));
    if (!XII_TEST_BOOL(document != nullptr))
      return;
    XII_SCOPE_EXIT(if (document) document->GetDocumentManager()->CloseDocument(document));
    document->GetDocumentManager()->EnsureWindowRequested(document);
    auto* window = xiiQtDocumentWindow::FindWindowByDocument(document);
    if (!XII_TEST_BOOL(window != nullptr))
      return;
    auto* quality = window->findChild<QToolButton*>("RenderPipelineQuality");
    if (!XII_TEST_BOOL(quality && quality->menu()))
      return;
    auto* accessor = document->GetObjectAccessor();
    accessor->StartTransaction("Create quality passes");
    xiiUuid     ids[3];
    const char* types[] = {"xiiShadowCascadePass", "xiiTemporalAntiAliasingPass", "xiiBloomPass"};
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      XII_TEST_STATUS(accessor->AddObjectByName(document->GetObjectManager()->GetRootObject(), "Children", -1, xiiRTTI::FindTypeByName(types[i]), ids[i]));
    }
    accessor->FinishTransaction();
    QMetaObject::invokeMethod(quality->menu(), "aboutToShow", Qt::DirectConnection);
    const auto categories = quality->menu()->actions();
    for (xiiUInt32 i = 0; i < 3; ++i)
    {
      if (!XII_TEST_BOOL(categories[i]->menu() && !categories[i]->menu()->actions().isEmpty()))
        return;
      categories[i]->menu()->actions().front()->trigger();
      XII_TEST_BOOL(document->GetSelectionManager()->GetCurrentObject()->GetGuid() == ids[i]);
    }
    // Selection shortcuts edit the original document. Remove the fixture nodes through its command history.
    XII_TEST_STATUS(document->GetCommandHistory()->Undo());
    document->GetSelectionManager()->Clear();
    const auto* settings = document->GetSettingsObject();
    if (!XII_TEST_BOOL(settings != nullptr))
      return;
    const auto id      = settings->GetGuid();
    const auto inspect = [&]() {
      QElapsedTimer elapsed;
      elapsed.start();
      while (document->GetSelectionManager()->GetCurrentObject() != settings && elapsed.elapsed() < 2000)
        QApplication::processEvents(QEventLoop::AllEvents, 20);
      XII_TEST_BOOL(document->GetSelectionManager()->GetCurrentObject() == settings);
      bool format = false, features = false;
      for (const auto* property : window->findChildren<xiiQtPropertyWidget*>())
      {
        if (!property->GetProperty())
          continue;
        format |= property->GetProperty()->GetPropertyName() == "OutputFormat";
        features |= property->GetProperty()->GetPropertyName() == "RequiredDeviceFeatures";
      }
      XII_TEST_BOOL(format && features);
    };
    inspect();
    auto* commands = document->GetObjectAccessor();
    commands->StartTransaction("Configure pipeline contract");
    XII_TEST_STATUS(commands->SetValueByName(settings, "OutputSamples", 1U));
    commands->FinishTransaction();
    XII_TEST_STATUS(document->GetCommandHistory()->Undo());
    XII_TEST_INT(settings->GetTypeAccessor().GetValue("OutputSamples").Get<xiiUInt32>(), 0);
    XII_TEST_STATUS(document->GetCommandHistory()->Redo());
    document->GetSelectionManager()->Clear();
    inspect();
    XII_TEST_STATUS(document->SaveDocument());
    document->GetDocumentManager()->CloseDocument(document);
    document = xiiDynamicCast<xiiRenderPipelineAssetDocument*>(editor.OpenDocument(path, xiiDocumentFlags::RequestWindow));
    if (!XII_TEST_BOOL(document != nullptr))
      return;
    document->GetDocumentManager()->EnsureWindowRequested(document);
    window   = xiiQtDocumentWindow::FindWindowByDocument(document);
    settings = document->GetSettingsObject();
    if (!XII_TEST_BOOL(window && settings))
      return;
    XII_TEST_BOOL(settings->GetGuid() == id);
    XII_TEST_INT(settings->GetTypeAccessor().GetValue("OutputSamples").Get<xiiUInt32>(), 1);
    inspect();
  }
}
