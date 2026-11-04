/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QMap>
#include <QVector>

class xiiStreamReader;

/// GUI-thread owned session. Reception only appends bounded data; widgets paint on their own timers.
class xiiInspectorProfilingSession
{
public:
  struct Scope
  {
    QString m_Name, m_Lane, m_Function;
    double  m_fBeginUs = 0, m_fEndUs = 0;
    quint64 m_uiId = 0;
  };
  struct Frame
  {
    quint64 m_uiIndex  = 0;
    double  m_fBeginUs = 0, m_fEndUs = 0;
  };
  struct GpuSample
  {
    quint64 m_uiGraph = 0, m_uiFrame = 0;
    quint32 m_uiPass = 0, m_uiQueue = 0;
    bool    m_bSubmission = false, m_bAvailable = false;
    double  m_fMs = 0;
    QString m_Name;
    double  m_fRecordedUs = 0;
  };
  struct Data
  {
    QVector<Scope>         m_Scopes;
    QVector<Scope>         m_GpuScopes;
    QVector<Frame>         m_Frames;
    QVector<GpuSample>     m_Gpu;
    QMap<QString, QString> m_Threads;
    QMap<quint64, QString> m_Graphs;
    quint64                m_uiDroppedScopes = 0, m_uiDroppedFrames = 0, m_uiDroppedGpu = 0;
    double                 m_fReceivedUs = 0;
    qsizetype              m_iBytes      = 0;
    bool                   m_bTruncated  = false;
    QString                m_Application;
    quint64                m_uiProcessId = 0;
  };

  QString     m_ProtocolError;
  bool        Receive(xiiStreamReader& reader);
  void        ResetConnection();
  void        BeginStream(quint64 session);
  void        SetPaused(bool paused);
  void        StartRecording(int frames, double seconds, qsizetype bytes);
  void        StopRecording(bool drainGpu = true);
  void        UpdateRecording();
  bool        SelectCapture(quint64 id);
  void        Append(const Data& batch);
  const Data& Display() const { return m_bPaused ? m_Frozen : m_Live; }
  QJsonObject Export() const;
  bool        Import(const QJsonObject& object);
  static bool Decode(const QJsonObject& object, QVector<Scope>& scopes, QMap<QString, QString>& lanes);

  Data                m_Live, m_Frozen;
  QMap<quint64, Data> m_Captures;
  quint64             m_uiSelectedCapture = 0;
  bool                m_bPaused = false, m_bRecording = false;
  int                 m_iFrameLimit   = 600;
  double              m_fSecondsLimit = 10, m_fRecordStartUs = 0;
  qsizetype           m_iByteLimit = 64 * 1024 * 1024;
  quint64             m_uiRevision = 0, m_uiSequence = 0, m_uiSession = 0;
  bool                m_bSubscribed  = false;
  bool                m_bFinishing   = false;
  double              m_fRecordEndUs = 0;
  QElapsedTimer       m_CaptureDrainTimer;

  bool          m_bSnapshotPending = false;
  QElapsedTimer m_SnapshotTimer;

private:
  void             ArchiveCapture();
  quint64          m_uiNextCapture = 1;
  QVector<QString> m_Strings;
};
