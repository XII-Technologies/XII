/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>
DECLARE_CONSTANT_BUFFER_AUTO(xiiGeometryLodFeedbackConstants)
{
  UINT1(PriorityTier);
  UINT1(PurposeMask);
  UINT2(GeometryFeedbackPadding);
};
