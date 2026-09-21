/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

#include <Core/World/CoordinateSystem.h>
#include <Core/World/SpatialSystem.h>

class xiiTimeStepSmoothing;

/// Describes the initial state of a world.
struct xiiWorldDescription
{
  XII_DECLARE_POD_TYPE();

  xiiWorldDescription(xiiStringView sWorldName) { m_sName.Assign(sWorldName); }

  xiiHashedString m_sName;
  xiiUInt64       m_uiRandomNumberGeneratorSeed = 0;

  xiiUniquePtr<xiiSpatialSystem> m_pSpatialSystem;
  bool                           m_bAutoCreateSpatialSystem = true; ///< automatically create a default spatial system if none is set

  xiiSharedPtr<xiiCoordinateSystemProvider> m_pCoordinateSystemProvider;
  xiiUniquePtr<xiiTimeStepSmoothing>        m_pTimeStepSmoothing; ///< if nullptr, xiiDefaultTimeStepSmoothing will be used

  bool m_bReportErrorWhenStaticObjectMoves = true;

  xiiTime m_MaxComponentInitializationTimePerFrame = xiiTime::MakeFromHours(10000); // max time to spend on component initialization per frame
};
