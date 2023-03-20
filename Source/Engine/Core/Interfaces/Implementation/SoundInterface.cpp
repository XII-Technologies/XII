#include <Core/CorePCH.h>

#include <Core/Interfaces/SoundInterface.h>
#include <Foundation/Configuration/Singleton.h>

xiiResult xiiSoundInterface::PlaySound(xiiStringView sResourceID, const xiiTransform& globalPosition, float fPitch /*= 1.0f*/, float fVolume /*= 1.0f*/, bool bBlockIfNotLoaded /*= true*/)
{
  if (xiiSoundInterface* pSoundInterface = xiiSingletonRegistry::GetSingletonInstance<xiiSoundInterface>())
  {
    return pSoundInterface->OneShotSound(sResourceID, globalPosition, fPitch, fVolume, bBlockIfNotLoaded);
  }

  return XII_FAILURE;
}
