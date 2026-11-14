/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Inspector/ProfilingSession.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>
#include <cmath>

namespace
{
  QString TraceId(const QJsonValue& value)
  {
    return value.isString() ? value.toString() : QString::number(value.toInteger());
  }
  qsizetype Cost(const xiiInspectorProfilingSession::Scope& scope)
  {
    return sizeof(scope) + 2 * (scope.m_Name.size() + scope.m_Lane.size() + scope.m_Function.size());
  }
  qsizetype Cost(const xiiInspectorProfilingSession::GpuSample& sample)
  {
    return sizeof(sample) + 2 * sample.m_Name.size();
  }
  bool ReadText(xiiStreamReader& reader, QString& text)
  {
    xiiUInt32 size = 0;
    if (reader.ReadDWordValue(&size).Failed() || size > 4096)
    {
      return false;
    }
    QByteArray bytes(size, Qt::Uninitialized);
    if (reader.ReadBytes(bytes.data(), size) != size)
    {
      return false;
    }
    text = QString::fromUtf8(bytes);
    return true;
  }
} // namespace

bool xiiInspectorProfilingSession::Receive(xiiStreamReader& reader)
{
  xiiUInt32 version = 0, count = 0;
  xiiUInt64 session = 0, sequence = 0;
  Data      batch;
  reader >> version >> session >> sequence >> batch.m_fReceivedUs >> batch.m_uiDroppedScopes >> batch.m_uiDroppedFrames >> batch.m_uiDroppedGpu;
  if (version != 1 || session != m_uiSession || sequence != m_uiSequence + 1 || !std::isfinite(batch.m_fReceivedUs))
  {
    m_ProtocolError = "Invalid live profiling batch (field 1). Resubscribing.";
    return false;
  }
  QVector<QString> strings = m_Strings;
  reader >> count;
  if (count > 32768 || count + strings.size() > 32768)
  {
    m_ProtocolError = "Invalid live profiling batch (field 2). Resubscribing.";
    return false;
  }
  for (xiiUInt32 i = 0; i < count; ++i)
  {
    QString text;
    if (!ReadText(reader, text))
    {
      m_ProtocolError = "Invalid live profiling batch (field 3). Resubscribing.";
      return false;
    }
    strings.push_back(text);
  }
  auto validName = [&strings](quint32 index) {
    return index < strings.size();
  };
  xiiUInt32 application = 0;
  reader >> application >> batch.m_uiProcessId;
  if (!validName(application))
  {
    return false;
  }
  batch.m_Application = strings.at(application);
  reader >> count;
  if (count > 1024)
  {
    m_ProtocolError = "Invalid live profiling batch (field 4). Resubscribing.";
    return false;
  }
  for (xiiUInt32 i = 0; i < count; ++i)
  {
    xiiUInt64 thread = 0;
    xiiUInt32 name   = 0;
    reader >> thread >> name;
    if (!validName(name))
    {
      m_ProtocolError = "Invalid live profiling batch (field 5). Resubscribing.";
      return false;
    }
    batch.m_Threads[QString::number(thread)] = strings.at(name);
  }
  reader >> count;
  if (count > 256)
  {
    m_ProtocolError = "Invalid live profiling batch (field 6). Resubscribing.";
    return false;
  }
  for (xiiUInt32 i = 0; i < count; ++i)
  {
    Frame frame;
    reader >> frame.m_uiIndex >> frame.m_fBeginUs >> frame.m_fEndUs;
    if (!std::isfinite(frame.m_fBeginUs) || !std::isfinite(frame.m_fEndUs) || frame.m_fBeginUs < 0 || frame.m_fEndUs < frame.m_fBeginUs)
    {
      m_ProtocolError = "Invalid live profiling batch (field 7). Resubscribing.";
      return false;
    }
    batch.m_Frames.push_back(frame);
  }
  reader >> count;
  if (count > 8192)
  {
    m_ProtocolError = "Invalid live profiling batch (field 8). Resubscribing.";
    return false;
  }
  for (xiiUInt32 i = 0; i < count; ++i)
  {
    Scope     scope;
    xiiUInt64 thread = 0;
    xiiUInt32 name = 0, function = 0;
    reader >> scope.m_uiId >> thread >> name >> function >> scope.m_fBeginUs >> scope.m_fEndUs;
    if (!validName(name) || !validName(function) || !std::isfinite(scope.m_fBeginUs) || !std::isfinite(scope.m_fEndUs) || scope.m_fBeginUs < 0 || scope.m_fEndUs < scope.m_fBeginUs)
    {
      m_ProtocolError = "Invalid live profiling batch (field 9). Resubscribing.";
      return false;
    }
    scope.m_Lane     = QString::number(thread);
    scope.m_Name     = strings.at(name).isEmpty() ? "Unnamed scope" : strings.at(name);
    scope.m_Function = strings.at(function);
    batch.m_Scopes.push_back(scope);
  }
  reader >> count;
  if (count > 2048)
  {
    m_ProtocolError = "Invalid live profiling batch (field 10). Resubscribing.";
    return false;
  }
  for (xiiUInt32 i = 0; i < count; ++i)
  {
    Scope     scope;
    xiiUInt32 gpu = 0, name = 0;
    reader >> gpu >> name >> scope.m_fBeginUs >> scope.m_fEndUs;
    if (!validName(name) || !std::isfinite(scope.m_fBeginUs) || !std::isfinite(scope.m_fEndUs) || scope.m_fEndUs < scope.m_fBeginUs)
    {
      m_ProtocolError = "Invalid live profiling batch (field 11). Resubscribing.";
      return false;
    }
    scope.m_Lane = QString("GPU %1").arg(gpu);
    scope.m_Name = strings.at(name);
    batch.m_GpuScopes.push_back(scope);
  }
  reader >> count;
  if (count > 64)
  {
    m_ProtocolError = "Invalid live profiling batch (field 12). Resubscribing.";
    return false;
  }
  for (xiiUInt32 i = 0; i < count; ++i)
  {
    xiiUInt64 graph = 0;
    xiiUInt32 name = 0, view = 0;
    reader >> graph >> name >> view;
    if (!validName(name) || !validName(view))
    {
      m_ProtocolError = "Invalid live profiling batch (field 13). Resubscribing.";
      return false;
    }
    batch.m_Graphs[graph] = (strings.at(view).isEmpty() ? QString{} : strings.at(view) + " / ") + (strings.at(name).isEmpty() ? "Render graph" : strings.at(name));
  }
  reader >> count;
  if (count > 4096)
  {
    m_ProtocolError = "Invalid live profiling batch (field 14). Resubscribing.";
    return false;
  }
  for (xiiUInt32 i = 0; i < count; ++i)
  {
    GpuSample sample;
    xiiUInt32 name = 0;
    reader >> sample.m_uiGraph >> sample.m_uiFrame >> sample.m_uiPass >> sample.m_uiQueue >> sample.m_bSubmission >> sample.m_bAvailable >> sample.m_fMs >> sample.m_fRecordedUs >> name;
    if (!validName(name) || sample.m_uiQueue > 2 || !std::isfinite(sample.m_fMs) || sample.m_fMs < 0 || !std::isfinite(sample.m_fRecordedUs))
    {
      m_ProtocolError = "Invalid live profiling batch (field 15). Resubscribing.";
      return false;
    }
    sample.m_Name = strings.at(name);
    if (sample.m_Name.isEmpty())
    {
      sample.m_Name = sample.m_bSubmission ? QString("Submission %1").arg(sample.m_uiPass + 1) : QString("Pass %1").arg(sample.m_uiPass + 1);
    }
    batch.m_Gpu.push_back(sample);
  }
  xiiUInt32 footer = 0;
  reader >> footer;
  if (footer != 0x50535452)
  {
    m_ProtocolError = "Invalid live profiling batch (field 16). Resubscribing.";
    return false;
  }
  m_ProtocolError.clear();
  m_Strings    = std::move(strings);
  m_uiSequence = sequence;
  Append(batch);
  return true;
}

