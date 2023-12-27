#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppSettings.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

xiiResult xiiCppSettings::Save(xiiStringView sFile)
{
  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(sFile));

  xiiOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  ddl.BeginObject("Target", "Default");

  xiiOpenDdlUtils::StoreString(ddl, m_sPluginName, "PluginName");

  switch (m_Compiler)
  {
    case Compiler::None:
      xiiOpenDdlUtils::StoreString(ddl, "", "Compiler");
      break;
    case Compiler::Vs2022:
      xiiOpenDdlUtils::StoreString(ddl, "Vs2022", "Compiler");
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  ddl.EndObject();

  return XII_SUCCESS;
}

xiiResult xiiCppSettings::Load(xiiStringView sFile)
{
  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(sFile));

  xiiOpenDdlReader ddl;
  XII_SUCCEED_OR_RETURN(ddl.ParseDocument(file));

  if (auto pTarget = ddl.GetRootElement()->FindChildOfType("Target", "Default"))
  {
    if (auto pValue = pTarget->FindChildOfType(xiiOpenDdlPrimitiveType::String, "PluginName"))
    {
      m_sPluginName = pValue->GetPrimitivesString()[0];
    }

    if (auto pValue = pTarget->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Compiler"))
    {
      if (pValue->GetPrimitivesString()[0] == "Vs2022")
        m_Compiler = Compiler::Vs2022;
      else
        m_Compiler = Compiler::None;
    }
  }

  return XII_SUCCESS;
}
