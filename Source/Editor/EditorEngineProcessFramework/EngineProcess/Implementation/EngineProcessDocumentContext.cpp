#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <GraphicsCore/Pipeline/RenderDataManager.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>
#include <Texture/Image/ImageUtils.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEngineProcessDocumentContext, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiHashTable<xiiUuid, xiiEngineProcessDocumentContext*> xiiEngineProcessDocumentContext::s_DocumentContexts;

xiiEngineProcessDocumentContext* xiiEngineProcessDocumentContext::GetDocumentContext(xiiUuid guid)
{
  xiiEngineProcessDocumentContext* pResult = nullptr;
  s_DocumentContexts.TryGetValue(guid, pResult);
  return pResult;
}

void xiiEngineProcessDocumentContext::AddDocumentContext(xiiUuid guid, const xiiVariant& metaData, xiiEngineProcessDocumentContext* pContext, xiiEngineProcessCommunicationChannel* pIPC, xiiStringView sDocumentType)
{
  XII_ASSERT_DEV(!s_DocumentContexts.Contains(guid), "Cannot add a view with an index that already exists");
  s_DocumentContexts[guid] = pContext;

  pContext->Initialize(guid, metaData, pIPC, sDocumentType);
}

bool xiiEngineProcessDocumentContext::PendingOperationsInProgress()
{
  for (auto it = s_DocumentContexts.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->PendingOperationInProgress())
      return true;
  }
  return false;
}

void xiiEngineProcessDocumentContext::UpdateDocumentContexts()
{
  XII_PROFILE_SCOPE("UpdateDocumentContexts");
  for (auto it = s_DocumentContexts.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->UpdateDocumentContext();
  }
}

void xiiEngineProcessDocumentContext::DestroyDocumentContext(xiiUuid guid)
{
  xiiEngineProcessDocumentContext* pContext = nullptr;
  if (s_DocumentContexts.Remove(guid, &pContext))
  {
    pContext->Deinitialize();
    pContext->GetDynamicRTTI()->GetAllocator()->Deallocate(pContext);
  }
}

xiiBoundingBoxSphere xiiEngineProcessDocumentContext::GetWorldBounds(xiiWorld* pWorld)
{
  xiiBoundingBoxSphere bounds = xiiBoundingBoxSphere::MakeInvalid();

  {
    XII_LOCK(pWorld->GetReadMarker());

    XII_ASSERT_DEV(!pWorld->GetWorldSimulationEnabled(), "World simulation must be disabled to get bounds!");

    const xiiWorld* pConstWorld = pWorld;
    for (auto it = pConstWorld->GetObjects(); it.IsValid(); ++it)
    {
      const xiiGameObject* pObj = it;

      const auto& b = pObj->GetGlobalBounds();

      if (b.IsValid())
        bounds.ExpandToInclude(b);
    }
  }

  if (!bounds.IsValid())
    bounds = xiiBoundingBoxSphere::MakeFromCenterExtents(xiiVec3::MakeZero(), xiiVec3(1, 1, 1), 2);

  return bounds;
}

xiiEngineProcessDocumentContext::xiiEngineProcessDocumentContext(xiiBitflags<xiiEngineProcessDocumentContextFlags> flags) :
  m_Flags(flags)
{
  GetContext().m_Events.AddEventHandler(xiiMakeDelegate(&xiiEngineProcessDocumentContext::WorldRttiConverterContextEventHandler, this));
}

xiiEngineProcessDocumentContext::~xiiEngineProcessDocumentContext()
{
  XII_ASSERT_DEV(m_pWorld == nullptr, "World has not been deleted! Call 'xiiEngineProcessDocumentContext::DestroyDocumentContext'");

  GetContext().m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiEngineProcessDocumentContext::WorldRttiConverterContextEventHandler, this));
}

void xiiEngineProcessDocumentContext::Initialize(const xiiUuid& documentGuid, const xiiVariant& metaData, xiiEngineProcessCommunicationChannel* pIPC, xiiStringView sDocumentType)
{
  m_DocumentGuid = documentGuid;
  m_MetaData     = metaData;
  m_pIPC         = pIPC;

  if (m_sDocumentType != sDocumentType)
  {
    m_sDocumentType = sDocumentType;
  }

  if (m_Flags.IsSet(xiiEngineProcessDocumentContextFlags::CreateWorld))
  {
    xiiStringBuilder tmp;
    xiiWorldDesc     desc(xiiConversionUtils::ToString(m_DocumentGuid, tmp));
    desc.m_bReportErrorWhenStaticObjectMoves = false;

    m_pWorld = XII_DEFAULT_NEW(xiiWorld, desc);
    m_pWorld->SetGameObjectReferenceResolver(xiiMakeDelegate(&xiiEngineProcessDocumentContext::ResolveStringToGameObjectHandle, this));

    GetContext().m_pWorld = m_pWorld;
    m_Mirror.InitReceiver(&GetContext());
  }
  OnInitialize();
}

