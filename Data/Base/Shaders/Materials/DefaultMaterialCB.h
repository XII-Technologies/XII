#include <Shaders/Common/GlobalConstants.h>

DECLARE_CONSTANT_BUFFER(xiiMaterialConstants, 1, 0)
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
