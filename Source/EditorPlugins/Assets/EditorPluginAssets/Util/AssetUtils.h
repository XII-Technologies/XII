/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

struct xiiMaterialResourceSlot
{
  xiiString m_sLabel;
  xiiString m_sResource;
  bool      m_bHighlight = false;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiMaterialResourceSlot);
