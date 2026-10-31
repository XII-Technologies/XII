/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppSettings.h>
#include <Foundation/Application/Config/ProjectManifest.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

bool xiiCppSettings::IsValidPluginName(xiiStringView sName)
{
  if (sName.IsEmpty())
    return false;

  bool bFirst = true;
  for (const auto c : sName)
  {
    const bool bLetter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
    if (!bLetter && (bFirst || c < '0' || c > '9'))
      return false;
    bFirst = false;
  }
  return true;
}

xiiResult xiiCppSettings::Save(xiiStringView sFile)
{
  if (!IsValidPluginName(m_sPluginName))
    return XII_FAILURE;

  xiiDeferredFileWriter file;
  xiiStringBuilder      resolved;
  XII_SUCCEED_OR_RETURN(xiiProjectManifest::ResolveConfigPath(sFile, resolved).GetResult());
  file.SetOutput(resolved);

  xiiOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  ddl.BeginObject("Target", "Default");

  xiiOpenDdlUtils::StoreString(ddl, m_sPluginName, "PluginName");

  ddl.EndObject();

  return file.Close();
}

xiiResult xiiCppSettings::Load(xiiStringView sFile)
{
  m_sPluginName.Clear();
  xiiFileReader    file;
  xiiStringBuilder resolved;
  XII_SUCCEED_OR_RETURN(xiiProjectManifest::ResolveConfigPath(sFile, resolved).GetResult());
  XII_SUCCEED_OR_RETURN(file.Open(resolved));

  xiiOpenDdlReader ddl;
  XII_SUCCEED_OR_RETURN(ddl.ParseDocument(file));

  if (auto pTarget = ddl.GetRootElement()->FindChildOfType("Target", "Default"))
  {
    if (auto pValue = pTarget->FindChildOfType(xiiOpenDdlPrimitiveType::String, "PluginName"))
    {
      m_sPluginName = pValue->GetPrimitivesString()[0];
    }
  }

  if (!IsValidPluginName(m_sPluginName))
  {
    m_sPluginName.Clear();
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}
