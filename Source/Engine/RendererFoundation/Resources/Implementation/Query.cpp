#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Query.h>

xiiGALQuery::xiiGALQuery(const xiiGALQueryCreationDescription& Description) :
  xiiGALResource<xiiGALQueryCreationDescription>(Description), m_bStarted(false)
{
}

xiiGALQuery::~xiiGALQuery() {}

XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Query);
