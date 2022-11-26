#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/IdTable.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

class xiiOpenXR;


class XII_OPENXRPLUGIN_DLL xiiOpenXRSpatialAnchors : public xiiXRSpatialAnchorsInterface
{
  XII_DECLARE_SINGLETON_OF_INTERFACE(xiiOpenXRSpatialAnchors, xiiXRSpatialAnchorsInterface);

public:
  xiiOpenXRSpatialAnchors(xiiOpenXR* pOpenXR);
  ~xiiOpenXRSpatialAnchors();

  xiiXRSpatialAnchorID CreateAnchor(const xiiTransform& globalTransform) override;
  xiiResult            DestroyAnchor(xiiXRSpatialAnchorID id) override;
  xiiResult            TryGetAnchorTransform(xiiXRSpatialAnchorID id, xiiTransform& out_globalTransform) override;

private:
  friend class xiiOpenXR;
  struct AnchorData
  {
    XII_DECLARE_POD_TYPE();
    XrSpatialAnchorMSFT m_Anchor;
    XrSpace             m_Space;
  };

  xiiOpenXR* m_pOpenXR = nullptr;

  xiiIdTable<xiiXRSpatialAnchorID, AnchorData> m_Anchors;
};
