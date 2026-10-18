/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>
#include <Inspector/ProfilingWidget.h>
#include <Inspector/RenderGraphWidget.h>
#include <QApplication>
#include <QComboBox>
#include <QGraphicsScene>
#include <QImage>
#include <QJsonDocument>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <TestFramework/Framework/TestFramework.h>
#include <ads/DockManager.h>

XII_CREATE_SIMPLE_TEST_GROUP(Inspector);

namespace
{
  QJsonObject MakeGraphCapture()
  {
    xiiRenderGraph graph("View \"inspector\" — capture");
    graph.BeginSetup(9007199254740993ULL);
    struct Data
    {
      xiiRenderGraphBufferHandle m_Buffer;
    };
    xiiRenderGraphBufferHandle buffer;
    graph.AddPass<Data>("Upload", xiiGALCommandQueueFlags::Transfer, [&buffer](Data& data, xiiRenderGraphBuilder& builder) {
      xiiGALBufferCreationDescription description;
      description.m_uiSize = 256;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      description.m_Mode = xiiGALBufferMode::Raw;
      buffer = data.m_Buffer = builder.WriteBuffer("Shared data", description, xiiGALResourceStateFlags::CopyDestination); }, [](const Data&, xiiRenderGraphPassContext&) {});
    graph.AddPass<Data>("Consume", xiiGALCommandQueueFlags::Graphics, [buffer](Data& data, xiiRenderGraphBuilder& builder) { data.m_Buffer = builder.ReadBuffer(buffer, xiiGALResourceStateFlags::ShaderResource); }, [](const Data&, xiiRenderGraphPassContext&) {}, true);
    graph.EndSetup();
    if (!XII_TEST_RESULT(graph.Compile()))
      return {};
    xiiContiguousMemoryStreamStorage storage;
    xiiMemoryStreamWriter            writer(&storage);
    XII_TEST_RESULT(xiiRenderGraphDebug::WriteSnapshot({graph, nullptr, graph.GetFrameIndex(), true, {}}, writer));
    return QJsonDocument::fromJson(QByteArray(reinterpret_cast<const char*>(storage.GetData()), storage.GetStorageSize32())).object();
  }
  QJsonObject MakeProfile()
  {
    return QJsonDocument::fromJson(R"trace({"traceEvents":[
      {"ph":"M","pid":1,"tid":0,"name":"thread_name","args":{"name":"Frames"}},
      {"ph":"M","pid":1,"tid":5,"name":"thread_name","args":{"name":"Render thread"}},
      {"ph":"X","pid":1,"tid":0,"name":"Frame 42","ts":1000,"dur":10000},
      {"ph":"B","pid":1,"tid":5,"name":"Render","ts":1000},
      {"ph":"E","pid":1,"tid":5,"name":"Render","ts":11000},
      {"ph":"B","pid":1,"tid":5,"name":"Draw","ts":2000,"args":{"function":"DrawWorld()"}},
      {"ph":"E","pid":1,"tid":5,"name":"Draw","ts":6000},
      {"ph":"X","pid":2,"tid":5,"name":"Render","ts":2000,"dur":3000}
    ]})trace")
      .object();
  }
} // namespace

XII_CREATE_SIMPLE_TEST(Inspector, GraphCaptureAndOfflineUi)
{
  const auto capture = MakeGraphCapture();
  XII_TEST_STRING(capture["frame"].toString().toUtf8().constData(), "9007199254740993");
  XII_TEST_INT(capture["passes"].toArray().size(), 2);
  XII_TEST_INT(capture["resources"].toArray().size(), 1);
  XII_TEST_BOOL(!capture["passes"].toArray()[0].toObject()["timingAvailable"].toBool());
  XII_TEST_BOOL(capture["passes"].toArray()[1].toObject()["dependencies"].toArray().contains(0));
  ads::CDockManager      manager;
  xiiQtRenderGraphWidget widget(&manager);
  XII_TEST_BOOL(widget.LoadCapture(QJsonDocument(capture).toJson()));
  auto* passes    = widget.findChild<QTreeWidget*>("Graph passes");
  auto* resources = widget.findChild<QTreeWidget*>("Graph resources");
  XII_TEST_BOOL(passes && resources);
  if (!passes || !resources)
    return;
  XII_TEST_INT(passes->topLevelItemCount(), 2);
  XII_TEST_INT(resources->topLevelItemCount(), 1);
  passes->setCurrentItem(passes->topLevelItem(0));
  XII_TEST_BOOL(!widget.findChild<QPlainTextEdit*>()->toPlainText().isEmpty());
  widget.findChild<QLineEdit*>()->setText("Consume");
  int visible = 0;
  for (int i = 0; i < passes->topLevelItemCount(); ++i)
    visible += !passes->topLevelItem(i)->isHidden();
  XII_TEST_INT(visible, 1);
  XII_TEST_BOOL(!widget.LoadCapture("{broken"));
  auto invalid       = capture;
  invalid["version"] = 99;
  XII_TEST_BOOL(!widget.ApplyCapture(invalid));
  XII_TEST_INT(passes->topLevelItemCount(), 2);
  widget.findChild<QLineEdit*>()->clear();
  widget.resize(1200, 800);
  widget.show();
  QApplication::processEvents();
  QImage image(widget.size(), QImage::Format_ARGB32);
  image.fill(Qt::transparent);
  widget.render(&image);
  image.save("D:/XII/Workspace/inspector-graph-preview.png");
  widget.ResetStats();
  XII_TEST_INT(passes->topLevelItemCount(), 0);
}