void xiiInspectorProfilingSession::BeginStream(quint64 session)
{
  m_Strings.clear();
  m_uiSequence  = 0;
  m_uiSession   = session;
  m_bSubscribed = true;
}

void xiiInspectorProfilingSession::ResetConnection()
{
  m_bSnapshotPending = false;
  // A recording ends on disconnect, but its owned capture remains inspectable.
  if (m_bRecording)
  {
    StopRecording(false);
  }
  m_Live = {};
  m_Strings.clear();
  m_uiSequence  = 0;
  m_bSubscribed = false;
  ++m_uiRevision;
}

void xiiInspectorProfilingSession::SetPaused(bool paused)
{
  if (paused && !m_bPaused)
  {
    m_Frozen = m_Live;
  }
  m_bPaused = paused;
  if (!paused)
  {
    m_uiSelectedCapture = 0;
  }
  ++m_uiRevision;
}

void xiiInspectorProfilingSession::StartRecording(int frames, double seconds, qsizetype bytes)
{
  m_Frozen            = {};
  m_uiSelectedCapture = 0;
  qsizetype retained  = 0;
  for (const auto& capture : m_Captures)
  {
    retained += capture.m_iBytes;
  }
  while (!m_Captures.isEmpty() && retained + bytes > 256 * 1024 * 1024)
  {
    retained -= m_Captures.first().m_iBytes;
    m_Captures.erase(m_Captures.begin());
  }
  m_Frozen.m_Threads     = m_Live.m_Threads;
  m_Frozen.m_Graphs      = m_Live.m_Graphs;
  m_Frozen.m_Application = m_Live.m_Application;
  m_Frozen.m_uiProcessId = m_Live.m_uiProcessId;
  m_iFrameLimit          = qBound(1, frames, 36000);
  m_fSecondsLimit        = qBound(0.1, seconds, 600.0);
  m_iByteLimit           = qBound(qsizetype(1024 * 1024), bytes, qsizetype(256 * 1024 * 1024));
  m_fRecordStartUs       = 0;
  for (auto it = m_Frozen.m_Threads.begin(); it != m_Frozen.m_Threads.end(); ++it)
  {
    m_Frozen.m_iBytes += 128 + 2 * (it.key().size() + it.value().size());
  }
  for (const auto& name : m_Frozen.m_Graphs)
  {
    m_Frozen.m_iBytes += 128 + 2 * name.size();
  }
  while (m_Frozen.m_iBytes > m_iByteLimit && !m_Frozen.m_Graphs.isEmpty())
  {
    m_Frozen.m_iBytes -= 128 + 2 * m_Frozen.m_Graphs.last().size();
    m_Frozen.m_Graphs.erase(std::prev(m_Frozen.m_Graphs.end()));
    m_Frozen.m_bTruncated = true;
  }
  while (m_Frozen.m_iBytes > m_iByteLimit && !m_Frozen.m_Threads.isEmpty())
  {
    const auto it = std::prev(m_Frozen.m_Threads.end());
    m_Frozen.m_iBytes -= 128 + 2 * (it.key().size() + it.value().size());
    m_Frozen.m_Threads.erase(it);
    m_Frozen.m_bTruncated = true;
  }
  m_bRecording   = true;
  m_bFinishing   = false;
  m_fRecordEndUs = 0;
  m_bPaused      = false;
  ++m_uiRevision;
}

