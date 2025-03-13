#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/LongOps/LongOpManager.h>

void xiiLongOpManager::Startup(xiiProcessCommunicationChannel* pCommunicationChannel)
{
  m_pCommunicationChannel = pCommunicationChannel;
  m_pCommunicationChannel->m_Events.AddEventHandler(xiiMakeDelegate(&xiiLongOpManager::ProcessCommunicationChannelEventHandler, this), m_Unsubscriber);
}

void xiiLongOpManager::Shutdown()
{
  XII_LOCK(m_Mutex);

  m_Unsubscriber.Unsubscribe();
  m_pCommunicationChannel = nullptr;
}
