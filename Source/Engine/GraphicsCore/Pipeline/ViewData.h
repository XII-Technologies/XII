/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsFoundation/Device/SwapChain.h>

/// Holds view data like the viewport, view and projection matrices
struct XII_GRAPHICSCORE_DLL xiiViewData
{
  xiiViewData()
  {
    m_ViewPortRect             = xiiRectFloat(0.0f, 0.0f);
    m_ViewRenderMode           = xiiViewRenderMode::None;
    m_fRenderResolutionScale   = 1.0f;
    m_uiRenderResolutionWidth  = 1U;
    m_uiRenderResolutionHeight = 1U;

    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      m_ViewMatrix[i].SetIdentity();
      m_InverseViewMatrix[i].SetIdentity();
      m_ProjectionMatrix[i].SetIdentity();
      m_InverseProjectionMatrix[i].SetIdentity();
      m_ViewProjectionMatrix[i].SetIdentity();
      m_InverseViewProjectionMatrix[i].SetIdentity();
    }
  }

  xiiRectFloat                m_ViewPortRect;
  xiiEnum<xiiViewRenderMode>  m_ViewRenderMode;
  xiiEnum<xiiCameraUsageHint> m_CameraUsageHint;

  /// Dynamic internal render scale applied to this view (1.0 = native viewport resolution).
  float m_fRenderResolutionScale;

  /// Dynamic internal render resolution in pixels.
  xiiUInt32 m_uiRenderResolutionWidth;
  xiiUInt32 m_uiRenderResolutionHeight;

  // Each matrix is there for both left and right camera lens.
  xiiMat4 m_ViewMatrix[2];
  xiiMat4 m_InverseViewMatrix[2];
  xiiMat4 m_ProjectionMatrix[2];
  xiiMat4 m_InverseProjectionMatrix[2];
  xiiMat4 m_ViewProjectionMatrix[2];
  xiiMat4 m_InverseViewProjectionMatrix[2];

  /// Calculates the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fNormalizedScreenPosX and fNormalizedScreenPosY are expected to be in [0; 1] range (normalized screen coordinates).
  /// If no ray can be computed, XII_FAILURE is returned.
  XII_ALWAYS_INLINE xiiResult ComputePickingRay(float fNormalizedScreenPosX, float fNormalizedScreenPosY, xiiVec3& out_vRayStartPos, xiiVec3& out_vRayDir, xiiCameraEye eye = xiiCameraEye::Left) const
  {
    xiiVec3 vScreenPos;
    vScreenPos.x = fNormalizedScreenPosX;
    vScreenPos.y = fNormalizedScreenPosY;
    vScreenPos.z = 0.0f;

    return xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseViewProjectionMatrix[static_cast<xiiUInt32>(eye)], vScreenPos, out_vRayStartPos, &out_vRayDir);
  }

  /// Calculates the normalized screen-space coordinate ([0; 1] range) that the given world-space point projects to.
  ///
  /// Returns XII_FAILURE, if the point could not be projected into screen-space.
  XII_ALWAYS_INLINE xiiResult ComputeScreenSpacePos(const xiiVec3& vWorldPos, xiiVec3& out_vScreenPosNormalized, xiiCameraEye eye = xiiCameraEye::Left) const
  {
    return xiiGraphicsUtils::ConvertWorldPosToScreenPos(m_ViewProjectionMatrix[static_cast<xiiUInt32>(eye)], vWorldPos, out_vScreenPosNormalized);
  }

  /// Calculates the world-space position that the given normalized screen-space coordinate maps to
  XII_ALWAYS_INLINE xiiResult ComputeWorldSpacePos(float fNormalizedScreenPosX, float fNormalizedScreenPosY, xiiVec3& out_vWorldPos, xiiCameraEye eye = xiiCameraEye::Left) const
  {
    return xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseViewProjectionMatrix[static_cast<xiiUInt32>(eye)], xiiVec3(fNormalizedScreenPosX, fNormalizedScreenPosY, 0.0f), out_vWorldPos);
  }

  /// Converts a screen-space position from pixel coordinates to normalized coordinates.
  XII_ALWAYS_INLINE void ConvertScreenPixelPosToNormalizedPos(xiiVec3& inout_vPixelPos) const
  {
    xiiUInt32 x = (xiiUInt32)m_ViewPortRect.x;
    xiiUInt32 y = (xiiUInt32)m_ViewPortRect.y;
    xiiUInt32 w = (xiiUInt32)m_ViewPortRect.width;
    xiiUInt32 h = (xiiUInt32)m_ViewPortRect.height;
    xiiGraphicsUtils::ConvertScreenPixelPosToNormalizedPos(x, y, w, h, inout_vPixelPos);
  }

  /// Converts a screen-space position from normalized coordinates to pixel coordinates.
  XII_ALWAYS_INLINE void ConvertScreenNormalizedPosToPixelPos(xiiVec3& inout_vNormalizedPos) const
  {
    {
      xiiUInt32 x = (xiiUInt32)m_ViewPortRect.x;
      xiiUInt32 y = (xiiUInt32)m_ViewPortRect.y;
      xiiUInt32 w = (xiiUInt32)m_ViewPortRect.width;
      xiiUInt32 h = (xiiUInt32)m_ViewPortRect.height;
      xiiGraphicsUtils::ConvertScreenNormalizedPosToPixelPos(x, y, w, h, inout_vNormalizedPos);
    }
  }
};
