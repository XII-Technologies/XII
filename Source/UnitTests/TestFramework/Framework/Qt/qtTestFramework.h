#pragma once

#ifdef XII_USE_QT

#  include <QObject>
#  include <TestFramework/Framework/TestFramework.h>
#  include <TestFramework/TestFrameworkDLL.h>

/// \brief Derived xiiTestFramework which signals the GUI to update whenever a new tests result comes in.
class XII_TEST_DLL xiiQtTestFramework : public QObject, public xiiTestFramework
{
  Q_OBJECT
public:
  xiiQtTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int argc, const char** argv);
  virtual ~xiiQtTestFramework();

private:
  xiiQtTestFramework(xiiQtTestFramework&);
  void operator=(xiiQtTestFramework&);

Q_SIGNALS:
  void TestResultReceived(qint32 iTestIndex, qint32 iSubTestIndex);

protected:
  virtual void OutputImpl(xiiTestOutput::Enum Type, const char* szMsg) override;
  virtual void TestResultImpl(xiiInt32 iSubTestIndex, bool bSuccess, double fDuration) override;
};

#endif