void xiiEngineProcessDocumentContext::Deinitialize()
{
  OnDeinitialize();

  ClearViewContexts();
  m_Mirror.Clear();
  m_Mirror.DeInit();
  GetContext().Clear();

  CleanUpContextSyncObjects();
  if (m_Flags.IsSet(xiiEngineProcessDocumentContextFlags::CreateWorld))
  {
    XII_DEFAULT_DELETE(m_pWorld);
  }
  m_pWorld = nullptr;
}

void xiiEngineProcessDocumentContext::SendProcessMessage(xiiProcessMessage* pMsg)
{
  m_pIPC->SendMessage(pMsg);
}

void xiiEngineProcessDocumentContext::HandleMessage(const xiiEditorEngineDocumentMsg* pMsg)
{
  XII_LOCK(m_pWorld->GetWriteMarker());

  const bool bIsRemoteProcess = xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode();

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiEntityMsgToEngine>())
  {
    const xiiEntityMsgToEngine* pMsg2 = static_cast<const xiiEntityMsgToEngine*>(pMsg);
    m_Mirror.ApplyOp(const_cast<xiiObjectChange&>(pMsg2->m_change));

    xiiRttiConverterObject target = GetContext().GetObjectByGUID(pMsg2->m_change.m_Root);

    if (target.m_pType == nullptr || target.m_pObject == nullptr)
      return;

    if (target.m_pType == xiiGetStaticRTTI<xiiGameObject>())
    {
      xiiGameObject* pObject = static_cast<xiiGameObject*>(target.m_pObject);
      if (pObject != nullptr && pObject->IsStatic())
      {
        pObject->GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->DeleteCachedRenderDataForObjectRecursive(pObject);
      }
    }
    else if (target.m_pType->IsDerivedFrom<xiiComponent>())
    {
      xiiComponent* pComponent = static_cast<xiiComponent*>(target.m_pObject);
      if (pComponent != nullptr && pComponent->GetOwner()->IsStatic())
      {
        pComponent->GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->DeleteCachedRenderData(pComponent->GetOwner()->GetHandle(), pComponent->GetHandle());
      }
    }
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiEditorEngineSyncObjectMsg>())
  {
    const xiiEditorEngineSyncObjectMsg* pMsg2 = static_cast<const xiiEditorEngineSyncObjectMsg*>(pMsg);

    ProcessEditorEngineSyncObjectMsg(*pMsg2);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiObjectTagMsgToEngine>())
  {
    const xiiObjectTagMsgToEngine* pMsg2 = static_cast<const xiiObjectTagMsgToEngine*>(pMsg);

    SetTagOnObject(pMsg2->m_ObjectGuid, pMsg2->m_sTag, pMsg2->m_bSetTag, pMsg2->m_bApplyOnAllChildren);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiExportDocumentMsgToEngine>())
  {
    // ignore when this is a remote process
    if (bIsRemoteProcess)
      return;

    const xiiExportDocumentMsgToEngine* pMsg2 = static_cast<const xiiExportDocumentMsgToEngine*>(pMsg);
    xiiExportDocumentMsgToEditor        ret;
    ret.m_DocumentGuid = pMsg->m_DocumentGuid;

    xiiStatus res        = ExportDocument(pMsg2);
    ret.m_bOutputSuccess = res.Succeeded();
    ret.m_sFailureMsg    = res.GetMessageString();

    if (!ret.m_bOutputSuccess)
    {
      xiiLog::Error("Could not export to file '{0}'.", pMsg2->m_sOutputFile);
    }

    SendProcessMessage(&ret);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiCreateThumbnailMsgToEngine>())
  {
    // ignore when this is a remote process
    if (bIsRemoteProcess)
      return;

    xiiFileSystem::ReloadAllExternalDataDirectoryConfigs();
    xiiResourceManager::ReloadAllResources(false);
    UpdateSyncObjects();
    const xiiCreateThumbnailMsgToEngine* pMsg2 = static_cast<const xiiCreateThumbnailMsgToEngine*>(pMsg);
    // As long as the thumbnail context is alive, we will trigger UpdateThumbnailViewContext
    // inside the UpdateDocumentContext function until the thumbnail rendering has converged and
    // the data is send back as a response.
    CreateThumbnailViewContext(pMsg2);
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiEditorEngineViewMsg>())
  {
    if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiViewRedrawMsgToEngine>())
    {
      UpdateSyncObjects();
    }

    const xiiEditorEngineViewMsg* pViewMsg = static_cast<const xiiEditorEngineViewMsg*>(pMsg);
    XII_ASSERT_DEV(pViewMsg->m_uiViewID < 0xFFFFFFFF, "Invalid view ID in '{0}'", pMsg->GetDynamicRTTI()->GetTypeName());

    m_ViewContexts.EnsureCount(pViewMsg->m_uiViewID + 1);

    if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiViewDestroyedMsgToEngine>())
    {
      if (m_ViewContexts[pViewMsg->m_uiViewID] != nullptr)
      {
        DestroyViewContext(m_ViewContexts[pViewMsg->m_uiViewID]);
        m_ViewContexts[pViewMsg->m_uiViewID] = nullptr;

        xiiLog::Debug("Destroyed View {0}", pViewMsg->m_uiViewID);
      }
      xiiViewDestroyedResponseMsgToEditor response;
      response.m_DocumentGuid = pViewMsg->m_DocumentGuid;
      response.m_uiViewID     = pViewMsg->m_uiViewID;
      m_pIPC->SendMessage(&response);
    }
    else
    {
      if (m_ViewContexts[pViewMsg->m_uiViewID] == nullptr)
      {
        if (!xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
        {
          m_ViewContexts[pViewMsg->m_uiViewID] = CreateViewContext();
        }
        else
        {
          m_ViewContexts[pViewMsg->m_uiViewID] = XII_DEFAULT_NEW(xiiRemoteEngineProcessViewContext, this);
        }

        m_ViewContexts[pViewMsg->m_uiViewID]->SetViewID(pViewMsg->m_uiViewID);
      }

      m_ViewContexts[pViewMsg->m_uiViewID]->HandleViewMessage(pViewMsg);
    }

    return;
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiViewHighlightMsgToEngine>())
  {
    // ignore when this is a remote process
    if (bIsRemoteProcess)
      return;

    const xiiViewHighlightMsgToEngine* pMsg2 = static_cast<const xiiViewHighlightMsgToEngine*>(pMsg);

    GetContext().m_uiHighlightID = GetContext().m_ComponentPickingMap.GetHandle(pMsg2->m_HighlightObject);

    if (GetContext().m_uiHighlightID == 0)
      GetContext().m_uiHighlightID = GetContext().m_OtherPickingMap.GetHandle(pMsg2->m_HighlightObject);
  }
}

