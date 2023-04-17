#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <RmlUiPlugin/Implementation/EventListener.h>
#include <RmlUiPlugin/Implementation/Extractor.h>
#include <RmlUiPlugin/Implementation/FileInterface.h>
#include <RmlUiPlugin/Implementation/SystemInterface.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

xiiResult xiiRmlUiConfiguration::Save(xiiStringView sFile) const
{
  XII_LOG_BLOCK("xiiRmlUiConfiguration::Save()");

  xiiFileWriter file;
  if (file.Open(sFile).Failed())
    return XII_FAILURE;

  xiiOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(xiiOpenDdlWriter::TypeStringMode::Compliant);

  writer.BeginObject("Fonts");
  for (auto& font : m_Fonts)
  {
    xiiOpenDdlUtils::StoreString(writer, font);
  }
  writer.EndObject();

  return XII_SUCCESS;
}

xiiResult xiiRmlUiConfiguration::Load(xiiStringView sFile)
{
  XII_LOG_BLOCK("xiiRmlUiConfiguration::Load()");

  m_Fonts.Clear();

  xiiFileReader file;
  if (file.Open(sFile).Failed())
    return XII_FAILURE;

  xiiOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, xiiLog::GetThreadLocalLogSystem()).Failed())
  {
    xiiLog::Error("Failed to parse RmlUi config file '{0}'", sFile);
    return XII_FAILURE;
  }

  const xiiOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const xiiOpenDdlReaderElement* pChild = pTree->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Fonts"))
    {
      for (const xiiOpenDdlReaderElement* pFont = pChild->GetFirstChild(); pFont != nullptr; pFont = pFont->GetSibling())
      {
        m_Fonts.PushBack(pFont->GetPrimitivesString()[0]);
      }
    }
  }

  return XII_SUCCESS;
}

bool xiiRmlUiConfiguration::operator==(const xiiRmlUiConfiguration& rhs) const
{
  return m_Fonts == rhs.m_Fonts;
}

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_SINGLETON(xiiRmlUi);

struct xiiRmlUi::Data
{
  xiiMutex                    m_ExtractionMutex;
  xiiRmlUiInternal::Extractor m_Extractor;

  xiiRmlUiInternal::FileInterface   m_FileInterface;
  xiiRmlUiInternal::SystemInterface m_SystemInterface;

  xiiRmlUiInternal::ContextInstancer       m_ContextInstancer;
  xiiRmlUiInternal::EventListenerInstancer m_EventListenerInstancer;

  xiiDynamicArray<xiiRmlUiContext*> m_Contexts;

  xiiRmlUiConfiguration m_Config;
};

xiiRmlUi::xiiRmlUi() :
  m_SingletonRegistrar(this)
{
  m_pData = XII_DEFAULT_NEW(Data);

  Rml::SetRenderInterface(&m_pData->m_Extractor);
  Rml::SetFileInterface(&m_pData->m_FileInterface);
  Rml::SetSystemInterface(&m_pData->m_SystemInterface);

  Rml::Initialise();

  Rml::Factory::RegisterContextInstancer(&m_pData->m_ContextInstancer);
  Rml::Factory::RegisterEventListenerInstancer(&m_pData->m_EventListenerInstancer);

  if (m_pData->m_Config.Load().Failed())
  {
    xiiLog::Warning("No valid RmlUi configuration file available in '{}'.", xiiRmlUiConfiguration::s_sConfigFile);
    return;
  }

  for (auto& font : m_pData->m_Config.m_Fonts)
  {
    if (Rml::LoadFontFace(font.GetData()) == false)
    {
      xiiLog::Warning("Failed to load font face '{0}'.", font);
    }
  }
}

xiiRmlUi::~xiiRmlUi()
{
  Rml::Shutdown();
}

xiiRmlUiContext* xiiRmlUi::CreateContext(const char* szName, const xiiVec2U32& vInitialSize)
{
  xiiRmlUiContext* pContext = static_cast<xiiRmlUiContext*>(Rml::CreateContext(szName, Rml::Vector2i(vInitialSize.x, vInitialSize.y)));

  m_pData->m_Contexts.PushBack(pContext);

  return pContext;
}

void xiiRmlUi::DeleteContext(xiiRmlUiContext* pContext)
{
  m_pData->m_Contexts.RemoveAndCopy(pContext);

  Rml::RemoveContext(pContext->GetName());
}

bool xiiRmlUi::AnyContextWantsInput()
{
  for (auto pContext : m_pData->m_Contexts)
  {
    if (pContext->WantsInput())
      return true;
  }

  return false;
}

void xiiRmlUi::ExtractContext(xiiRmlUiContext& ref_context, xiiMsgExtractRenderData& ref_msg)
{
  if (ref_context.HasDocument() == false)
    return;

  // Unfortunately we need to hold a lock for the whole extraction of a context since RmlUi is not thread safe.
  XII_LOCK(m_pData->m_ExtractionMutex);

  ref_context.ExtractRenderData(m_pData->m_Extractor);

  if (ref_context.m_pRenderData != nullptr)
  {
    ref_msg.AddRenderData(ref_context.m_pRenderData, xiiDefaultRenderDataCategories::GUI, xiiRenderData::Caching::Never);
  }
}