void xiiInspectorProfilingSession::StopRecording(bool drainGpu)
{
  if (drainGpu && m_bRecording && !m_bFinishing && !m_Frozen.m_bTruncated)
  {
    m_bFinishing   = true;
    m_fRecordEndUs = m_Frozen.m_Frames.isEmpty() ? m_Live.m_fReceivedUs : m_Frozen.m_Frames.back().m_fEndUs;
    m_CaptureDrainTimer.restart();
    ++m_uiRevision;
    return;
  }
  const bool wasRecording = m_bRecording;
  m_bFinishing            = false;
  m_bRecording            = false;
  m_bPaused               = true;
  if (wasRecording)
  {
    ArchiveCapture();
  }
  ++m_uiRevision;
}

void xiiInspectorProfilingSession::ArchiveCapture()
{
  m_uiSelectedCapture             = m_uiNextCapture++;
  m_Captures[m_uiSelectedCapture] = m_Frozen;
  qsizetype bytes                 = 0;
  for (const auto& capture : m_Captures)
  {
    bytes += capture.m_iBytes;
  }
  while (m_Captures.size() > 8 || (bytes > 256 * 1024 * 1024 && m_Captures.size() > 1))
  {
    bytes -= m_Captures.first().m_iBytes;
    m_Captures.erase(m_Captures.begin());
  }
}