void xiiEngineProcessDocumentContext::AddSyncObject(xiiEditorEngineSyncObject* pSync)
{
  pSync->Configure(m_DocumentGuid, [this](xiiEditorEngineSyncObject* pSync) { RemoveSyncObject(pSync); });

  m_SyncObjects[pSync->GetGuid()] = pSync;
}

void xiiEngineProcessDocumentContext::RemoveSyncObject(xiiEditorEngineSyncObject* pSync)
{
  m_SyncObjects.Remove(pSync->GetGuid());
}

xiiEditorEngineSyncObject* xiiEngineProcessDocumentContext::FindSyncObject(const xiiUuid& guid)
{
  return m_SyncObjects.GetValueOrDefault(guid, nullptr);
}

void xiiEngineProcessDocumentContext::ClearViewContexts()
{
  for (xiiEngineProcessViewContext* pContext : m_ViewContexts)
  {
    DestroyViewContext(pContext);
  }

  m_ViewContexts.Clear();
}


void xiiEngineProcessDocumentContext::CleanUpContextSyncObjects()
{
  while (!m_SyncObjects.IsEmpty())
  {
    auto it = m_SyncObjects.GetIterator();
    it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
  }
}

void xiiEngineProcessDocumentContext::ProcessEditorEngineSyncObjectMsg(const xiiEditorEngineSyncObjectMsg& msg)
{
  auto it = m_SyncObjects.Find(msg.m_ObjectGuid);

  if (msg.m_sObjectType.IsEmpty())
  {
    // object has been deleted!
    if (it.IsValid())
    {
      it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
    }

    return;
  }

  const xiiRTTI*             pRtti       = xiiRTTI::FindTypeByName(msg.m_sObjectType);
  xiiEditorEngineSyncObject* pSyncObject = nullptr;
  bool                       bSetOwner   = false;

  if (pRtti == nullptr)
  {
    xiiLog::Error("Cannot sync object of type unknown '{0}' to engine process", msg.m_sObjectType);
    return;
  }

  if (!it.IsValid())
  {
    // object does not yet exist
    XII_ASSERT_DEV(pRtti->GetAllocator() != nullptr, "Sync object of type '{0}' does not have a default allocator", msg.m_sObjectType);
    void* pObject = pRtti->GetAllocator()->Allocate<void>();

    pSyncObject = static_cast<xiiEditorEngineSyncObject*>(pObject);
    bSetOwner   = true;
  }
  else
  {
    pSyncObject = it.Value();
  }

  xiiRawMemoryStreamReader reader(msg.m_ObjectData);

  xiiReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *pRtti, pSyncObject);

  if (bSetOwner)
  {
    AddSyncObject(pSyncObject);
  }

  pSyncObject->SetModified(true);
}

