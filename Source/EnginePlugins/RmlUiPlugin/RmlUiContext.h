#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

class xiiRenderData;
class xiiBlackboard;

namespace xiiRmlUiInternal
{
  class Extractor;
  class EventListener;
} // namespace xiiRmlUiInternal

class XII_RMLUIPLUGIN_DLL xiiRmlUiContext final : public Rml::Context
{
public:
  xiiRmlUiContext(const Rml::String& name);
  ~xiiRmlUiContext();

public:
  xiiResult LoadDocumentFromResource(const xiiRmlUiResourceHandle& hResource);
  xiiResult LoadDocumentFromString(const xiiStringView& sContent);

  void      UnloadDocument();
  xiiResult ReloadDocumentFromResource(const xiiRmlUiResourceHandle& hResource);

  void ShowDocument();
  void HideDocument();

  void UpdateInput(const xiiVec2& mousePos);
  bool WantsInput() const { return m_bWantsInput; }

  void SetOffset(const xiiVec2I32& offset);
  void SetSize(const xiiVec2U32& size);
  void SetDpiScale(float fScale);

  using EventHandler = xiiDelegate<void(Rml::Event&)>;

  void RegisterEventHandler(const char* szIdentifier, EventHandler handler);
  void DeregisterEventHandler(const char* szIdentifier);

private:
  bool HasDocument() { return GetNumDocuments() > 0; }

  friend class xiiRmlUi;
  void ExtractRenderData(xiiRmlUiInternal::Extractor& extractor);

  friend class xiiRmlUiInternal::EventListener;
  void ProcessEvent(const xiiHashedString& sIdentifier, Rml::Event& event);

  xiiVec2I32 m_vOffset = xiiVec2I32::ZeroVector();

  xiiHashTable<xiiHashedString, EventHandler> m_EventHandler;

  xiiUInt64      m_uiExtractedFrame = 0;
  xiiRenderData* m_pRenderData      = nullptr;

  bool m_bWantsInput = false;
};

namespace xiiRmlUiInternal
{
  class ContextInstancer : public Rml::ContextInstancer
  {
  public:
    virtual Rml::ContextPtr InstanceContext(const Rml::String& name) override;
    virtual void            ReleaseContext(Rml::Context* context) override;

  private:
    virtual void Release() override;
  };
} // namespace xiiRmlUiInternal
