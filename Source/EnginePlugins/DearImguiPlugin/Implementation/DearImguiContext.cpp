/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <DearImguiPlugin/DearImguiPluginPCH.h>

#include <DearImguiPlugin/DearImguiSubsystem.h>
#include <DearImguiPlugin/Implementation/DearImguiContextData.h>
#include <Foundation/Threading/ThreadUtils.h>

xiiImguiContext::xiiImguiContext()
{
  auto* previous = ImGui::GetCurrentContext();
  m_pContext     = ImGui::CreateContext();
  ImGui::SetCurrentContext(m_pContext);
  auto& io               = ImGui::GetIO();
  io.IniFilename         = nullptr; // Independent tools explicitly opt in to their own settings file.
  io.BackendRendererName = "XII GAL";
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;
  ImGui::GetPlatformIO().Renderer_TextureMaxWidth  = 4096;
  ImGui::GetPlatformIO().Renderer_TextureMaxHeight = 4096;
  ImGui::SetCurrentContext(previous);
}

xiiImguiContext::~xiiImguiContext()
{
  auto* previous = ImGui::GetCurrentContext();
  ImGui::SetCurrentContext(m_pContext);
  if (m_bFrameActive)
    ImGui::EndFrame();
  for (auto* texture : ImGui::GetPlatformIO().Textures)
  {
    texture->SetTexID(ImTextureID_Invalid);
    texture->SetStatus(ImTextureStatus_Destroyed);
  }
  ImGui::DestroyContext(m_pContext);
  ImGui::SetCurrentContext(previous == m_pContext ? nullptr : previous);
}

xiiImguiScopedContext::xiiImguiScopedContext(xiiImguiContextHandle hContext)
{
  m_pContext = xiiImguiSubsystem::AcquireContext(hContext);
  if (!m_pContext)
    return;
  m_pPrevious = ImGui::GetCurrentContext();
  ++m_pContext->m_uiScopes;
  ImGui::SetCurrentContext(m_pContext->m_pContext);
}

xiiImguiScopedContext::~xiiImguiScopedContext()
{
  if (!m_pContext)
    return;
  XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "ImGui scopes cannot move between threads.");
  ImGui::SetCurrentContext(m_pPrevious);
  --m_pContext->m_uiScopes;
}

xiiResult xiiImguiScopedContext::BeginFrame(const xiiImguiFrameDescription& description)
{
  if (!m_pContext || ImGui::GetCurrentContext() != m_pContext->m_pContext || m_pContext->m_bFrameActive ||
      !description.m_vDisplaySize.IsValid() || !description.m_vFramebufferScale.IsValid() ||
      description.m_vDisplaySize.x < 0 || description.m_vDisplaySize.y < 0 ||
      description.m_vFramebufferScale.x <= 0 || description.m_vFramebufferScale.y <= 0 ||
      !xiiMath::IsFinite(description.m_DeltaTime.GetSeconds()) || description.m_DeltaTime.GetSeconds() <= 0)
    return XII_FAILURE;
  auto& io                   = ImGui::GetIO();
  io.DisplaySize             = {description.m_vDisplaySize.x, description.m_vDisplaySize.y};
  io.DisplayFramebufferScale = {description.m_vFramebufferScale.x, description.m_vFramebufferScale.y};
  io.DeltaTime               = static_cast<float>(description.m_DeltaTime.GetSeconds());
  ImGui::NewFrame();
  m_pContext->m_bFrameActive = true;
  return XII_SUCCESS;
}

xiiResult xiiImguiScopedContext::EndFrame(xiiSharedPtr<const xiiImguiFrame>& out_frame)
{
  out_frame.Clear();
  if (!m_pContext || ImGui::GetCurrentContext() != m_pContext->m_pContext || !m_pContext->m_bFrameActive)
    return XII_FAILURE;
  ImGui::Render();
  m_pContext->m_bFrameActive = false;
  return m_pContext->Snapshot(out_frame);
}

xiiResult xiiImguiScopedContext::CancelFrame()
{
  if (!m_pContext || ImGui::GetCurrentContext() != m_pContext->m_pContext || !m_pContext->m_bFrameActive)
    return XII_FAILURE;
  ImGui::EndFrame();
  m_pContext->m_bFrameActive = false;
  return XII_SUCCESS;
}

void xiiImguiScopedContext::SetInputEnabled(bool bEnabled)
{
  if (!m_pContext || ImGui::GetCurrentContext() != m_pContext->m_pContext)
    return;
  m_pContext->m_bInputEnabled = bEnabled;
  if (!bEnabled)
  {
    ImGui::GetIO().ClearEventsQueue();
    ImGui::GetIO().ClearInputKeys();
    ImGui::GetIO().ClearInputMouse();
  }
}

xiiImguiInputCapture xiiImguiScopedContext::GetInputCapture() const
{
  if (!m_pContext || !m_pContext->m_bInputEnabled || ImGui::GetCurrentContext() != m_pContext->m_pContext)
    return {};
  const auto& io = ImGui::GetIO();
  return {io.WantCaptureMouse, io.WantCaptureKeyboard, io.WantTextInput};
}

ImTextureID xiiImguiContext::AddTexture(xiiSharedPtr<xiiImguiTexture> pTexture)
{
  const auto id = xiiImguiSubsystem::AllocateTextureId();
  m_Textures.PushBack({id, std::move(pTexture), false});
  return id;
}

xiiResult xiiImguiContext::RemoveTexture(ImTextureID id, bool bFontTexture)
{
  for (xiiUInt32 i = 0; i < m_Textures.GetCount(); ++i)
    if (m_Textures[i].m_Id == id && m_Textures[i].m_bFont == bFontTexture)
    {
      m_Textures.RemoveAtAndSwap(i);
      return XII_SUCCESS;
    }
  return XII_FAILURE;
}

