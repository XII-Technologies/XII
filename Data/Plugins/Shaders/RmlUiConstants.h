#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(xiiRmlUiConstants, 4)
{
  MAT4(UiTransform);
  FLOAT4(UiTranslation);
  FLOAT4(QuadVertexPos)[4];
};
