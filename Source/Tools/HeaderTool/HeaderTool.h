
#include <Foundation/Application/Application.h>

/// \brief Build tool that exports the Engine Source Header Files into a specified directory.
class xiiHeaderTool : public xiiApplication
{
public:
  typedef xiiApplication SUPER;

  xiiHeaderTool();

  virtual void AfterCoreSystemsStartup() override;

  virtual void BeforeCoreSystemsShutdown() override;

  virtual xiiApplication::Execution Run() override;

private:
  xiiString m_sSourceDirectory;
  xiiString m_sExportDirectory;

  bool m_bErrorEncountered;
};
