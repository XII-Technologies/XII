#pragma once

#include "../Common/ShaderResourceMacros.h"

DECLARE_CONSTANT_BUFFER_AUTO(xiiGizmoConstants)
{
  MAT4(ObjectToWorldMatrix);
  MAT4(WorldToObjectMatrix);
  COLOR4F(GizmoColor);
  FLOAT1(GizmoScale);
  INT1(GameObjectID);
};
