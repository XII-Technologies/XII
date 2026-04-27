#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Components/Camera/CinematicCameraComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiFilmFormat, 1)
  XII_ENUM_CONSTANTS(xiiFilmFormat::FullFrame35mm, xiiFilmFormat::Super35, xiiFilmFormat::AnamorphicScope,
                     xiiFilmFormat::IMAX70mm, xiiFilmFormat::Custom)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCinematicCameraRenderData, 1, xiiRTTIDefaultAllocator<xiiCinematicCameraRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiCinematicCameraComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("FocalLength", GetFocalLength, SetFocalLength)->AddAttributes(new xiiDefaultValueAttribute(50.0f), new xiiClampValueAttribute(1.0f, 2000.0f)),
    XII_ACCESSOR_PROPERTY("Aperture", GetAperture, SetAperture)->AddAttributes(new xiiDefaultValueAttribute(2.8f), new xiiClampValueAttribute(0.5f, 64.0f)),
    XII_ACCESSOR_PROPERTY("FocusDist", GetFocusDist, SetFocusDist)->AddAttributes(new xiiDefaultValueAttribute(5.0f), new xiiClampValueAttribute(0.01f, xiiVariant())),
    XII_ENUM_ACCESSOR_PROPERTY("FilmFormat", xiiFilmFormat, GetFilmFormat, SetFilmFormat),
    XII_ACCESSOR_PROPERTY("EnableDoF", GetEnableDoF, SetEnableDoF),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering/Camera"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiCinematicCameraComponent::xiiCinematicCameraComponent()  = default;
xiiCinematicCameraComponent::~xiiCinematicCameraComponent() = default;

void xiiCinematicCameraComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s << m_fFocalLength << m_fAperture << m_fFocusDist;
  s << m_fSensorWidth << m_fSensorHeight;
  s << m_FilmFormat.GetValue();
  s << m_bEnableDoF;
}

void xiiCinematicCameraComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s >> m_fFocalLength >> m_fAperture >> m_fFocusDist;
  s >> m_fSensorWidth >> m_fSensorHeight;
  xiiUInt8 fmt = 0;
  s >> fmt;
  m_FilmFormat = static_cast<xiiFilmFormat::Enum>(fmt);
  s >> m_bEnableDoF;
}

void xiiCinematicCameraComponent::SetFocalLength(float fMM) { m_fFocalLength = xiiMath::Clamp(fMM, 1.0f, 2000.0f); }
void xiiCinematicCameraComponent::SetAperture(float fStop) { m_fAperture = xiiMath::Clamp(fStop, 0.5f, 64.0f); }
void xiiCinematicCameraComponent::SetFocusDist(float fMetres) { m_fFocusDist = xiiMath::Max(fMetres, 0.01f); }

void xiiCinematicCameraComponent::SetFilmFormat(xiiEnum<xiiFilmFormat> fmt)
{
  m_FilmFormat = fmt;
  // Update sensor dimensions for presets
  switch (fmt)
  {
    case xiiFilmFormat::FullFrame35mm:
      m_fSensorWidth  = 36.0f;
      m_fSensorHeight = 24.0f;
      break;
    case xiiFilmFormat::Super35:
      m_fSensorWidth  = 24.89f;
      m_fSensorHeight = 18.67f;
      break;
    case xiiFilmFormat::AnamorphicScope:
      m_fSensorWidth  = 21.95f;
      m_fSensorHeight = 18.59f;
      break;
    case xiiFilmFormat::IMAX70mm:
      m_fSensorWidth  = 70.41f;
      m_fSensorHeight = 52.63f;
      break;
    default: break;
  }
}

void xiiCinematicCameraComponent::SetEnableDoF(bool b) { m_bEnableDoF = b; }

xiiAngle xiiCinematicCameraComponent::ComputeHFOV() const
{
  return xiiAngle::MakeFromRadian(2.0f * xiiMath::ATan(m_fSensorWidth * 0.5f / m_fFocalLength));
}

void xiiCinematicCameraComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_pView == nullptr || ref_msg.m_pExtractedRenderData == nullptr)
    return;

  auto pWorldModule = GetWorld()->GetModule<xiiRenderWorldModule>();
  if (pWorldModule == nullptr)
    return;

  xiiCinematicCameraRenderData* pRenderData = pWorldModule->CreateRenderDataForThisFrame<xiiCinematicCameraRenderData>(this);
  pRenderData->m_GlobalTransform            = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalBounds               = GetOwner()->GetGlobalBounds();
  pRenderData->m_hOwnerObject               = GetOwner()->GetHandle();
  pRenderData->m_hOwnerComponent            = GetHandle();
  pRenderData->m_fFocalLength               = m_fFocalLength;
  pRenderData->m_fAperture                  = m_fAperture;
  pRenderData->m_fFocusDist                 = m_fFocusDist;
  pRenderData->m_fSensorWidth               = m_fSensorWidth;
  pRenderData->m_fSensorHeight              = m_fSensorHeight;
  pRenderData->m_FilmFormat                 = m_FilmFormat;
  pRenderData->m_bEnableDoF                 = m_bEnableDoF;
  pRenderData->m_uiSortingKey               = GetUniqueIdForRendering(0);

  ref_msg.AddRenderData(pRenderData, xiiRenderData::Caching::Never);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Camera_Implementation_CinematicCameraComponent);
