/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Shaders/Common/GlobalConstants.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiMaterialConstants)
{
  COLOR4F(BaseColor);
  COLOR4F(EmissiveColor);
  FLOAT1(MetallicValue);
  FLOAT1(ReflectanceValue);
  FLOAT1(RoughnessValue);
  FLOAT1(MaskThreshold);
  BOOL1(UseBaseTexture);
  BOOL1(UseNormalTexture);
  BOOL1(UseRoughnessTexture);
  BOOL1(UseMetallicTexture);
  BOOL1(UseEmissiveTexture);
  BOOL1(UseOcclusionTexture);
  BOOL1(UseOrmTexture);
};
