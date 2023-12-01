#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Resources/TextureView.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALGraphicsUtilities
{
public:
  static bool IsIdentityComponentMapping(const xiiGALTextureComponentMapping& mapping);
};

#include <GraphicsFoundation/Utilities/Implementation/GraphicsUtilities_inl.h>