void xiiEngineProcessDocumentContext::Reset()
{
  xiiUuid guid = m_DocumentGuid;
  auto    ipc  = m_pIPC;

  Deinitialize();

  Initialize(guid, m_MetaData, ipc, m_sDocumentType);
}

void xiiEngineProcessDocumentContext::ClearExistingObjects()
{
  GetContext().DeleteExistingObjects();
}

void xiiEngineProcessDocumentContext::OnInitialize() {}
void xiiEngineProcessDocumentContext::OnDeinitialize() {}

bool xiiEngineProcessDocumentContext::PendingOperationInProgress() const
{
  auto pState = xiiGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState();
  return m_pThumbnailViewContext != nullptr || pState != nullptr;
}

void xiiEngineProcessDocumentContext::UpdateDocumentContext()
{
  if (xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
  {
    // in remote mode simply redraw all all views every time a context is updated
    for (xiiEngineProcessViewContext* pViewContext : m_ViewContexts)
    {
      if (pViewContext)
      {
        pViewContext->Redraw(false);
      }
    }
  }

  if (m_pThumbnailViewContext)
  {
    xiiResourceManager::ForceNoFallbackAcquisition(3);
    m_uiThumbnailConvergenceFrames++;

    if (!UpdateThumbnailViewContext(m_pThumbnailViewContext))
    {
      m_uiThumbnailConvergenceFrames = 0;
    }

    if (m_uiThumbnailConvergenceFrames > ThumbnailConvergenceFramesTarget)
    {
      xiiCreateThumbnailMsgToEditor ret;
      ret.m_DocumentGuid = GetDocumentGuid();

      // Download image
      {
        xiiSharedPtr<xiiGALDevice> pDevice        = xiiGALDevice::GetDefaultDevice();
        auto                       pGraphicsQueue = xiiGALDevice::GetDefaultDevice()->GetCommandQueue();

        xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
        XII_ASSERT_DEV(pCommandList != nullptr, "Failed to create command list!");

        pCommandList->Begin();
        {
          pCommandList->BeginDebugGroup("Thumbnail Readback");
          {
            pCommandList->CopyTexture(m_pThumbnailColorRT, m_pThumbnailColorRTStaging);
          }
          pCommandList->EndDebugGroup();
        }
        pCommandList->End();
        pGraphicsQueue->Submit(pCommandList);

        const xiiEnum<xiiGALResourceFormat> format = m_pThumbnailColorRT->GetDescription().m_Format;

        xiiImageHeader header;
        header.SetImageFormat(xiiTextureUtils::GalFormatToImageFormat(format, true));
        header.SetWidth(m_uiThumbnailWidth);
        header.SetHeight(m_uiThumbnailHeight);
        xiiImage image;
        image.ResetAndAlloc(header);
        XII_ASSERT_DEV(static_cast<xiiUInt64>(m_uiThumbnailWidth) * static_cast<xiiUInt64>(m_uiThumbnailHeight) * 4 == header.ComputeDataSize(), "Thumbnail xiiImage has different size than data buffer!");

        const xiiUInt32 uiStride      = 4U * m_uiThumbnailWidth;
        const xiiUInt32 uiDepthStride = 4U * m_uiThumbnailWidth * m_uiThumbnailHeight;
        auto*           pImageData    = image.GetPixelPointer<xiiUInt8>();

        pCommandList->Begin();
        {
          pCommandList->BeginDebugGroup("Thumbnail Readback Download");
          {
            xiiGALTextureMipLevelData      sourceSubResource;
            xiiGALMappedTextureSubresource mappedSubResource;
            if (pCommandList->MapTextureSubresource(m_pThumbnailColorRTStaging, sourceSubResource, xiiGALMapType::Read, xiiGALMapFlags::None, nullptr, mappedSubResource).Succeeded())
            {
              const auto& textureDescription = m_pThumbnailColorRTStaging->GetDescription();
              const auto& formatProperties   = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

              if (mappedSubResource.m_pData)
              {
                /// \todo Support depth pitch.
                if (mappedSubResource.m_uiStride == uiStride)
                {
                  const xiiUInt32 uiMemorySize = formatProperties.GetElementSize() * xiiGALTextureUtilities::GetMipSize(textureDescription.m_Size.width, sourceSubResource.m_uiMipLevel) * xiiGALTextureUtilities::GetMipSize(textureDescription.m_Size.height, sourceSubResource.m_uiMipLevel);

                  memcpy(pImageData, mappedSubResource.m_pData, uiMemorySize);
                }
                else
                {
                  // Copy row by row.
                  const xiiUInt32 uiHeight = xiiGALTextureUtilities::GetMipSize(textureDescription.m_Size.height, sourceSubResource.m_uiMipLevel);

                  for (xiiUInt32 y = 0; y < uiHeight; ++y)
                  {
                    const void* pSource      = xiiMemoryUtils::AddByteOffset(mappedSubResource.m_pData, y * mappedSubResource.m_uiStride);
                    void*       pDestination = xiiMemoryUtils::AddByteOffset(pImageData, y * uiStride);

                    memcpy(pDestination, pSource, formatProperties.GetElementSize() * xiiGALTextureUtilities::GetMipSize(textureDescription.m_Size.width, sourceSubResource.m_uiMipLevel));
                  }
                }
              }
              else
              {
                xiiLog::Error("Failed to map texture subresource for reading backbuffer data.");
              }

              pCommandList->UnmapTextureSubresource(m_pThumbnailColorRTStaging, sourceSubResource).IgnoreResult();
            }
          }
          pCommandList->EndDebugGroup();
        }
        pCommandList->End();
        pGraphicsQueue->Submit(pCommandList);

        xiiImage  imageSwap;
        xiiImage* pImage     = &image;
        xiiImage* pImageSwap = &imageSwap;
        for (xiiUInt32 uiSuperscaleFactor = ThumbnailSuperscaleFactor; uiSuperscaleFactor > 1; uiSuperscaleFactor /= 2)
        {
          xiiImageUtils::Scale(*pImage, *pImageSwap, pImage->GetWidth() / 2, pImage->GetHeight() / 2).IgnoreResult();
          xiiMath::Swap(pImage, pImageSwap);
        }

        ret.m_ThumbnailData.SetCountUninitialized((m_uiThumbnailWidth / ThumbnailSuperscaleFactor) * (m_uiThumbnailHeight / ThumbnailSuperscaleFactor) * 4);
        xiiMemoryUtils::Copy(ret.m_ThumbnailData.GetData(), pImage->GetPixelPointer<xiiUInt8>(), ret.m_ThumbnailData.GetCount());
      }

      DestroyThumbnailViewContext();

      // Send response.
      SendProcessMessage(&ret);
    }
    else
    {
      m_pThumbnailViewContext->Redraw(false);
    }
  }
}

xiiStatus xiiEngineProcessDocumentContext::ExportDocument(const xiiExportDocumentMsgToEngine* pMsg)
{
  return xiiStatus(xiiFmt("Export document not implemented for '{0}'", GetDynamicRTTI()->GetTypeName()));
}

void xiiEngineProcessDocumentContext::CreateThumbnailViewContext(const xiiCreateThumbnailMsgToEngine* pMsg)
{
  XII_ASSERT_DEV(!xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode(), "Wrong mode for thumbnail creation");
  XII_ASSERT_DEV(m_pThumbnailViewContext == nullptr, "Thumbnail rendering already in progress.");
  static_assert((ThumbnailSuperscaleFactor & (ThumbnailSuperscaleFactor - 1)) == 0, "ThumbnailSuperscaleFactor must be power of 2.");
  m_uiThumbnailConvergenceFrames = 0;
  m_uiThumbnailWidth             = pMsg->m_uiWidth * ThumbnailSuperscaleFactor;
  m_uiThumbnailHeight            = pMsg->m_uiHeight * ThumbnailSuperscaleFactor;
  m_pThumbnailViewContext        = CreateViewContext();

  // make sure the world is not simulating while making a screenshot
  {
    XII_LOCK(m_pWorld->GetWriteMarker());
    m_bWorldSimStateBeforeThumbnail = m_pWorld->GetWorldSimulationEnabled();
    m_pWorld->SetWorldSimulationEnabled(false);
  }

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Create render target for picking
  xiiGALTextureCreationDescription tcd;
  tcd.m_Type        = xiiGALResourceDimension::Texture2D;
  tcd.m_Format      = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  tcd.m_Size.width  = m_uiThumbnailWidth;
  tcd.m_Size.height = m_uiThumbnailHeight;
  tcd.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;

  m_pThumbnailColorRT = pDevice->CreateTexture(tcd);

  tcd.m_BindFlags      = {};
  tcd.m_Usage          = xiiGALResourceUsage::Staging;
  tcd.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;

  m_pThumbnailColorRTStaging = pDevice->CreateTexture(tcd);

  tcd.m_Format         = xiiGALResourceFormat::D32Float;
  tcd.m_CPUAccessFlags = {};
  tcd.m_Usage          = xiiGALResourceUsage::Mutable;
  tcd.m_BindFlags      = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;

  m_pThumbnailDepthRT = pDevice->CreateTexture(tcd);

  m_ThumbnailRenderTargets.m_pRTs[0]   = m_pThumbnailColorRT->GetDefaultView(xiiGALTextureViewType::RenderTarget);
  m_ThumbnailRenderTargets.m_pDSTarget = m_pThumbnailDepthRT->GetDefaultView(xiiGALTextureViewType::DepthStencil);
  m_pThumbnailViewContext->SetupRenderTarget({}, &m_ThumbnailRenderTargets, m_uiThumbnailWidth, m_uiThumbnailHeight);

  xiiResourceManager::ForceNoFallbackAcquisition(3);
  OnThumbnailViewContextRequested();
  UpdateThumbnailViewContext(m_pThumbnailViewContext);

  // disable editor specific render passes in the thumbnail view
  xiiView* pView = nullptr;
  if (xiiRenderWorld::TryGetView(m_pThumbnailViewContext->GetViewHandle(), pView))
  {
    pView->SetViewRenderMode(xiiViewRenderMode::Default);
    pView->SetRenderPassProperty("EditorSelectionPass", "Active", false);
    pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", false);
    pView->SetExtractorProperty("EditorGridExtractor", "Active", false);
    pView->SetRenderPassProperty("EditorPickingPass", "Active", false);

    for (const xiiString& sTag : pMsg->m_ViewExcludeTags)
    {
      pView->m_ExcludeTags.SetByName(sTag);
    }
  }

  m_pThumbnailViewContext->Redraw(false);

  OnThumbnailViewContextCreated();
}

void xiiEngineProcessDocumentContext::DestroyThumbnailViewContext()
{
  OnDestroyThumbnailViewContext();

  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  DestroyViewContext(m_pThumbnailViewContext);
  m_pThumbnailViewContext = nullptr;

  m_pThumbnailColorRTStaging.Clear();
  m_pThumbnailColorRT.Clear();
  m_pThumbnailDepthRT.Clear();

  m_pWorld->SetWorldSimulationEnabled(m_bWorldSimStateBeforeThumbnail);
}

bool xiiEngineProcessDocumentContext::UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext)
{
  xiiLog::Error("UpdateThumbnailViewContext not implemented for '{0}'", GetDynamicRTTI()->GetTypeName());
  return true;
}

