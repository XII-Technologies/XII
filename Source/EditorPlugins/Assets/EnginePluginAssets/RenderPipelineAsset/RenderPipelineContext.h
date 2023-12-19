#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>

struct xiiRenderPipelineContextLoaderConnection
{
  xiiUuid   m_Source;
  xiiUuid   m_Target;
  xiiString m_SourcePin;
  xiiString m_TargetPin;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiRenderPipelineContextLoaderConnection);


class xiiRenderPipelineRttiConverterContext : public xiiWorldRttiConverterContext
{
public:
  const xiiRTTI* FindTypeByName(xiiStringView sName) const override;
};

class XII_ENGINEPLUGINASSETS_DLL xiiRenderPipelineContext : public xiiEngineProcessDocumentContext
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRenderPipelineContext, xiiEngineProcessDocumentContext);

public:
  xiiRenderPipelineContext();

  virtual void HandleMessage(const xiiEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual xiiEngineProcessViewContext* CreateViewContext() override;
  virtual void                         DestroyViewContext(xiiEngineProcessViewContext* pContext) override;

  virtual xiiStatus ExportDocument(const xiiExportDocumentMsgToEngine* pMsg) override;

  virtual xiiWorldRttiConverterContext&       GetContext() override;
  virtual const xiiWorldRttiConverterContext& GetContext() const override;

private:
  xiiRenderPipelineRttiConverterContext m_RenderPipelineContext;
};
