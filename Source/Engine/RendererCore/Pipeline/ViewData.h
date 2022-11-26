#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Pipeline/ViewRenderMode.h>
#include <RendererFoundation/Device/SwapChain.h>

/// \brief Holds view data like the viewport, view and projection matrices
struct XII_RENDERERCORE_DLL xiiViewData
{
  xiiViewData()
  {
    m_ViewPortRect   = xiiRectFloat(0.0f, 0.0f);
    m_ViewRenderMode = xiiViewRenderMode::None;

    for (int i = 0; i < 2; ++i)
    {
      m_ViewMatrix[i].SetIdentity();
      m_InverseViewMatrix[i].SetIdentity();
      m_ProjectionMatrix[i].SetIdentity();
      m_InverseProjectionMatrix[i].SetIdentity();
      m_ViewProjectionMatrix[i].SetIdentity();
      m_InverseViewProjectionMatrix[i].SetIdentity();
    }
  }

  xiiGALRenderTargets         m_renderTargets;
  xiiGALSwapChainHandle       m_hSwapChain;
  xiiRectFloat                m_ViewPortRect;
  xiiEnum<xiiViewRenderMode>  m_ViewRenderMode;
  xiiEnum<xiiCameraUsageHint> m_CameraUsageHint;

  // Each matrix is there for both left and right camera lens.
  xiiMat4 m_ViewMatrix[2];
  xiiMat4 m_InverseViewMatrix[2];
  xiiMat4 m_ProjectionMatrix[2];
  xiiMat4 m_InverseProjectionMatrix[2];
  xiiMat4 m_ViewProjectionMatrix[2];
  xiiMat4 m_InverseViewProjectionMatrix[2];

  /// \brief Returns the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fScreenPosX and fScreenPosY are expected to be in [0; 1] range (normalized pixel coordinates).
  /// If no ray can be computed, XII_FAILURE is returned.
  xiiResult ComputePickingRay(
    float        fScreenPosX,
    float        fScreenPosY,
    xiiVec3&     out_RayStartPos,
    xiiVec3&     out_RayDir,
    xiiCameraEye eye = xiiCameraEye::Left) const
  {
    xiiVec3 vScreenPos;
    vScreenPos.x = fScreenPosX;
    vScreenPos.y = 1.0f - fScreenPosY;
    vScreenPos.z = 0.0f;

    return xiiGraphicsUtils::ConvertScreenPosToWorldPos(
      m_InverseViewProjectionMatrix[static_cast<int>(eye)], 0, 0, 1, 1, vScreenPos, out_RayStartPos, &out_RayDir);
  }

  xiiResult ComputeScreenSpacePos(const xiiVec3& vPoint, xiiVec3& out_vScreenPos, xiiCameraEye eye = xiiCameraEye::Left) const
  {
    xiiUInt32 x = (xiiUInt32)m_ViewPortRect.x;
    xiiUInt32 y = (xiiUInt32)m_ViewPortRect.y;
    xiiUInt32 w = (xiiUInt32)m_ViewPortRect.width;
    xiiUInt32 h = (xiiUInt32)m_ViewPortRect.height;

    if (xiiGraphicsUtils::ConvertWorldPosToScreenPos(m_ViewProjectionMatrix[static_cast<int>(eye)], x, y, w, h, vPoint, out_vScreenPos).Succeeded())
    {
      out_vScreenPos.y = m_ViewPortRect.height - out_vScreenPos.y;

      return XII_SUCCESS;
    }

    return XII_FAILURE;
  }
};
