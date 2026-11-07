/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <Shaders/Common/ShaderResourceMacros.h>

DECLARE_CONSTANT_BUFFER_AUTO(xiiDDGIWorldTraceConstants)
{
  UINT4(ProbeSceneLayout); ///< Geometry/material frame bases, packed vertex stride and UV offset.
};
