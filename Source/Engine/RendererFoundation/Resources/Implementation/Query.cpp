#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Query.h>

xiiGALQuery::xiiGALQuery(const xiiGALQueryCreationDescription& Description) :
  xiiGALResource<xiiGALQueryCreationDescription>(Description), m_bStarted(false)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(Description.m_szName);
#endif
}

xiiGALQuery::~xiiGALQuery() {}

XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Query);