bool xiiInspectorProfilingSession::SelectCapture(quint64 id)
{
  if (!m_Captures.contains(id))
  {
    return false;
  }
  if (m_bRecording)
  {
    StopRecording(false);
  }
  m_Frozen            = m_Captures[id];
  m_uiSelectedCapture = id;
  m_bPaused           = true;
  ++m_uiRevision;
  return true;
}

void xiiInspectorProfilingSession::UpdateRecording()
{
  if (m_bFinishing && m_CaptureDrainTimer.elapsed() >= 500)
  {
    StopRecording(false);
  }
}

void xiiInspectorProfilingSession::Append(const Data& batch)
{
  auto append = [](Data& target, const Data& batch, qsizetype limit, bool recording) {
    target.m_uiDroppedScopes = batch.m_uiDroppedScopes;
    target.m_uiDroppedFrames = batch.m_uiDroppedFrames;
    target.m_uiDroppedGpu    = batch.m_uiDroppedGpu;
    target.m_fReceivedUs     = batch.m_fReceivedUs;
    if (!batch.m_Application.isEmpty())
    {
      target.m_Application = batch.m_Application;
      target.m_uiProcessId = batch.m_uiProcessId;
    }
    auto room = [&target, limit, recording](qsizetype cost) {
      if (recording && target.m_iBytes + cost > limit)
      {
        target.m_bTruncated = true;
        return false;
      }
      target.m_iBytes += cost;
      return true;
    };
    for (auto it = batch.m_Threads.begin(); it != batch.m_Threads.end(); ++it)
    {
      const bool      exists   = target.m_Threads.contains(it.key());
      const qsizetype bytes    = 128 + 2 * (it.key().size() + it.value().size());
      const qsizetype previous = exists ? 128 + 2 * (it.key().size() + target.m_Threads[it.key()].size()) : 0;
      if (!exists && target.m_Threads.size() >= 1024)
      {
        continue; // Scope identities remain available even without additional thread-name metadata.
      }
      if (room(bytes - previous))
      {
        target.m_Threads[it.key()] = it.value();
      }
    }
    for (auto it = batch.m_Graphs.begin(); it != batch.m_Graphs.end(); ++it)
    {
      const bool      exists   = target.m_Graphs.contains(it.key());
      const qsizetype bytes    = 128 + 2 * it.value().size();
      const qsizetype previous = exists ? 128 + 2 * target.m_Graphs[it.key()].size() : 0;
      if (!exists && target.m_Graphs.size() >= (recording ? 4096 : 256))
      {
        if (recording)
        {
          target.m_bTruncated = true;
          continue;
        }
        target.m_iBytes -= 128 + 2 * target.m_Graphs.first().size();
        target.m_Graphs.erase(target.m_Graphs.begin());
      }
      if (room(bytes - previous))
      {
        target.m_Graphs[it.key()] = it.value();
      }
    }
    for (const auto& scope : batch.m_Scopes)
    {
      if (!target.m_Threads.contains(scope.m_Lane) && target.m_Threads.size() < 1024)
      {
        const auto name = QString("Unnamed thread %1").arg(target.m_Threads.size() + 1);
        if (room(128 + 2 * (scope.m_Lane.size() + name.size())))
        {
          target.m_Threads[scope.m_Lane] = name;
        }
      }
    }
    for (const auto& frame : batch.m_Frames)
    {
      if (room(sizeof(frame)))
      {
        target.m_Frames.push_back(frame);
      }
    }
    for (const auto& scope : batch.m_Scopes)
    {
      if ((!recording || target.m_Scopes.size() < 500000) && room(Cost(scope)))
      {
        target.m_Scopes.push_back(scope);
      }
    }
    for (const auto& scope : batch.m_GpuScopes)
    {
      if ((!recording || target.m_GpuScopes.size() < 100000) && room(Cost(scope)))
      {
        target.m_GpuScopes.push_back(scope);
      }
    }
    for (const auto& sample : batch.m_Gpu)
    {
      if ((!recording || target.m_Gpu.size() < 500000) && room(Cost(sample)))
      {
        target.m_Gpu.push_back(sample);
      }
    }
  };
  append(m_Live, batch, 64 * 1024 * 1024, false);
  while (m_Live.m_Frames.size() > 1200)
  {
    m_Live.m_Frames.removeFirst();
    m_Live.m_iBytes -= sizeof(Frame);
  }
  const double cutoff = m_Live.m_Frames.isEmpty() ? batch.m_fReceivedUs - 20000000 : m_Live.m_Frames.front().m_fBeginUs;
  // Completion order is not begin-time order. Long scopes may arrive late.
  m_Live.m_Scopes.erase(std::remove_if(m_Live.m_Scopes.begin(), m_Live.m_Scopes.end(), [&](const Scope& scope) {
                          if (scope.m_fEndUs >= cutoff)
                          {
                            return false;
                          }
                          m_Live.m_iBytes -= Cost(scope);
                          return true;
                        }),
                        m_Live.m_Scopes.end());
  qsizetype remove = 0;
  while (remove < m_Live.m_Scopes.size() && (m_Live.m_Scopes.size() - remove > 100000 || m_Live.m_iBytes > 64 * 1024 * 1024))
  {
    m_Live.m_iBytes -= Cost(m_Live.m_Scopes[remove++]);
  }
  m_Live.m_Scopes.remove(0, remove);
  if (m_Live.m_GpuScopes.size() > 60000)
  {
    const auto remove = m_Live.m_GpuScopes.size() - 60000;
    for (qsizetype i = 0; i < remove; ++i)
    {
      m_Live.m_iBytes -= Cost(m_Live.m_GpuScopes[i]);
    }
    m_Live.m_GpuScopes.remove(0, remove);
  }
  if (m_Live.m_Gpu.size() > 60000)
  {
    const auto remove = m_Live.m_Gpu.size() - 60000;
    for (qsizetype i = 0; i < remove; ++i)
    {
      m_Live.m_iBytes -= Cost(m_Live.m_Gpu[i]);
    }
    m_Live.m_Gpu.remove(0, remove);
  }
  // Apply the byte budget even when GPU histories dominate the session.
  qsizetype gpuRemove = 0;
  while (m_Live.m_iBytes > 64 * 1024 * 1024 && gpuRemove < m_Live.m_Gpu.size())
  {
    m_Live.m_iBytes -= Cost(m_Live.m_Gpu[gpuRemove++]);
  }
  m_Live.m_Gpu.remove(0, gpuRemove);
  qsizetype scopeRemove = 0;
  while (m_Live.m_iBytes > 64 * 1024 * 1024 && scopeRemove < m_Live.m_GpuScopes.size())
  {
    m_Live.m_iBytes -= Cost(m_Live.m_GpuScopes[scopeRemove++]);
  }
  m_Live.m_GpuScopes.remove(0, scopeRemove);
  if (m_bRecording)
  {
    if (m_fRecordStartUs == 0)
    {
      m_fRecordStartUs = batch.m_Frames.isEmpty() ? batch.m_fReceivedUs : batch.m_Frames.front().m_fBeginUs;
    }
    Data   selected = batch;
    double end      = m_bFinishing ? m_fRecordEndUs : m_fRecordStartUs + m_fSecondsLimit * 1000000;
    selected.m_Frames.erase(std::remove_if(selected.m_Frames.begin(), selected.m_Frames.end(), [&](const Frame& frame) {
                              return frame.m_fBeginUs < m_fRecordStartUs || frame.m_fBeginUs >= end;
                            }),
                            selected.m_Frames.end());
    const int room = qMax(0, m_iFrameLimit - int(m_Frozen.m_Frames.size()));
    if (selected.m_Frames.size() > room)
    {
      selected.m_Frames.resize(room);
    }
    if (!selected.m_Frames.isEmpty() && selected.m_Frames.size() == room)
    {
      end = qMin(end, selected.m_Frames.back().m_fEndUs);
    }
    selected.m_Scopes.erase(std::remove_if(selected.m_Scopes.begin(), selected.m_Scopes.end(), [&](const Scope& scope) {
                              return scope.m_fEndUs <= m_fRecordStartUs || scope.m_fBeginUs >= end;
                            }),
                            selected.m_Scopes.end());
    selected.m_Gpu.erase(std::remove_if(selected.m_Gpu.begin(), selected.m_Gpu.end(), [&](const GpuSample& sample) {
                           return sample.m_fRecordedUs < m_fRecordStartUs || sample.m_fRecordedUs >= end;
                         }),
                         selected.m_Gpu.end());
    append(m_Frozen, selected, m_iByteLimit, true);
    if (m_Frozen.m_Scopes.size() >= 500000 || m_Frozen.m_Gpu.size() >= 500000 || m_Frozen.m_GpuScopes.size() >= 100000)
    {
      m_Frozen.m_bTruncated = true;
    }
    if (m_Frozen.m_bTruncated)
    {
      StopRecording(false);
    }
    else if (!m_bFinishing && (m_Frozen.m_Frames.size() >= m_iFrameLimit || batch.m_fReceivedUs >= end))
    {
      StopRecording();
    }
  }
  ++m_uiRevision;
}

