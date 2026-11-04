/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Communication/DataTransfer.h>
#include <Foundation/IO/MemoryStream.h>
#include <Inspector/ProfilingSession.h>
#include <Inspector/ProfilingWidget.h>
#include <QApplication>
#include <QJsonDocument>
#include <QPushButton>
#include <QTreeWidget>
#include <TestFramework/Framework/TestFramework.h>
#include <ads/DockManager.h>

namespace
{
  using Session = xiiInspectorProfilingSession;
  Session::Data Batch(quint64 frame, double begin)
  {
    Session::Data batch;
    batch.m_fReceivedUs = begin + 10000;
    batch.m_Frames.push_back({frame, begin, begin + 10000});
    batch.m_Threads["77"] = "Main Thread";
    batch.m_Scopes.push_back({"Simulation", "77", "UpdateWorld", begin, begin + 8000, frame * 2});
    batch.m_Scopes.push_back({"Physics", "77", "Step", begin + 1000, begin + 4000, frame * 2 + 1});
    batch.m_Graphs[101] = "Scene / Pipeline";
    batch.m_Graphs[102] = "Scene / Pipeline";
    batch.m_Gpu.push_back({101, frame, 0, 0, false, true, 2.5, "Repeated pass", begin});
    batch.m_Gpu.push_back({102, frame, 0, 1, false, false, 0, "Repeated pass", begin});
    return batch;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(Inspector, IncrementalProtocolAndFrameIdentity)
{
  Session session;
  session.BeginStream(91);
  xiiContiguousMemoryStreamStorage storage;
  xiiMemoryStreamWriter            writer(&storage);
  writer << xiiUInt32(1) << xiiUInt64(91) << xiiUInt64(1) << double(20000) << xiiUInt64(3) << xiiUInt64(4) << xiiUInt64(5);
  writer << xiiUInt32(4) << "" << "Main Thread" << "Simulation" << "UpdateWorld";
  writer << xiiUInt32(2) << xiiUInt64(12345); // Application name and real process identity.
  writer << xiiUInt32(1) << xiiUInt64(77) << xiiUInt32(1);
  writer << xiiUInt32(1) << xiiUInt64(9007199254740993ULL) << double(10000) << double(20000);
  writer << xiiUInt32(1) << xiiUInt64(123) << xiiUInt64(77) << xiiUInt32(2) << xiiUInt32(3) << double(11000) << double(19000);
  writer << xiiUInt32(0); // Timestamped GPU scopes.
  writer << xiiUInt32(1) << xiiUInt64(101) << xiiUInt32(2) << xiiUInt32(0);
  writer << xiiUInt32(1) << xiiUInt64(101) << xiiUInt64(9007199254740991ULL) << xiiUInt32(0) << xiiUInt32(1) << false << true << double(4.25) << double(12000) << xiiUInt32(2);
  writer << xiiUInt32(0x50535452);
  xiiMemoryStreamReader reader(&storage);
  XII_TEST_BOOL(session.Receive(reader));
  XII_TEST_BOOL(session.m_Live.m_Frames[0].m_uiIndex == 9007199254740993ULL);
  XII_TEST_BOOL(session.m_Live.m_Gpu[0].m_uiFrame == 9007199254740991ULL);
  XII_TEST_BOOL(session.m_Live.m_Scopes[0].m_uiId == 123);
  XII_TEST_INT(session.m_Live.m_uiDroppedScopes, 3);
  XII_TEST_BOOL(session.m_Live.m_Application == "Simulation");
  XII_TEST_INT(session.m_Live.m_uiProcessId, 12345);
  XII_TEST_INT(session.m_Live.m_uiDroppedFrames, 4);
  XII_TEST_INT(session.m_Live.m_uiDroppedGpu, 5);
  XII_TEST_INT(session.m_Live.m_Gpu[0].m_uiQueue, 1);
  XII_TEST_FLOAT(session.m_Live.m_Gpu[0].m_fMs, 4.25, 0.0001);
  xiiMemoryStreamReader duplicate(&storage);
  XII_TEST_BOOL(!session.Receive(duplicate));
  XII_TEST_INT(session.m_Live.m_Frames.size(), 1);
  storage.Clear();
  xiiMemoryStreamWriter malformed(&storage);
  malformed << xiiUInt32(1) << xiiUInt64(91) << xiiUInt64(2) << double(30000) << xiiUInt64(0) << xiiUInt64(0) << xiiUInt64(0) << xiiUInt32(32769);
  xiiMemoryStreamReader badReader(&storage);
  XII_TEST_BOOL(!session.Receive(badReader));
  XII_TEST_INT(session.m_Live.m_Frames.size(), 1);
  session.BeginStream(92);
  XII_TEST_INT(session.m_uiSequence, 0);
  xiiMemoryStreamReader stale(&storage);
  XII_TEST_BOOL(!session.Receive(stale));
}

XII_CREATE_SIMPLE_TEST(Inspector, RollingPauseCaptureAndRoundTrip)
{
  Session session;
  for (int i = 1; i <= 1250; ++i)
    session.Append(Batch(i, i * 10000));
  XII_TEST_INT(session.m_Live.m_Frames.size(), 1200);
  XII_TEST_BOOL(session.m_Live.m_iBytes <= 64 * 1024 * 1024);
  session.SetPaused(true);
  const auto frozen = session.Display().m_Frames.back().m_uiIndex;
  session.Append(Batch(1251, 12510000));
  XII_TEST_BOOL(session.Display().m_Frames.back().m_uiIndex == frozen);
  session.SetPaused(false);
  XII_TEST_BOOL(session.Display().m_Frames.back().m_uiIndex == 1251);
  session.StartRecording(2, 100, 1024 * 1024);
  session.BeginStream(123); // Subscribing must never cancel a new recording.
  XII_TEST_BOOL(session.m_bRecording);
  session.Append(Batch(1252, 12520000));
  session.Append(Batch(1253, 12530000));
  XII_TEST_BOOL(session.m_bFinishing);
  auto late = Batch(1254, 12540000);
  late.m_Gpu.clear();
  late.m_Gpu.push_back({101, 1253, 1, 0, false, true, 1.5, "Delayed pass", 12530000});
  session.Append(late);
  XII_TEST_INT(session.m_Frozen.m_Frames.size(), 2);
  XII_TEST_INT(session.m_Frozen.m_Gpu.size(), 5);
  session.StopRecording(false);
  XII_TEST_BOOL(!session.m_bRecording && session.m_bPaused);
  XII_TEST_INT(session.Display().m_Frames.size(), 2);
  const auto encoded = QJsonDocument(session.Export()).toJson();
  Session    reopened;
  XII_TEST_BOOL(reopened.Import(QJsonDocument::fromJson(encoded).object()));
  XII_TEST_INT(reopened.Display().m_Frames.size(), 2);
  XII_TEST_INT(reopened.Display().m_Gpu.size(), 5);
  XII_TEST_BOOL(reopened.Display().m_Scopes[0].m_Lane == "77");
  XII_TEST_BOOL(reopened.Display().m_Scopes[0].m_uiId == session.Display().m_Scopes[0].m_uiId);
  XII_TEST_BOOL(!reopened.Display().m_Gpu[1].m_bAvailable);
  reopened.ResetConnection();
  XII_TEST_INT(reopened.Display().m_Frames.size(), 2);
  XII_TEST_INT(reopened.m_Live.m_Frames.size(), 0);
  const auto previousCapture = session.m_uiSelectedCapture;
  session.StartRecording(100, 100, 1024 * 1024);
  auto large = Batch(1256, 12560000);
  large.m_Scopes.clear();
  for (int i = 0; i < 8192; ++i)
    large.m_Scopes.push_back({QString(300, 'A'), "77", QString(300, 'B'), 12560000, 12568000, quint64(i + 1)});
  session.Append(large);
  XII_TEST_BOOL(!session.m_bRecording && session.Display().m_bTruncated);
  XII_TEST_BOOL(session.Display().m_iBytes <= 1024 * 1024);
  XII_TEST_BOOL(session.SelectCapture(previousCapture));
  XII_TEST_INT(session.Display().m_Frames.size(), 2);
  XII_TEST_INT(session.Display().m_Gpu.size(), 5);
  Session duration;
  duration.StartRecording(1000, 0.1, 1024 * 1024);
  for (int i = 1; i <= 15; ++i)
    duration.Append(Batch(i, i * 10000));
  XII_TEST_BOOL(duration.m_bFinishing);
  duration.StopRecording(false);
  XII_TEST_BOOL(!duration.m_bRecording && duration.m_bPaused);
  XII_TEST_BOOL(duration.Display().m_Frames.size() <= 10);
}

XII_CREATE_SIMPLE_TEST(Inspector, LiveDockIdentityAndControls)
{
  ads::CDockManager    manager;
  xiiQtProfilingWidget cpu(&manager);
  xiiQtProfilingWidget gpu(&manager, true, cpu.GetSession());
  auto                 session = cpu.GetSession();
  session->Append(Batch(1, 10000));
  session->m_Live.m_GpuScopes.push_back({"Timestamp scope", "GPU 0", {}, 100, 900, 1});
  cpu.Refresh();
  gpu.Refresh();
  auto* costs    = gpu.findChild<QTreeWidget*>("GPU pass costs");
  auto* hotspots = cpu.findChild<QTreeWidget*>("Profiling hotspots");
  XII_TEST_INT(costs->topLevelItemCount(), 2);
  XII_TEST_INT(hotspots->topLevelItemCount(), 2);
  XII_TEST_BOOL(costs->topLevelItem(0)->text(0) != costs->topLevelItem(1)->text(0));
  XII_TEST_BOOL(costs->topLevelItem(0)->data(0, Qt::UserRole) != costs->topLevelItem(1)->data(0, Qt::UserRole));
  auto*      selected = costs->topLevelItem(0);
  const auto identity = selected->data(0, Qt::UserRole);
  costs->setCurrentItem(selected);
  session->Append(Batch(2, 20000));
  gpu.Refresh();
  XII_TEST_BOOL(costs->currentItem() == selected);
  XII_TEST_BOOL(costs->currentItem()->data(0, Qt::UserRole) == identity);
  cpu.findChild<QPushButton*>("Pause profiling")->click();
  XII_TEST_BOOL(session->m_bPaused);
  const auto count = session->Display().m_Frames.size();
  session->Append(Batch(3, 30000));
  XII_TEST_INT(session->Display().m_Frames.size(), count);
  cpu.findChild<QPushButton*>("Pause profiling")->click();
  XII_TEST_BOOL(!session->m_bPaused);
  cpu.resize(1400, 850);
  gpu.resize(1400, 850);
  cpu.show();
  gpu.show();
  QApplication::processEvents();
  QImage cpuImage(cpu.size(), QImage::Format_ARGB32);
  cpu.render(&cpuImage);
  XII_TEST_BOOL(cpuImage.save("D:/XII/Workspace/inspector-live-cpu.png"));
  QImage gpuImage(gpu.size(), QImage::Format_ARGB32);
  gpu.render(&gpuImage);
  XII_TEST_BOOL(gpuImage.save("D:/XII/Workspace/inspector-live-gpu.png"));
}

XII_CREATE_SIMPLE_TEST(Inspector, GpuHistoryAndMetadataBudgets)
{
  Session       session;
  Session::Data batch;
  batch.m_fReceivedUs = 10000;
  const QString name(4096, 'G');
  for (int i = 0; i < 60000; ++i)
    batch.m_Gpu.push_back({quint64(i % 500), quint64(i), 0, 0, false, true, 1.0, name, 10000});
  for (quint64 graph = 0; graph < 500; ++graph)
    batch.m_Graphs[graph] = "Graph metadata";
  session.Append(batch);
  XII_TEST_BOOL(session.m_Live.m_iBytes <= 64 * 1024 * 1024);
  XII_TEST_BOOL(session.m_Live.m_Gpu.size() < 60000);
  XII_TEST_INT(session.m_Live.m_Graphs.size(), 256);
}

XII_CREATE_SIMPLE_TEST(Inspector, FoundationDataTransferRemainsAvailable)
{
  xiiDataTransfer transfer;
  XII_TEST_BOOL(!transfer.IsTransferRequested());
  transfer.EnableDataTransfer("Foundation transfer regression");
  transfer.RequestDataTransfer();
  XII_TEST_BOOL(transfer.IsTransferRequested(false));
  XII_TEST_BOOL(transfer.IsTransferRequested());
  XII_TEST_BOOL(!transfer.IsTransferRequested());
  {
    xiiDataTransferObject object(transfer, "Payload", "application/octet-stream", "bin");
    object.GetWriter() << xiiUInt64(9007199254740993ULL);
    object.Transmit();
  }
  transfer.DisableDataTransfer();
  transfer.RequestDataTransfer();
  XII_TEST_BOOL(!transfer.IsTransferRequested());
}
