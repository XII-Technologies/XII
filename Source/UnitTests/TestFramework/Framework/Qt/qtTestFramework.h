/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#ifdef XII_USE_QT

#  include <QObject>
#  include <TestFramework/Framework/TestFramework.h>
#  include <TestFramework/TestFrameworkDLL.h>

/// Derived xiiTestFramework which signals the GUI to update whenever a new tests result comes in.
class XII_TEST_DLL xiiQtTestFramework : public QObject, public xiiTestFramework
{
  Q_OBJECT
public:
  xiiQtTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int iArgc, const char** pArgv);
  virtual ~xiiQtTestFramework();

private:
  xiiQtTestFramework(xiiQtTestFramework&);
  void operator=(xiiQtTestFramework&);

Q_SIGNALS:
  void TestResultReceived(qint32 testIndex, qint32 subTestIndex);

protected:
  virtual void OutputImpl(xiiTestOutput::Enum Type, const char* szMsg) override;
  virtual void TestResultImpl(xiiUInt32 uiSubTestIndex, bool bSuccess, double fDuration) override;
  virtual void SetSubTestStatusImpl(xiiUInt32 uiSubTestIndex, const char* szStatus) override;
};

#endif