bool xiiInspectorProfilingSession::Decode(const QJsonObject& object, QVector<Scope>& scopes, QMap<QString, QString>& lanes)
{
  if (!object["traceEvents"].isArray() || object["traceEvents"].toArray().size() > 1000000)
  {
    return false;
  }
  QVector<Scope>                                decoded;
  QMap<QString, QString>                        names;
  QMap<QPair<QString, QString>, QVector<Scope>> pending;
  for (const auto& value : object["traceEvents"].toArray())
  {
    if (!value.isObject())
    {
      return false;
    }
    const auto    event = value.toObject();
    const QString phase = event["ph"].toString(), lane = TraceId(event["pid"]) + ":" + TraceId(event["tid"]), name = event["name"].toString();
    if (phase == "M" && name == "thread_name")
    {
      names[lane] = event["args"].toObject()["name"].toString();
    }
    if (phase != "B" && phase != "E" && phase != "X")
    {
      continue;
    }
    const double begin = event["ts"].toDouble(-1);
    if (!std::isfinite(begin) || begin < 0)
    {
      return false;
    }
    if (phase == "B")
    {
      pending[{lane, name}].push_back({name, lane, event["args"].toObject()["function"].toString(), begin, 0});
    }
    else if (phase == "E")
    {
      auto& stack = pending[{lane, name}];
      if (stack.isEmpty())
      {
        continue;
      }
      Scope scope = stack.takeLast();
      if (begin < scope.m_fBeginUs)
      {
        return false;
      }
      scope.m_fEndUs = begin;
      decoded.push_back(scope);
    }
    else
    {
      const double duration = event["dur"].toDouble(-1);
      if (!std::isfinite(duration) || duration < 0 || duration > 1e12)
      {
        return false;
      }
      decoded.push_back({name, lane, event["args"].toObject()["function"].toString(), begin, begin + duration, event["args"].toObject()["xiiScopeId"].toString().toULongLong()});
    }
  }
  std::stable_sort(decoded.begin(), decoded.end(), [](const Scope& a, const Scope& b) {
    if (a.m_Lane != b.m_Lane)
    {
      return a.m_Lane < b.m_Lane;
    }
    return a.m_fBeginUs == b.m_fBeginUs ? a.m_fEndUs > b.m_fEndUs : a.m_fBeginUs < b.m_fBeginUs;
  });
  quint64 id = 0;
  for (auto& scope : decoded)
  {
    if (scope.m_uiId == 0)
    {
      scope.m_uiId = ++id;
    }
    if (!names.contains(scope.m_Lane))
    {
      names[scope.m_Lane] = QString("Unnamed thread %1").arg(names.size() + 1);
    }
  }
  scopes = std::move(decoded);
  lanes  = std::move(names);
  return true;
}

