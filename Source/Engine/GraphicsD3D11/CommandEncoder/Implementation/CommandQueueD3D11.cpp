#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/CommandEncoder/CommandListD3D11.h>
#include <GraphicsD3D11/CommandEncoder/CommandQueueD3D11.h>
#include <GraphicsD3D11/Device/DeviceD3D11.h>

xiiGALCommandQueueD3D11::xiiGALCommandQueueD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(pDeviceD3D11, creationDescription)
{
}

xiiGALCommandQueueD3D11::~xiiGALCommandQueueD3D11()
{
  m_SwapChainCommandListReferences.Clear();

  for (xiiUInt32 i = 0; i < m_CommandLists.GetCount(); ++i)
  {
    auto pCommandList = m_CommandLists[i];

    XII_DEFAULT_DELETE(pCommandList)
  }
  m_CommandLists.Clear();
}

xiiGALCommandList* xiiGALCommandQueueD3D11::BeginCommandList()
{
  // Try to find a command list that has been reset.
  for (xiiUInt32 i = 0; i < m_CommandLists.GetCount(); ++i)
  {
    auto pCommandList = m_CommandLists[i];

    if (pCommandList->GetRecordingState() == xiiGALCommandList::RecordingState::Reset)
    {
      pCommandList->Begin();

      return pCommandList;
    }
  }

  // Allocate a new command list.
  xiiGALCommandListCreationDescription commandListDescription = {.m_QueueType = m_Description.m_QueueType};
  xiiGALDeviceD3D11*                   pDeviceD3D11           = static_cast<xiiGALDeviceD3D11*>(m_pDevice);
  xiiGALCommandList*                   pCommandListD3D11      = XII_DEFAULT_NEW(xiiGALCommandListD3D11, pDeviceD3D11, this, commandListDescription);

  m_CommandLists.PushFront(pCommandListD3D11);

  pCommandListD3D11->Begin();

  return pCommandListD3D11;
}

void xiiGALCommandQueueD3D11::AddSwapChainCommandListReference(xiiGALCommandListD3D11* pCommandListD3D11)
{
  m_SwapChainCommandListReferences.PushBack(pCommandListD3D11);
}

void xiiGALCommandQueueD3D11::RemoveSwapChainCommandListReference(xiiGALCommandListD3D11* pCommandListD3D11)
{
  for (xiiUInt32 i = 0; i < m_SwapChainCommandListReferences.GetCount(); ++i)
  {
    auto pCommandListReferenceD3D11 = m_SwapChainCommandListReferences[i];

    if (pCommandListReferenceD3D11 == pCommandListD3D11)
    {
      XII_ASSERT_DEV(pCommandListReferenceD3D11->GetRecordingState() != xiiGALCommandList::RecordingState::Reset, "Attempting to remove a command list in recording state is an error, until the command list has been reset.");

      m_SwapChainCommandListReferences.RemoveAtAndSwap(i);
      break;
    }
  }
}

void xiiGALCommandQueueD3D11::ReleaseSwapChainCommanListReferences()
{
  for (xiiUInt32 i = 0; i < m_SwapChainCommandListReferences.GetCount(); ++i)
  {
    auto pCommandListReferenceD3D11 = m_SwapChainCommandListReferences[i];

    if (pCommandListReferenceD3D11->GetRecordingState() != xiiGALCommandList::RecordingState::Reset)
    {
      pCommandListReferenceD3D11->Reset();
    }
  }
  m_SwapChainCommandListReferences.Clear();
}

void xiiGALCommandQueueD3D11::SubmitPlatform(xiiGALCommandList* pCommandList, bool bReset)
{
  if (pCommandList == nullptr)
    return;

  xiiGALDeviceD3D11*      pDeviceD3D11      = static_cast<xiiGALDeviceD3D11*>(m_pDevice);
  xiiGALCommandListD3D11* pCommandListD3D11 = static_cast<xiiGALCommandListD3D11*>(pCommandList);

  pDeviceD3D11->GetImmediateContext()->ExecuteCommandList(pCommandListD3D11->GetD3D11CommandList(), FALSE);

  if (bReset && (pCommandListD3D11->GetRecordingState() != xiiGALCommandList::RecordingState::Reset))
  {
    pCommandListD3D11->Reset();
  }
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_CommandEncoder_Implementation_CommandQueueD3D11);