void xiiEngineProcessDocumentContext::OnThumbnailViewContextCreated() {}
void xiiEngineProcessDocumentContext::OnDestroyThumbnailViewContext() {}

void xiiEngineProcessDocumentContext::SetTagOnObject(const xiiUuid& object, const char* szTag, bool bSet, bool recursive)
{
  xiiGameObjectHandle hObject = GetContext().m_GameObjectMap.GetHandle(object);

  const xiiTag& tag = xiiTagRegistry::GetGlobalRegistry().RegisterTag(szTag);

  xiiGameObject* pObject;
  if (m_pWorld->TryGetObject(hObject, pObject))
  {
    if (recursive)
    {
      if (bSet)
        SetTagRecursive(pObject, tag);
      else
        ClearTagRecursive(pObject, tag);
    }
    else
    {
      if (bSet)
        pObject->SetTag(tag);
      else
        pObject->RemoveTag(tag);
    }
  }
}

void xiiEngineProcessDocumentContext::SetTagRecursive(xiiGameObject* pObject, const xiiTag& tag)
{
  pObject->SetTag(tag);

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    SetTagRecursive(itChild, tag);
  }
}

void xiiEngineProcessDocumentContext::ClearTagRecursive(xiiGameObject* pObject, const xiiTag& tag)
{
  pObject->RemoveTag(tag);

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    ClearTagRecursive(itChild, tag);
  }
}

