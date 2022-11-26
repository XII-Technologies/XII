#pragma once

#include <RmlUiPlugin/Resources/RmlUiResource.h>

class xiiRmlUiAssetProperties : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRmlUiAssetProperties, xiiReflectedClass);

public:
  xiiRmlUiAssetProperties();
  ~xiiRmlUiAssetProperties();

  xiiString                  m_sRmlFile;
  xiiEnum<xiiRmlUiScaleMode> m_ScaleMode;
  xiiVec2U32                 m_ReferenceResolution;
};
