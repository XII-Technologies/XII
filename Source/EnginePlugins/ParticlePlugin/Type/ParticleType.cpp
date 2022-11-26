#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Type/ParticleType.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleTypeFactory, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleType, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleType* xiiParticleTypeFactory::CreateType(xiiParticleSystemInstance* pOwner) const
{
  const xiiRTTI* pRtti = GetTypeType();

  xiiParticleType* pType = pRtti->GetAllocator()->Allocate<xiiParticleType>();
  pType->Reset(pOwner);

  CopyTypeProperties(pType, true);
  pType->CreateRequiredStreams();

  return pType;
}

xiiParticleType::xiiParticleType()
{
  m_uiLastExtractedFrame = 0;

  // run these as the last, after all the initializers and behaviors
  m_fPriority = +1000.0f;
}

xiiUInt32 xiiParticleType::ComputeSortingKey(xiiParticleTypeRenderMode::Enum mode, xiiUInt32 uiTextureHash)
{
  xiiUInt32 key = 0;

  switch (mode)
  {
    case xiiParticleTypeRenderMode::Additive:
      key = xiiParticleTypeSortingKey::Additive;
      break;

    case xiiParticleTypeRenderMode::Blended:
      key = xiiParticleTypeSortingKey::Blended;
      break;

    case xiiParticleTypeRenderMode::BlendedForeground:
      key = xiiParticleTypeSortingKey::BlendedForeground;
      break;

    case xiiParticleTypeRenderMode::BlendedBackground:
      key = xiiParticleTypeSortingKey::BlendedBackground;
      break;

    case xiiParticleTypeRenderMode::Opaque:
      key = xiiParticleTypeSortingKey::Opaque;
      break;

    case xiiParticleTypeRenderMode::BlendAdd:
      key = xiiParticleTypeSortingKey::BlendAdd;
      break;

    case xiiParticleTypeRenderMode::Distortion:
      key = xiiParticleTypeSortingKey::Distortion;
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  key <<= 32 - 3; // require 3 bits for the values above
  key |= uiTextureHash & 0x1FFFFFFFu;

  return key;
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_ParticleType);