xiiResult xiiImguiContext::UpdateTextures()
{
  for (auto* source : ImGui::GetPlatformIO().Textures)
  {
    if (source->Status == ImTextureStatus_WantDestroy)
    {
      XII_SUCCEED_OR_RETURN(RemoveTexture(source->GetTexID(), true));
      source->SetTexID(ImTextureID_Invalid);
      source->SetStatus(ImTextureStatus_Destroyed);
      continue;
    }
    if (source->Status != ImTextureStatus_WantCreate && source->Status != ImTextureStatus_WantUpdates)
      continue;
    if (source->Format != ImTextureFormat_RGBA32 || source->Width <= 0 || source->Height <= 0 || source->Width > 4096 || source->Height > 4096 || !source->Pixels)
      return XII_FAILURE;
    xiiSharedPtr<xiiImguiTexture> texture = XII_DEFAULT_NEW(xiiImguiTexture);
    texture->m_Size                       = {static_cast<xiiUInt32>(source->Width), static_cast<xiiUInt32>(source->Height)};
    texture->m_Pixels.SetCountUninitialized(source->Width * source->Height * 4U);
    xiiMemoryUtils::Copy(texture->m_Pixels.GetData(), static_cast<const xiiUInt8*>(source->GetPixels()), texture->m_Pixels.GetCount());
    if (source->Status == ImTextureStatus_WantCreate)
    {
      source->SetTexID(AddTexture(texture));
      m_Textures.PeekBack().m_bFont = true;
    }
    else
    {
      bool found = false;
      for (auto& entry : m_Textures)
        if (entry.m_Id == source->GetTexID() && entry.m_bFont)
        {
          entry.m_pTexture = texture;
          found            = true;
          break;
        }
      if (!found)
        return XII_FAILURE;
    }
    source->SetStatus(ImTextureStatus_OK);
  }
  return XII_SUCCESS;
}

xiiResult xiiImguiContext::Snapshot(xiiSharedPtr<const xiiImguiFrame>& out_frame)
{
  XII_SUCCEED_OR_RETURN(UpdateTextures());
  const auto* draw = ImGui::GetDrawData();
  if (!draw || !draw->Valid || draw->TotalVtxCount < 0 || draw->TotalIdxCount < 0)
    return XII_FAILURE;
  xiiSharedPtr<xiiImguiFrame> frame = XII_DEFAULT_NEW(xiiImguiFrame);
  frame->m_vDisplayRect             = {draw->DisplayPos.x, draw->DisplayPos.y, draw->DisplaySize.x, draw->DisplaySize.y};
  frame->m_Vertices.Reserve(draw->TotalVtxCount);
  frame->m_Indices.Reserve(draw->TotalIdxCount);
  for (const auto* list : draw->CmdLists)
  {
    const auto firstVertex = frame->m_Vertices.GetCount();
    const auto firstIndex  = frame->m_Indices.GetCount();
    frame->m_Vertices.PushBackRange(xiiMakeArrayPtr(list->VtxBuffer.Data, static_cast<xiiUInt32>(list->VtxBuffer.Size)));
    frame->m_Indices.PushBackRange(xiiMakeArrayPtr(list->IdxBuffer.Data, static_cast<xiiUInt32>(list->IdxBuffer.Size)));
    for (const auto& command : list->CmdBuffer)
    {
      // Arbitrary user callbacks cannot safely cross the deferred-render boundary. State reset is a no-op:
      // every draw below binds the full backend state, and no foreign commands execute between draws.
      if (command.UserCallback == ImDrawCallback_ResetRenderState)
        continue;
      if (command.UserCallback)
      {
        xiiLog::Error("ImGui deferred rendering does not support user draw callbacks.");
        return XII_FAILURE;
      }
      if (!command.ElemCount)
        continue;
      if (command.IdxOffset > static_cast<xiiUInt32>(list->IdxBuffer.Size) || command.ElemCount > static_cast<xiiUInt32>(list->IdxBuffer.Size) - command.IdxOffset || command.VtxOffset >= static_cast<xiiUInt32>(list->VtxBuffer.Size))
        return XII_FAILURE;
      xiiSharedPtr<xiiImguiTexture> texture;
      for (const auto& entry : m_Textures)
        if (entry.m_Id == command.GetTexID())
        {
          texture = entry.m_pTexture;
          break;
        }
      if (!texture)
      {
        xiiLog::Error("ImGui draw references an unregistered or stale texture token.");
        return XII_FAILURE;
      }
      xiiUInt32 textureIndex = 0;
      while (textureIndex < frame->m_Textures.GetCount() && frame->m_Textures[textureIndex] != texture)
        ++textureIndex;
      if (textureIndex == frame->m_Textures.GetCount())
        frame->m_Textures.PushBack(texture);
      auto& target       = frame->m_Commands.ExpandAndGetRef();
      target.m_vClipRect = {command.ClipRect.x, command.ClipRect.y, command.ClipRect.z, command.ClipRect.w};
      if (!target.m_vClipRect.IsValid())
        return XII_FAILURE;
      target.m_uiFirstIndex = firstIndex + command.IdxOffset;
      target.m_uiBaseVertex = firstVertex + command.VtxOffset;
      target.m_uiIndexCount = command.ElemCount;
      target.m_uiTexture    = textureIndex;
    }
  }
  out_frame = frame;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(DearImguiPlugin, DearImguiPlugin_Implementation_DearImguiContext);
