#pragma once

#include <Foundation/Math/Mat4.h>

namespace xiiGraphicsUtils
{
  /// \brief Projects the given point from 3D world space into screen space, if possible.
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
  XII_FOUNDATION_DLL xiiResult ConvertWorldPosToScreenPos(const xiiMat4& ModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3& vPoint, xiiVec3& out_vScreenPos,
                                                          xiiClipSpaceDepthRange::Enum DepthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// \brief Takes the screen space position (including depth in [0;1] range) and converts it into a world space position.
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
  /// The z value of vScreenPos is always expected to be in [0; 1] range (meaning 0 is at the near plane, 1 at the far plane),
  /// even on platforms that use [-1; +1] range for clip-space z values. The DepthRange parameter needs to be correct to handle this case
  /// properly.
  XII_FOUNDATION_DLL xiiResult ConvertScreenPosToWorldPos(const xiiMat4& InverseModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3& vScreenPos, xiiVec3& out_vPoint, xiiVec3* out_vDirection = nullptr,
                                                          xiiClipSpaceDepthRange::Enum DepthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// \brief A double-precision version of ConvertScreenPosToWorldPos()
  XII_FOUNDATION_DLL xiiResult ConvertScreenPosToWorldPos(const xiiMat4d& InverseModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3& vScreenPos, xiiVec3& out_vPoint, xiiVec3* out_vDirection = nullptr,
                                                          xiiClipSpaceDepthRange::Enum DepthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// \brief Checks whether the given transformation matrix would change the winding order of a triangle's vertices and thus requires that
  /// the vertex order gets reversed to compensate.
  XII_FOUNDATION_DLL bool IsTriangleFlipRequired(const xiiMat3& mTransformation);

  /// \brief Converts a projection or view-projection matrix from one depth-range convention to another
  XII_FOUNDATION_DLL void ConvertProjectionMatrixDepthRange(
    xiiMat4&                     inout_Matrix,
    xiiClipSpaceDepthRange::Enum SrcDepthRange,
    xiiClipSpaceDepthRange::Enum DstDepthRange); // [tested]

  /// \brief Retrieves the horizontal and vertical field-of-view angles from the perspective matrix.
  ///
  /// \note If an orthographic projection matrix is passed in, the returned angle values will be zero.
  XII_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const xiiMat4& ProjectionMatrix, xiiAngle& out_fFovX, xiiAngle& out_fFovY); // [tested]

  /// \brief Extracts the field of view angles from a perspective matrix.
  /// \param ProjectionMatrix Perspective projection matrix to be decomposed.
  /// \param out_fFovLeft Left angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovRight Right angle of the frustum.
  /// \param out_fFovBottom Bottom angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovTop Top angle of the frustum.
  /// \param yRange The Y range used to construct the perspective matrix.
  XII_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const xiiMat4& ProjectionMatrix, xiiAngle& out_fFovLeft, xiiAngle& out_fFovRight, xiiAngle& out_fFovBottom, xiiAngle& out_fFovTop, xiiClipSpaceYMode::Enum yRange = xiiClipSpaceYMode::Regular); // [tested]

  /// \brief Extracts the field of view distances on the near plane from a perspective matrix.
  ///
  /// Convenience function that also extracts near / far values and returns the distances on the near plane to be the inverse of xiiGraphicsUtils::CreatePerspectiveProjectionMatrix.
  /// \sa xiiGraphicsUtils::CreatePerspectiveProjectionMatrix
  XII_FOUNDATION_DLL xiiResult ExtractPerspectiveMatrixFieldOfView(const xiiMat4& ProjectionMatrix, float& out_fLeft, float& out_fRight, float& out_fBottom, float& out_fTop, xiiClipSpaceDepthRange::Enum DepthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum yRange = xiiClipSpaceYMode::Regular); // [tested]

  /// \brief Computes the distances of the near and far clip planes from the given perspective projection matrix.
  ///
  /// Returns XII_FAILURE when one of the values could not be computed, because it would result in a "division by zero".
  XII_FOUNDATION_DLL xiiResult ExtractNearAndFarClipPlaneDistances(float& out_fNear, float& out_fFar, const xiiMat4& ProjectionMatrix,
                                                                   xiiClipSpaceDepthRange::Enum DepthRange = xiiClipSpaceDepthRange::Default); // [tested]


  enum class FrustumPlaneInterpolation
  {
    LeftToRight,
    BottomToTop,
    NearToFar,
  };

  /// \brief Computes an interpolated frustum plane by using linear interpolation in normalized clip space.
  ///
  /// Along left/right, up/down this makes it easy to create a regular grid of planes.
  /// Along near/far creating planes at regular intervals will result in planes in world-space that represent
  /// the same amount of depth-precision.
  ///
  /// \param dir Specifies which planes to interpolate.
  /// \param fLerpFactor The interpolation coefficient (usually in the interval [0;1]).
  XII_FOUNDATION_DLL xiiPlane ComputeInterpolatedFrustumPlane(FrustumPlaneInterpolation dir, float fLerpFactor, const xiiMat4& ProjectionMatrix,
                                                              xiiClipSpaceDepthRange::Enum DepthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// \brief Creates a perspective projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  XII_FOUNDATION_DLL xiiMat4 CreatePerspectiveProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum DepthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum yRange = xiiClipSpaceYMode::Regular,
                                                               xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  XII_FOUNDATION_DLL xiiMat4 CreatePerspectiveProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum DepthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum yRange = xiiClipSpaceYMode::Regular,
                                                               xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  /// \param fFieldOfViewX    Horizontal field of view.
  XII_FOUNDATION_DLL xiiMat4 CreatePerspectiveProjectionMatrixFromFovX(xiiAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum DepthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum yRange = xiiClipSpaceYMode::Regular,
                                                                       xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  /// \param fFieldOfViewY    Vertical field of view.
  XII_FOUNDATION_DLL xiiMat4 CreatePerspectiveProjectionMatrixFromFovY(xiiAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum DepthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum yRange = xiiClipSpaceYMode::Regular,
                                                                       xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates an orthographic projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  XII_FOUNDATION_DLL xiiMat4 CreateOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum yRange = xiiClipSpaceYMode::Regular,
                                                                xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates an orthographic projection matrix.
  XII_FOUNDATION_DLL xiiMat4 CreateOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum yRange = xiiClipSpaceYMode::Regular,
                                                                xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Returns a look-at matrix (only direction, no translation).
  ///
  /// Since this only creates a rotation matrix, vTarget can be interpreted both as a position or a direction.
  XII_FOUNDATION_DLL xiiMat3 CreateLookAtViewMatrix(
    const xiiVec3&      vTarget,
    const xiiVec3&      vUpDir,
    xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Same as CreateLookAtViewMatrix() but returns the inverse matrix
  XII_FOUNDATION_DLL xiiMat3 CreateInverseLookAtViewMatrix(
    const xiiVec3&      vTarget,
    const xiiVec3&      vUpDir,
    xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]


  /// \brief Returns a look-at matrix with both rotation and translation
  XII_FOUNDATION_DLL xiiMat4 CreateLookAtViewMatrix(const xiiVec3& vEyePos, const xiiVec3& vLookAtPos, const xiiVec3& vUpDir,
                                                    xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Same as CreateLookAtViewMatrix() but returns the inverse matrix
  XII_FOUNDATION_DLL xiiMat4 CreateInverseLookAtViewMatrix(const xiiVec3& vEyePos, const xiiVec3& vLookAtPos, const xiiVec3& vUpDir,
                                                           xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates a view matrix from the given camera vectors.
  ///
  /// The vectors are put into the appropriate matrix rows and depending on the handedness negated where necessary.
  XII_FOUNDATION_DLL xiiMat4 CreateViewMatrix(const xiiVec3& vPosition, const xiiVec3& vForwardDir, const xiiVec3& vRightDir, const xiiVec3& vUpDir,
                                              xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Similar to CreateViewMatrix() but creates the inverse matrix.
  XII_FOUNDATION_DLL xiiMat4 CreateInverseViewMatrix(const xiiVec3& vPosition, const xiiVec3& vForwardDir, const xiiVec3& vRightDir, const xiiVec3& vUpDir,
                                                     xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Extracts the forward, right and up dir and camera position from the given view matrix.
  ///
  /// The handedness should be the same as used in CreateViewMatrix() or CreateLookAtViewMatrix().
  XII_FOUNDATION_DLL void DecomposeViewMatrix(xiiVec3& out_vPosition, xiiVec3& out_vForwardDir, xiiVec3& out_vRightDir, xiiVec3& out_vUpDir, const xiiMat4& viewMatrix, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Computes the barycentric coordinates of a point in a 3D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns XII_FAILURE.
  XII_FOUNDATION_DLL xiiResult ComputeBarycentricCoordinates(xiiVec3& out_vCoordinates, const xiiVec3& v0, const xiiVec3& v1, const xiiVec3& v2, const xiiVec3& pos);

  /// \brief Computes the barycentric coordinates of a point in a 2D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns XII_FAILURE.
  XII_FOUNDATION_DLL xiiResult ComputeBarycentricCoordinates(xiiVec3& out_vCoordinates, const xiiVec2& v0, const xiiVec2& v1, const xiiVec2& v2, const xiiVec2& pos);

} // namespace xiiGraphicsUtils