XII_CREATE_SIMPLE_TEST(Inspector, ProfilingScopesAndSelfTime)
{
  QVector<xiiQtProfilingWidget::Scope> scopes;
  QMap<QString, QString>               lanes;
  const auto                           capture = MakeProfile();
  XII_TEST_BOOL(xiiQtProfilingWidget::Decode(capture, scopes, lanes));
  XII_TEST_INT(scopes.size(), 4);
  XII_TEST_BOOL(lanes.contains("1:5") && lanes.contains("2:5"));
  ads::CDockManager    manager;
  xiiQtProfilingWidget widget(&manager);
  XII_TEST_BOOL(widget.ApplyCapture(capture));
  auto* hotspots = widget.findChild<QTreeWidget*>("Profiling hotspots");
  XII_TEST_BOOL(hotspots != nullptr);
  if (!hotspots)
    return;
  bool found = false;
  for (int i = 0; i < hotspots->topLevelItemCount(); ++i)
  {
    auto* item = hotspots->topLevelItem(i);
    if (item->text(0) == "Render" && item->text(1) == "Render thread")
    {
      XII_TEST_FLOAT(item->text(3).toDouble(), 10, 0.0001);
      XII_TEST_FLOAT(item->text(4).toDouble(), 6, 0.0001);
      found = true;
    }
  }
  XII_TEST_BOOL(found);
  const int before = hotspots->topLevelItemCount();
  XII_TEST_BOOL(!widget.LoadCapture(R"({"traceEvents":[{"ph":"X","ts":0,"dur":-1}]})"));
  XII_TEST_INT(hotspots->topLevelItemCount(), before);
  XII_TEST_BOOL(widget.ApplyCapture(capture));
  widget.resize(1200, 800);
  widget.show();
  QApplication::processEvents();
  QImage image(widget.size(), QImage::Format_ARGB32);
  image.fill(Qt::transparent);
  widget.render(&image);
  image.save("D:/XII/Workspace/inspector-profile-preview.png");
  widget.ResetStats();
  XII_TEST_INT(hotspots->topLevelItemCount(), 0);
}

XII_CREATE_SIMPLE_TEST(Inspector, CaptureLimitsAndUnavailableTiming)
{
  xiiRenderGraphTimestampProfiler profiler;
  float                           ms    = 17;
  xiiUInt64                       frame = 42;
  XII_TEST_BOOL(!profiler.GetPassTiming(1, 0, "Render", ms, frame));
  XII_TEST_FLOAT(ms, 17, 0);
  XII_TEST_INT(frame, 42);
  ads::CDockManager      manager;
  xiiQtRenderGraphWidget graph(&manager);
  auto                   capture = MakeGraphCapture();
  auto                   passes  = capture["passes"].toArray();
  passes.append(passes[0]);
  capture["passes"] = passes;
  XII_TEST_BOOL(!graph.ApplyCapture(capture));
  const auto first = MakeGraphCapture(), second = MakeGraphCapture();
  XII_TEST_BOOL(first["id"] == second["id"] && first["instance"] != second["instance"]);
  XII_TEST_BOOL(graph.ApplyCapture(first));
  XII_TEST_BOOL(graph.ApplyCapture(second));
  XII_TEST_INT(graph.findChild<QComboBox*>()->count(), 2);
  xiiQtProfilingWidget profile(&manager);
  profile.ApplyGraphCapture(MakeGraphCapture());
  auto* costs = profile.findChild<QTreeWidget*>("GPU pass costs");
  XII_TEST_INT(costs->topLevelItemCount(), 2);
  XII_TEST_BOOL(costs->topLevelItem(0)->text(3) == "Unavailable");
}
