#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <SampleGamePlugin/Interface/SingletonInterface.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>

// BEGIN-DOCS-CODE-SNIPPET: singleton-impl-definition
XII_IMPLEMENT_SINGLETON(PrintImplementation);

PrintImplementation::PrintImplementation() :
  m_SingletonRegistrar(this) // needed for automatic registration
{
}
// END-DOCS-CODE-SNIPPET

void PrintImplementation::Print(const xiiFormatString& text)
{
  xiiStringBuilder tmp;
  const char*      szFormattedText = text.GetText(tmp);

  xiiLog::Info(szFormattedText);
}
