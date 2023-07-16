#include <Core/CorePCH.h>

#include <Core/World/WorldModule.h>
#include <Core/World/WorldModuleConfig.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

xiiResult xiiWorldModuleConfig::Save()
{
  m_InterfaceImpls.Sort();

  xiiStringBuilder sPath;
  sPath = ":project/WorldModules.ddl";

  xiiFileWriter file;
  if (file.Open(sPath).Failed())
    return XII_FAILURE;

  xiiOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(xiiOpenDdlWriter::TypeStringMode::Compliant);

  for (auto& interfaceImpl : m_InterfaceImpls)
  {
    writer.BeginObject("InterfaceImpl");

    xiiOpenDdlUtils::StoreString(writer, interfaceImpl.m_sInterfaceName, "Interface");
    xiiOpenDdlUtils::StoreString(writer, interfaceImpl.m_sImplementationName, "Implementation");

    writer.EndObject();
  }

  return XII_SUCCESS;
}

void xiiWorldModuleConfig::Load()
{
  const char* szPath = ":project/WorldModules.ddl";

  XII_LOG_BLOCK("xiiWorldModuleConfig::Load()", szPath);

  m_InterfaceImpls.Clear();

  xiiFileReader file;
  if (file.Open(szPath).Failed())
  {
    xiiLog::Dev("World module config file is not available: '{0}'", szPath);
    return;
  }
  else
  {
    xiiLog::Success("World module config file is available: '{0}'", szPath);
  }

  xiiOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
  {
    xiiLog::Error("Failed to parse world module config file '{0}'", szPath);
    return;
  }

  const xiiOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const xiiOpenDdlReaderElement* pInterfaceImpl = pTree->GetFirstChild(); pInterfaceImpl != nullptr;
       pInterfaceImpl                                = pInterfaceImpl->GetSibling())
  {
    if (!pInterfaceImpl->IsCustomType("InterfaceImpl"))
      continue;

    const xiiOpenDdlReaderElement* pInterface      = pInterfaceImpl->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Interface");
    const xiiOpenDdlReaderElement* pImplementation = pInterfaceImpl->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Implementation");

    // this prevents duplicates
    AddInterfaceImplementation(pInterface->GetPrimitivesString()[0], pImplementation->GetPrimitivesString()[0]);
  }
}

void xiiWorldModuleConfig::Apply()
{
  XII_LOG_BLOCK("xiiWorldModuleConfig::Apply");

  for (const auto& interfaceImpl : m_InterfaceImpls)
  {
    xiiWorldModuleFactory::GetInstance()->RegisterInterfaceImplementation(interfaceImpl.m_sInterfaceName, interfaceImpl.m_sImplementationName);
  }
}

void xiiWorldModuleConfig::AddInterfaceImplementation(xiiStringView sInterfaceName, xiiStringView sImplementationName)
{
  for (auto& interfaceImpl : m_InterfaceImpls)
  {
    if (interfaceImpl.m_sInterfaceName == sInterfaceName)
    {
      interfaceImpl.m_sImplementationName = sImplementationName;
      return;
    }
  }

  m_InterfaceImpls.PushBack({sInterfaceName, sImplementationName});
}

void xiiWorldModuleConfig::RemoveInterfaceImplementation(xiiStringView sInterfaceName)
{
  for (xiiUInt32 i = 0; i < m_InterfaceImpls.GetCount(); ++i)
  {
    if (m_InterfaceImpls[i].m_sInterfaceName == sInterfaceName)
    {
      m_InterfaceImpls.RemoveAtAndCopy(i);
      return;
    }
  }
}


XII_STATICLINK_FILE(Core, Core_World_Implementation_WorldModuleConfig);