void xiiEngineProcessDocumentContext::WorldRttiConverterContextEventHandler(const xiiWorldRttiConverterContext::Event& e)
{
  if (e.m_Type == xiiWorldRttiConverterContext::Event::Type::GameObjectCreated)
  {
    // see whether the newly created object is already referenced by other objects
    auto it = m_GoRef_ReferencedBy.Find(e.m_ObjectGuid);
    if (!it.IsValid())
      return;

    xiiStringBuilder tmp;

    // iterate over all objects that may reference the new object
    for (const auto& ref : it.Value())
    {
      const xiiUuid compGuid = ref.m_ReferencedByComponent;
      if (!compGuid.IsValid())
        continue;

      // check whether the object that references the new object is already known (may be dead or not yet created as well)
      xiiComponentHandle hRefComp = GetContext().m_ComponentMap.GetHandle(compGuid);

      if (hRefComp.IsInvalidated())
        continue;

      xiiComponent* pRefComp = nullptr;
      if (!GetWorld()->TryGetComponent(hRefComp, pRefComp))
        continue;

      if (!ref.m_sComponentProperty.IsEmpty())
      {
        // in this case, a 'regular' component+property reference the new object
        // so we can just re-apply the reference (by setting the property again)
        // and thus trigger that the other object updates/fixes its internal state

        const xiiAbstractProperty* pAbsProp = pRefComp->GetDynamicRTTI()->FindPropertyByName(ref.m_sComponentProperty);
        if (pAbsProp == nullptr)
          continue;

        if (pAbsProp->GetCategory() != xiiPropertyCategory::Member)
          continue;

        xiiConversionUtils::ToString(e.m_ObjectGuid, tmp);

        xiiReflectionUtils::SetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(pAbsProp), pRefComp, tmp.GetData());
      }
      else
      {
        // in this case, the object was referenced from a component inside a prefab
        // the problem with prefabs is, that their internal objects and components are recreated all the time
        // (e.g. every time any exposed parameter is changed)
        // and since we have no GUIDs for the internal objects, we cannot directly update those internal references
        // we can, however, just update the entire prefab, which will kill all internal objects and recreate them

        xiiPrefabReferenceComponent* pPrefab = xiiDynamicCast<xiiPrefabReferenceComponent*>(pRefComp);
        XII_ASSERT_DEV(pPrefab != nullptr, "Game-Object reference update: Expected a xiiPrefabReferenceComponent");

        xiiPrefabReferenceComponentManager* pManager = xiiStaticCast<xiiPrefabReferenceComponentManager*>(pPrefab->GetOwningManager());
        pManager->AddToUpdateList(pPrefab);
      }
    }
  }
  else if (e.m_Type == xiiWorldRttiConverterContext::Event::Type::GameObjectDeleted)
  {
    m_GoRef_ReferencesTo.Remove(e.m_ObjectGuid);
  }
}

