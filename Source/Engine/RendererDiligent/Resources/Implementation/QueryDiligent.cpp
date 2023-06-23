#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Device/DeviceDiligent.h>
#include <RendererDiligent/Resources/QueryDiligent.h>

xiiGALQueryDiligent::xiiGALQueryDiligent(const xiiGALQueryCreationDescription& Description) :
  xiiGALQuery(Description), m_pQuery(nullptr)
{
}

xiiGALQueryDiligent::~xiiGALQueryDiligent() {}

xiiResult xiiGALQueryDiligent::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  Diligent::QueryDesc queryDesc;
  queryDesc.Name = m_Description.m_szName;

  switch (m_Description.m_Type)
  {
    case xiiGALQueryType::Occlusion:
      queryDesc.Type = Diligent::QUERY_TYPE_OCCLUSION;
      break;
    case xiiGALQueryType::BinaryOcclusion:
      queryDesc.Type = Diligent::QUERY_TYPE_BINARY_OCCLUSION;
      break;
    case xiiGALQueryType::Timestamp:
      queryDesc.Type = Diligent::QUERY_TYPE_TIMESTAMP;
      break;
    case xiiGALQueryType::PipelineStatistics:
      queryDesc.Type = Diligent::QUERY_TYPE_PIPELINE_STATISTICS;
      break;
    case xiiGALQueryType::Duration:
      queryDesc.Type = Diligent::QUERY_TYPE_DURATION;
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  pDeviceDiligent->GetDevice()->CreateQuery(queryDesc, &m_pQuery);

  return (m_pQuery != nullptr) ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiGALQueryDiligent::DeInitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceDiligent* pDeviceDiligent = static_cast<xiiGALDeviceDiligent*>(pDevice);

  XII_GAL_DILIGENT_WRAPPED_RELEASE(m_pQuery);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(RendererDiligent, RendererDiligent_Resources_Implementation_QueryDiligent);