QJsonObject xiiInspectorProfilingSession::Export() const
{
  const auto& data = Display();
  QJsonArray  events;
  for (auto it = data.m_Threads.begin(); it != data.m_Threads.end(); ++it)
  {
    events.append(QJsonObject{{"ph", "M"}, {"pid", 1}, {"tid", it.key()}, {"name", "thread_name"}, {"args", QJsonObject{{"name", it.value()}}}});
  }
  events.append(QJsonObject{{"ph", "M"}, {"pid", 1}, {"tid", "frames"}, {"name", "thread_name"}, {"args", QJsonObject{{"name", "Frames"}}}});
  for (const auto& scope : data.m_Scopes)
  {
    events.append(QJsonObject{{"ph", "X"}, {"pid", 1}, {"tid", scope.m_Lane}, {"name", scope.m_Name}, {"ts", scope.m_fBeginUs}, {"dur", scope.m_fEndUs - scope.m_fBeginUs}, {"args", QJsonObject{{"function", scope.m_Function}, {"xiiScopeId", QString::number(scope.m_uiId)}}}});
  }
  QSet<QString> gpuLanes;
  for (const auto& scope : data.m_GpuScopes)
  {
    if (!gpuLanes.contains(scope.m_Lane))
    {
      events.append(QJsonObject{{"ph", "M"}, {"pid", 2}, {"tid", scope.m_Lane}, {"name", "thread_name"}, {"args", QJsonObject{{"name", scope.m_Lane}}}});
      gpuLanes.insert(scope.m_Lane);
    }
    events.append(QJsonObject{{"ph", "X"}, {"pid", 2}, {"tid", scope.m_Lane}, {"name", scope.m_Name}, {"ts", scope.m_fBeginUs}, {"dur", scope.m_fEndUs - scope.m_fBeginUs}});
  }
  QJsonArray frames;
  for (const auto& frame : data.m_Frames)
  {
    frames.append(QJsonObject{{"id", QString::number(frame.m_uiIndex)}, {"beginUs", frame.m_fBeginUs}, {"endUs", frame.m_fEndUs}});
    events.append(QJsonObject{{"ph", "X"}, {"pid", 1}, {"tid", "frames"}, {"name", QString("Frame %1").arg(frame.m_uiIndex)}, {"ts", frame.m_fBeginUs}, {"dur", frame.m_fEndUs - frame.m_fBeginUs}});
  }
  QJsonArray gpu;
  for (const auto& sample : data.m_Gpu)
  {
    gpu.append(QJsonObject{{"graph", QString::number(sample.m_uiGraph)}, {"frame", QString::number(sample.m_uiFrame)}, {"pass", int(sample.m_uiPass)}, {"queue", int(sample.m_uiQueue)}, {"submission", sample.m_bSubmission}, {"available", sample.m_bAvailable}, {"ms", sample.m_fMs}, {"recordedUs", sample.m_fRecordedUs}, {"name", sample.m_Name}});
  }
  QJsonObject graphs;
  for (auto it = data.m_Graphs.begin(); it != data.m_Graphs.end(); ++it)
  {
    graphs[QString::number(it.key())] = it.value();
  }
  return {{"traceEvents", events}, {"xiiProfiler", QJsonObject{{"version", 2}, {"frames", frames}, {"gpu", gpu}, {"graphs", graphs}, {"application", data.m_Application}, {"processId", QString::number(data.m_uiProcessId)}, {"truncated", data.m_bTruncated}, {"droppedScopes", QString::number(data.m_uiDroppedScopes)}, {"droppedFrames", QString::number(data.m_uiDroppedFrames)}, {"droppedGpu", QString::number(data.m_uiDroppedGpu)}}}};
}

