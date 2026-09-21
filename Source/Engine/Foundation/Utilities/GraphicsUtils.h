/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Mat4.h>

namespace xiiGraphicsUtils
{
  /// Converts a screen-space position from pixel coordinates to normalized coordinates.
  XII_FOUNDATION_DLL void ConvertScreenPixelPosToNormalizedPos(const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, xiiVec3& inout_vPixelPos);

  /// Converts a screen-space position from normalized coordinates to pixel coordinates.
  XII_FOUNDATION_DLL void ConvertScreenNormalizedPosToPixelPos(const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, xiiVec3& inout_vNormalizedPos);

  /// Projects the given point from 3D world space into screen space, if possible.
  ///
  /// \param ModelViewProjection
  ///   The Model-View-Projection matrix that is used by the camera.
  /// \param DepthRange
  ///   The depth range that is used by this projection matrix. \see xiiClipSpaceDepthRange
  ///
  /// Returns XII_FAILURE, if the point could not be projected into screen space.
  /// \note The function reports XII_SUCCESS, when the point could be projected, however, that does not mean that the point actually lies
  /// within the viewport, it might still be outside the viewport.
  ///
  /// out_vScreenPos.z is the depth of the point in [0;1] range. The z value is always 'normalized' to this range
  /// (as long as the DepthRange parameter is correct), to make it easier to make subsequent code platform independent.
  XII_FOUNDATION_DLL xiiResult ConvertWorldPosToScreenPos(const xiiMat4& mModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3& vPoint, xiiVec3& out_vScreenPos, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// Overload of ConvertWorldPosToScreenPos() that returns the screen position in normalized space ([0; 1] range) and therefore doesn't require the viewport dimensions.
  XII_FOUNDATION_DLL xiiResult ConvertWorldPosToScreenPos(const xiiMat4& mModelViewProjection, const xiiVec3& vPoint, xiiVec3& out_vScreenPosNormalized, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// Takes the screen space position (including depth in [0;1] range) and converts it into a world space position.
  ///
  /// \param InverseModelViewProjection
  ///   The inverse of the Model-View-Projection matrix that is used by the camera.
  /// \param DepthRange
  ///   The depth range that is used by this projection matrix. \see xiiClipSpaceDepthRange
  ///
  /// Returns XII_FAILURE when the screen coordinate could not be converted to a world position,
  /// which should generally not be possible as long as the coordinate is actually inside the viewport.
  ///
  /// Optionally this function also computes the direction vector through the world space position, that should be used for picking
  /// operations. Note that for perspective cameras this is the same as the direction from the camera position to the computed point,
  /// but for orthographic cameras it is not (it's simply the forward vector of the camera).
  /// This function handles both cases properly.
  ///
  /// The z value of vScreenPixelPos is always expected to be in [0; 1] range (meaning 0 is at the near plane, 1 at the far plane),
  /// even on platforms that use [-1; +1] range for clip-space z values. The DepthRange parameter needs to be correct to handle this case
  /// properly.
  ///
  /// vScreenPixelPos is expected to be in range [viewport x/y; viewport width/height]. There is an overload below that takes just a normalized value
  /// in range [0; 1].
  XII_FOUNDATION_DLL xiiResult ConvertScreenPosToWorldPos(const xiiMat4& mInverseModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3& vScreenPixelPos, xiiVec3& out_vPoint, xiiVec3* out_pDirection = nullptr, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// Overload of ConvertScreenPosToWorldPos() that takes the coordinate in normalized space ([0; 1]) and therefore doesn't require the viewport dimensions.
  XII_FOUNDATION_DLL xiiResult ConvertScreenPosToWorldPos(const xiiMat4& mInverseModelViewProjection, const xiiVec3& vNormalizedScreenPos, xiiVec3& out_vPoint, xiiVec3* out_pDirection = nullptr, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// A double-precision version of ConvertScreenPosToWorldPos()
  XII_FOUNDATION_DLL xiiResult ConvertScreenPosToWorldPos(const xiiMat4d& mInverseModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3& vScreenPixelPos, xiiVec3& out_vPoint, xiiVec3* out_pDirection = nullptr, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// Double-precision overload of ConvertScreenPosToWorldPos() that takes the coordinate in normalized space ([0; 1]) and therefore doesn't require the viewport dimensions.
  XII_FOUNDATION_DLL xiiResult ConvertScreenPosToWorldPos(const xiiMat4d& mInverseModelViewProjection, const xiiVec3& vNormalizedScreenPos, xiiVec3& out_vPoint, xiiVec3* out_pDirection = nullptr, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// Checks whether the given transformation matrix would change the winding order of a triangle's vertices and thus requires that
  /// the vertex order gets reversed to compensate.
  XII_FOUNDATION_DLL bool IsTriangleFlipRequired(const xiiMat3& mTransformation);

  /// Converts a projection or view-projection matrix from one depth-range convention to another
  XII_FOUNDATION_DLL void ConvertProjectionMatrixDepthRange(xiiMat4& inout_mMatrix, xiiClipSpaceDepthRange::Enum srcDepthRange, xiiClipSpaceDepthRange::Enum dstDepthRange); // [tested]

  /// Retrieves the horizontal and vertical field-of-view angles from the perspective matrix.
  ///
  /// \note If an orthographic projection matrix is passed in, the returned angle values will be zero.
  XII_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const xiiMat4& mProjectionMatrix, xiiAngle& out_fovX, xiiAngle& out_fovY); // [tested]

  /// Extracts the field of view angles from a perspective matrix.
  /// \param ProjectionMatrix Perspective projection matrix to be decomposed.
  /// \param out_fFovLeft Left angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovRight Right angle of the frustum.
  /// \param out_fFovBottom Bottom angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovTop Top angle of the frustum.
  /// \param yRange The Y range used to construct the perspective matrix.
  XII_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const xiiMat4& mProjectionMatrix, xiiAngle& out_fovLeft, xiiAngle& out_fovRight, xiiAngle& out_fovBottom, xiiAngle& out_fovTop, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular); // [tested]

  /// Extracts the field of view distances on the near plane from a perspective matrix.
  ///
  /// Convenience function that also extracts near / far values and returns the distances on the near plane to be the inverse of xiiGraphicsUtils::CreatePerspectiveProjectionMatrix.
  /// \sa xiiGraphicsUtils::CreatePerspectiveProjectionMatrix
  XII_FOUNDATION_DLL xiiResult ExtractPerspectiveMatrixFieldOfView(const xiiMat4& mProjectionMatrix, float& out_fLeft, float& out_fRight, float& out_fBottom, float& out_fTop, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular); // [tested]

  /// Computes the distances of the near and far clip planes from the given perspective projection matrix.
  ///
  /// Returns XII_FAILURE when one of the values could not be computed, because it would result in a "division by zero".
  XII_FOUNDATION_DLL xiiResult ExtractNearAndFarClipPlaneDistances(float& out_fNear, float& out_fFar, const xiiMat4& mProjectionMatrix, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]


  enum class FrustumPlaneInterpolation
  {
    LeftToRight,
    BottomToTop,
    NearToFar,
  };

  /// Computes an interpolated frustum plane by using linear interpolation in normalized clip space.
  ///
  /// Along left/right, up/down this makes it easy to create a regular grid of planes.
  /// Along near/far creating planes at regular intervals will result in planes in world-space that represent
  /// the same amount of depth-precision.
  ///
  /// \param dir Specifies which planes to interpolate.
  /// \param fLerpFactor The interpolation coefficient (usually in the interval [0;1]).
  XII_FOUNDATION_DLL xiiPlane ComputeInterpolatedFrustumPlane(FrustumPlaneInterpolation dir, float fLerpFactor, const xiiMat4& mProjectionMatrix, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// Creates a perspective projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  XII_FOUNDATION_DLL xiiMat4 CreatePerspectiveProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// Creates a perspective projection matrix.
  XII_FOUNDATION_DLL xiiMat4 CreatePerspectiveProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// Creates a perspective projection matrix.
  /// \param fFieldOfViewX    Horizontal field of view.
  XII_FOUNDATION_DLL xiiMat4 CreatePerspectiveProjectionMatrixFromFovX(xiiAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// Creates a perspective projection matrix.
  /// \param fFieldOfViewY    Vertical field of view.
  XII_FOUNDATION_DLL xiiMat4 CreatePerspectiveProjectionMatrixFromFovY(xiiAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// Creates an orthographic projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  XII_FOUNDATION_DLL xiiMat4 CreateOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// Creates an orthographic projection matrix.
  XII_FOUNDATION_DLL xiiMat4 CreateOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// Returns a look-at matrix (only direction, no translation).
  ///
  /// Since this only creates a rotation matrix, vTarget can be interpreted both as a position or a direction.
  XII_FOUNDATION_DLL xiiMat3 CreateLookAtViewMatrix(const xiiVec3& vTarget, const xiiVec3& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// Same as CreateLookAtViewMatrix() but returns the inverse matrix
  XII_FOUNDATION_DLL xiiMat3 CreateInverseLookAtViewMatrix(const xiiVec3& vTarget, const xiiVec3& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// Returns a look-at matrix with both rotation and translation
  XII_FOUNDATION_DLL xiiMat4 CreateLookAtViewMatrix(const xiiVec3& vEyePos, const xiiVec3& vLookAtPos, const xiiVec3& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// Same as CreateLookAtViewMatrix() but returns the inverse matrix
  XII_FOUNDATION_DLL xiiMat4 CreateInverseLookAtViewMatrix(const xiiVec3& vEyePos, const xiiVec3& vLookAtPos, const xiiVec3& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// Creates a view matrix from the given camera vectors.
  ///
  /// The vectors are put into the appropriate matrix rows and depending on the handedness negated where necessary.
  XII_FOUNDATION_DLL xiiMat4 CreateViewMatrix(const xiiVec3& vPosition, const xiiVec3& vForwardDir, const xiiVec3& vRightDir, const xiiVec3& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// Similar to CreateViewMatrix() but creates the inverse matrix.
  XII_FOUNDATION_DLL xiiMat4 CreateInverseViewMatrix(const xiiVec3& vPosition, const xiiVec3& vForwardDir, const xiiVec3& vRightDir, const xiiVec3& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// Extracts the forward, right and up dir and camera position from the given view matrix.
  ///
  /// The handedness should be the same as used in CreateViewMatrix() or CreateLookAtViewMatrix().
  XII_FOUNDATION_DLL void DecomposeViewMatrix(xiiVec3& out_vPosition, xiiVec3& out_vForwardDir, xiiVec3& out_vRightDir, xiiVec3& out_vUpDir, const xiiMat4& mViewMatrix, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// Computes the barycentric coordinates of a point in a 3D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns XII_FAILURE.
  XII_FOUNDATION_DLL xiiResult ComputeBarycentricCoordinates(xiiVec3& out_vCoordinates, const xiiVec3& v0, const xiiVec3& v1, const xiiVec3& v2, const xiiVec3& vPos);

  /// Computes the barycentric coordinates of a point in a 2D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns XII_FAILURE.
  XII_FOUNDATION_DLL xiiResult ComputeBarycentricCoordinates(xiiVec3& out_vCoordinates, const xiiVec2& v0, const xiiVec2& v1, const xiiVec2& v2, const xiiVec2& vPos);

  /// Returns a coverage value of how much space a sphere at a given location would take up on screen using a perspective projection.
  ///
  /// The coverage value is close to 0 for very small or far away spheres and approaches 1 when the projected sphere would take up the entire screen.
  /// The calculation is resolution independent and also doesn't take into account whether the sphere is inside the view frustum at all.
  /// Thus the value doesn't change depending on camera view direction, it only depends on distance and the camera's field-of-view.
  /// Values (much) larger than 1 are possible.
  ///
  /// \note Only one camera FOV angle is used for the calculation, pass in either the horizontal or vertical FOV angle,
  /// depending on what is most relevant to you.
  /// Typically the 'fixed' angle is used (usually the vertical one) since the other one depends on the window size.
  inline float CalculateSphereScreenCoverage(const xiiBoundingSphere& sphere, const xiiVec3& vCameraPosition, xiiAngle perspectiveCameraFov)
  {
    const float fDist       = (sphere.m_vCenter - vCameraPosition).GetLength();
    const float fHalfHeight = xiiMath::Tan(perspectiveCameraFov * 0.5f) * fDist;
    return sphere.m_fRadius / fHalfHeight;
  }

  /// Returns a coverage value of how much space a sphere of a given size would take up on screen using an orthographic projection.
  ///
  /// The coverage value is close to 0 for very small spheres and approaches 1 when the projected sphere would take up the entire screen.
  /// The calculation is resolution independent and also doesn't take into account whether the sphere is inside the view frustum at all.
  /// Thus the value doesn't change depending on camera view direction. In orthographic projections even the distance to the camera is irrelevant,
  /// only the dimensions of the ortho camera are needed.
  /// Values (much) larger than 1 are possible.
  ///
  /// \note Only one camera dimension is used for the calculation, pass in either the X or Y dimension, depending on what is most relevant to you.
  /// Typically the 'fixed' dimension is used (usually Y) since the other one depends on the window size.
  inline float CalculateSphereScreenCoverage(float fSphereRadius, float fOrthoCameraDimensions)
  {
    const float fHalfHeight = fOrthoCameraDimensions * 0.5f;
    return fSphereRadius / fHalfHeight;
  }

} // namespace xiiGraphicsUtils