/// Tries to resolve a 'reference' (given in pData) to a xiiGameObject.
/// hThis is the 'owner' of the reference and szComponentProperty is the name of the reference property in that component.
///
/// There are two different use cases:
///
///  1) hThis is invalid and szComponentProperty is null:
///
///     This is used by xiiPrefabReferenceComponent::SerializeComponent() to check whether a string represents a game object reference.
///     It may be any arbitrary string and thus must not assert.
///     In this case a reference is always a stringyfied GUID.
///     Since this is only used for scene export, only the lookup shall be done and nothing else.
///
///  2) hThis and szComponentProperty represent a valid component+property combination:
///
///     This is called at edit time whenever a reference property is queried, which also happens whenever a reference is modified.
///     In this case we need to maintain two maps:
///       one that know which object references which other objects
///       one that knows by which other objects an object is referenced
///     These are needed to fix up references during undo/redo when objects get deleted and recreated.
///     Ie. when an object that has references or is referenced gets deleted and then undo restores it, the references should appear as well.
///
xiiGameObjectHandle xiiEngineProcessDocumentContext::ResolveStringToGameObjectHandle(const void* pData, xiiComponentHandle hThis, xiiStringView sComponentProperty) const
{
  const char* szTargetGuid = reinterpret_cast<const char*>(pData);

  if (hThis.IsInvalidated() && sComponentProperty.IsEmpty())
  {
    // This code path is used by xiiPrefabReferenceComponent::SerializeComponent() to check whether an arbitrary string may
    // represent a game object reference. References will always be stringyfied GUIDs.

    if (!xiiConversionUtils::IsStringUuid(szTargetGuid))
      return xiiGameObjectHandle();

    // convert string to GUID and check if references a known object
    return GetContext().m_GameObjectMap.GetHandle(xiiConversionUtils::ConvertStringToUuid(szTargetGuid));
  }



  xiiUuid srcComponentGuid = GetContext().m_ComponentMap.GetGuid(hThis);
  if (!srcComponentGuid.IsValid())
  {
    // if we do not know hThis, it is usually a component that was created by a prefab instance
    // since we need hThis/srcComponentGuid to update our tables who references whom, we now try to walk up the node hierarchy
    // until we find a known game object
    // there, currently, we assume to find a xiiPrefabReferenceComponent, which will be used as srcComponentGuid

    xiiComponent* pComponent = nullptr;
    if (!m_pWorld->TryGetComponent(hThis, pComponent))
      return xiiGameObjectHandle();

    xiiGameObject* pOwner = pComponent->GetOwner();

    // search the parents for a known game object
    while (pOwner)
    {
      srcComponentGuid = GetContext().m_GameObjectMap.GetGuid(pOwner->GetHandle());

      if (srcComponentGuid.IsValid())
        break;

      pOwner = pOwner->GetParent();
    }

    // currently we assume all these conditions should be met
    // have to check with reality, though
    XII_ASSERT_DEV(pOwner != nullptr, "Expected a known top level object");
    XII_ASSERT_DEV(srcComponentGuid.IsValid(), "Expected a known top level object");

    // for the time being we assume to find a prefab component here, but this may change if new use cases come up
    xiiPrefabReferenceComponent* pPrefabComponent;
    if (pOwner->TryGetComponentOfBaseType(pPrefabComponent))
    {
      srcComponentGuid = GetContext().m_ComponentMap.GetGuid(pPrefabComponent->GetHandle());
      XII_ASSERT_DEV(srcComponentGuid.IsValid(), "");

      // tag this reference as being special
      sComponentProperty = {};
    }
    else
    {
      // this could probably happen if we have a component that creates sub-objects even at edit time
      // and is not a prefab reference component
      // and uses game object references
      XII_ASSERT_DEV(false, "Expected a known xiiPrefabReferenceComponent as top-level object");
      return xiiGameObjectHandle();
    }
  }


  xiiUuid newTargetGuid;
  xiiUuid oldTargetGuid;

  if (xiiConversionUtils::IsStringUuid(szTargetGuid))
  {
    newTargetGuid = xiiConversionUtils::ConvertStringToUuid(szTargetGuid);
  }
  else
  {
    XII_ASSERT_DEV(xiiStringUtils::IsNullOrEmpty(szTargetGuid), "Expected GUID references");
  }

  if (sComponentProperty.IsEmpty())
  {
    return GetContext().m_GameObjectMap.GetHandle(newTargetGuid);
  }

  // overview for the steps below:
  //
  // check if m_GoRef_ReferencesTo[srcComponentGuid] already maps from [szComponentProperty] to something -> update (remove if pData is empty/invalid)
  // otherwise add reference
  //
  // if already mapped to something, remove reference from m_GoRef_ReferencedBy
  // then add new reference to m_GoRef_ReferencedBy



  // update which object this component+property map to
  {
    auto& referencesTo = m_GoRef_ReferencesTo[srcComponentGuid];

    // check all references from the src component
    for (xiiUInt32 i = 0; i < referencesTo.GetCount(); ++i)
    {
      // if this is the desired property, update it
      if (referencesTo[i].m_sComponentProperty == sComponentProperty)
      {
        // retrieve previous reference, needed to update m_GoRef_ReferencedBy
        oldTargetGuid = referencesTo[i].m_ReferenceToGameObject;

        // write the new reference
        if (newTargetGuid.IsValid())
        {
          referencesTo[i].m_ReferenceToGameObject = newTargetGuid;
        }
        else
        {
          referencesTo.RemoveAtAndSwap(i);
        }

        goto ref_to_is_updated;
      }
    }

    // if we end up here, the reference-to was previously unknown and must be added
    if (newTargetGuid.IsValid())
    {
      auto& refTo                   = referencesTo.ExpandAndGetRef();
      refTo.m_sComponentProperty    = sComponentProperty;
      refTo.m_ReferenceToGameObject = newTargetGuid;
    }
  }

ref_to_is_updated:

  // only need to updated m_GoRef_ReferencedBy if the reference has actually changed
  if (oldTargetGuid != newTargetGuid)
  {
    // if we referenced another object previously, remove the 'referenced-by' link to the old object
    if (oldTargetGuid.IsValid())
    {
      auto& referencedBy = m_GoRef_ReferencedBy[oldTargetGuid];

      for (xiiUInt32 i = 0; i < referencedBy.GetCount(); ++i)
      {
        if (referencedBy[i].m_ReferencedByComponent == srcComponentGuid && referencedBy[i].m_sComponentProperty == sComponentProperty)
        {
          referencedBy.RemoveAtAndSwap(i);
          break;
        }
      }
    }

    // if we are now referencing a valid object, add a 'referenced-by' link to the new object
    if (newTargetGuid.IsValid())
    {
      auto& referencedBy = m_GoRef_ReferencedBy[newTargetGuid];

      // this loop is currently only to validate that no bugs creeped in
      for (xiiUInt32 i = 0; i < referencedBy.GetCount(); ++i)
      {
        if (referencedBy[i].m_ReferencedByComponent == srcComponentGuid && referencedBy[i].m_sComponentProperty == sComponentProperty)
        {
          XII_REPORT_FAILURE("Go-reference was not updated correctly");
        }
      }

      // add the back-reference
      auto& newRef                   = referencedBy.ExpandAndGetRef();
      newRef.m_ReferencedByComponent = srcComponentGuid;
      newRef.m_sComponentProperty    = sComponentProperty;
    }
  }

  // just an optimization for the common case
  if (!newTargetGuid.IsValid())
    return xiiGameObjectHandle();

  return GetContext().m_GameObjectMap.GetHandle(newTargetGuid);
}

void xiiEngineProcessDocumentContext::UpdateSyncObjects()
{
  for (auto it : m_SyncObjects)
  {
    auto pSyncObject = it.Value();

    if (pSyncObject->GetModified())
    {
      // reset the modified state to make sure the object isn't updated unless a new sync messages comes in
      pSyncObject->SetModified(false);

      XII_LOCK(m_pWorld->GetWriteMarker());

      if (pSyncObject->SetupForEngine(m_pWorld, GetContext().m_uiNextComponentPickingID))
      {
        GetContext().m_OtherPickingMap.RegisterObject(pSyncObject->GetGuid(), GetContext().m_uiNextComponentPickingID);
        ++GetContext().m_uiNextComponentPickingID;
      }

      pSyncObject->UpdateForEngine(m_pWorld);
    }
  }
}