bool xiiInspectorProfilingSession::Import(const QJsonObject& object)
{
  Data data;
  if (!Decode(object, data.m_Scopes, data.m_Threads))
  {
    return false;
  }
  for (const auto& value : object["traceEvents"].toArray())
  {
    const auto event = value.toObject();
    if (event["ph"] == "M" && event["name"] == "process_name")
    {
      data.m_Application = event["args"].toObject()["name"].toString();
      data.m_uiProcessId = TraceId(event["pid"]).toULongLong();
      break;
    }
  }
  const auto extension = object["xiiProfiler"].toObject();
  if (!extension.isEmpty())
  {
    QMap<QString, QString> threads;
    for (auto it = data.m_Threads.begin(); it != data.m_Threads.end(); ++it)
    {
      threads[it.key().mid(2)] = it.value();
    }
    data.m_Threads = threads;
    for (auto& scope : data.m_Scopes)
    {
      scope.m_Lane = scope.m_Lane.mid(2);
    }
    if (extension["version"].toInt() != 2 || !extension["gpu"].isArray() || extension["gpu"].toArray().size() > 1000000 || extension["frames"].toArray().size() > 36000)
    {
      return false;
    }
    for (const auto& value : extension["frames"].toArray())
    {
      const auto    frame = value.toObject();
      bool          valid = false;
      const quint64 id    = frame["id"].toString().toULongLong(&valid);
      const double  begin = frame["beginUs"].toDouble(-1), end = frame["endUs"].toDouble(-1);
      if (!valid || !std::isfinite(begin) || !std::isfinite(end) || begin < 0 || end < begin)
      {
        return false;
      }
      data.m_Frames.push_back({id, begin, end});
    }
    for (const auto& value : extension["gpu"].toArray())
    {
      const auto   sample     = value.toObject();
      bool         graphValid = false, frameValid = false;
      const auto   graph = sample["graph"].toString().toULongLong(&graphValid), frame = sample["frame"].toString().toULongLong(&frameValid);
      const double ms = sample["ms"].toDouble(-1);
      if (!graphValid || !frameValid || sample["pass"].toInt(-1) < 0 || sample["queue"].toInt(-1) < 0 || sample["queue"].toInt() > 2 || !std::isfinite(ms) || ms < 0)
      {
        return false;
      }
      data.m_Gpu.push_back({graph, frame, quint32(sample["pass"].toInt()), quint32(sample["queue"].toInt()), sample["submission"].toBool(), sample["available"].toBool(), ms, sample["name"].toString(), sample["recordedUs"].toDouble()});
    }
    const auto graphs = extension["graphs"].toObject();
    for (auto it = graphs.begin(); it != graphs.end(); ++it)
    {
      bool       valid = false;
      const auto id    = it.key().toULongLong(&valid);
      if (!valid || !it.value().isString() || graphs.size() > 4096)
      {
        return false;
      }
      data.m_Graphs[id] = it.value().toString();
    }
    data.m_bTruncated      = extension["truncated"].toBool();
    data.m_Application     = extension["application"].toString();
    data.m_uiProcessId     = extension["processId"].toString().toULongLong();
    data.m_uiDroppedScopes = extension["droppedScopes"].toString().toULongLong();
    data.m_uiDroppedFrames = extension["droppedFrames"].toString().toULongLong();
    data.m_uiDroppedGpu    = extension["droppedGpu"].toString().toULongLong();
  }
  else
  {
    // Original Chrome traces remain readable. Their GPU clock lanes remain separate from CPU analysis.
    for (const auto& scope : data.m_Scopes)
    {
      if (data.m_Threads.value(scope.m_Lane) == "Frames" && scope.m_Name.startsWith("Frame "))
      {
        data.m_Frames.push_back({scope.m_Name.mid(6).toULongLong(), scope.m_fBeginUs, scope.m_fEndUs});
      }
    }
    for (const auto& value : object["xiiGpuPassCosts"].toArray())
    {
      const auto graph  = value.toObject();
      const auto id     = graph["instance"].toString(graph["id"].toString()).toULongLong();
      data.m_Graphs[id] = graph["name"].toString();
      for (const auto& value : graph["passes"].toArray())
      {
        const auto pass = value.toObject();
        data.m_Gpu.push_back({id, pass["timingFrame"].toString().toULongLong(), quint32(pass["index"].toInt()), quint32(pass["queue"].toInt()), false, pass["timingAvailable"].toBool(), pass["gpuMs"].toDouble(), pass["name"].toString()});
      }
    }
  }
  // Frame markers are not application work. GPU clock domains cannot be compared with CPU timestamps.
  data.m_Scopes.erase(std::remove_if(data.m_Scopes.begin(), data.m_Scopes.end(), [&](const Scope& scope) {
                        const auto lane = data.m_Threads.value(scope.m_Lane);
                        if (lane.startsWith("GPU"))
                        {
                          data.m_GpuScopes.push_back(scope);
                        }
                        return lane == "Frames" || lane.startsWith("GPU");
                      }),
                      data.m_Scopes.end());
  for (const auto& scope : data.m_Scopes)
  {
    data.m_iBytes += Cost(scope);
  }
  for (const auto& sample : data.m_Gpu)
  {
    data.m_iBytes += Cost(sample);
  }
  for (const auto& scope : data.m_GpuScopes)
  {
    data.m_iBytes += Cost(scope);
  }
  data.m_iBytes += data.m_Frames.size() * sizeof(Frame);
  for (auto it = data.m_Threads.begin(); it != data.m_Threads.end(); ++it)
  {
    data.m_iBytes += 128 + 2 * (it.key().size() + it.value().size());
  }
  for (const auto& name : data.m_Graphs)
  {
    data.m_iBytes += 128 + 2 * name.size();
  }
  if (data.m_iBytes > 256 * 1024 * 1024)
  {
    return false;
  }
  if (m_bRecording)
  {
    StopRecording(false);
  }
  m_Frozen = std::move(data);
  ArchiveCapture();
  m_bPaused    = true;
  m_bRecording = false;
  ++m_uiRevision;
  return true;
}
