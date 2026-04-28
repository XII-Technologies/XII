/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/DragDropHandler.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDragDropHandler, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiDragDropHandler* xiiDragDropHandler::s_pActiveDnD = nullptr;

xiiDragDropHandler::xiiDragDropHandler() = default;

xiiDragDropHandler* xiiDragDropHandler::FindDragDropHandler(const xiiDragDropInfo* pInfo)
{
  float               fBestValue = 0.0f;
  xiiDragDropHandler* pBestDnD   = nullptr;

  xiiRTTI::ForEachDerivedType<xiiDragDropHandler>(
    [&](const xiiRTTI* pRtti) {
      xiiDragDropHandler* pDnD = pRtti->GetAllocator()->Allocate<xiiDragDropHandler>();

      const float fValue = pDnD->CanHandle(pInfo);
      if (fValue > fBestValue)
      {
        if (pBestDnD != nullptr)
        {
          pBestDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(pBestDnD);
        }

        fBestValue = fValue;
        pBestDnD   = pDnD;
      }
      else
      {
        pDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(pDnD);
      }
    },
    xiiRTTI::ForEachOptions::ExcludeNonAllocatable);

  return pBestDnD;
}

bool xiiDragDropHandler::BeginDragDropOperation(const xiiDragDropInfo* pInfo, xiiDragDropConfig* pConfigToFillOut)
{
  XII_ASSERT_DEV(s_pActiveDnD == nullptr, "A drag & drop handler is already active");

  xiiDragDropHandler* pHandler = FindDragDropHandler(pInfo);

  if (pHandler != nullptr)
  {
    if (pConfigToFillOut != nullptr)
      pHandler->RequestConfiguration(pConfigToFillOut);

    s_pActiveDnD = pHandler;
    s_pActiveDnD->OnDragBegin(pInfo);
    return true;
  }

  return false;
}

void xiiDragDropHandler::UpdateDragDropOperation(const xiiDragDropInfo* pInfo)
{
  if (s_pActiveDnD == nullptr)
    return;

  s_pActiveDnD->OnDragUpdate(pInfo);
}

void xiiDragDropHandler::FinishDragDrop(const xiiDragDropInfo* pInfo)
{
  if (s_pActiveDnD == nullptr)
    return;

  s_pActiveDnD->OnDrop(pInfo);

  s_pActiveDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(s_pActiveDnD);
  s_pActiveDnD = nullptr;
}

void xiiDragDropHandler::CancelDragDrop()
{
  if (s_pActiveDnD == nullptr)
    return;

  s_pActiveDnD->OnDragCancel();

  s_pActiveDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(s_pActiveDnD);
  s_pActiveDnD = nullptr;
}

bool xiiDragDropHandler::CanDropOnly(const xiiDragDropInfo* pInfo)
{
  XII_ASSERT_DEV(s_pActiveDnD == nullptr, "A drag & drop handler is already active");

  xiiDragDropHandler* pHandler = FindDragDropHandler(pInfo);

  if (pHandler != nullptr)
  {
    pHandler->GetDynamicRTTI()->GetAllocator()->Deallocate(pHandler);
    return true;
  }

  return false;
}

bool xiiDragDropHandler::DropOnly(const xiiDragDropInfo* pInfo)
{
  XII_ASSERT_DEV(s_pActiveDnD == nullptr, "A drag & drop handler is already active");

  if (BeginDragDropOperation(pInfo))
  {
    FinishDragDrop(pInfo);
    return true;
  }

  return false;
}
