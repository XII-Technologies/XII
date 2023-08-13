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
  XII_FOUNDATION_DLL xiiResult ConvertWorldPosToScreenPos(const xiiMat4& mModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3& vPoint, xiiVec3& out_vScreenPos, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

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
  XII_FOUNDATION_DLL xiiResult ConvertWorldPosToScreenPos(const xiiMat4d& mModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3d& vPoint, xiiVec3d& out_vScreenPos, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

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
  XII_FOUNDATION_DLL xiiResult ConvertScreenPosToWorldPos(const xiiMat4& mInverseModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3& vScreenPos, xiiVec3& out_vPoint, xiiVec3* out_pDirection = nullptr, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

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
  XII_FOUNDATION_DLL xiiResult ConvertScreenPosToWorldPos(const xiiMat4d& mInverseModelViewProjection, const xiiUInt32 uiViewportX, const xiiUInt32 uiViewportY, const xiiUInt32 uiViewportWidth, const xiiUInt32 uiViewportHeight, const xiiVec3d& vScreenPos, xiiVec3d& out_vPoint, xiiVec3d* out_pDirection = nullptr, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// \brief Checks whether the given transformation matrix would change the winding order of a triangle's vertices and thus requires that
  /// the vertex order gets reversed to compensate.
  XII_FOUNDATION_DLL bool IsTriangleFlipRequired(const xiiMat3& mTransformation);

  /// \brief Checks whether the given transformation matrix would change the winding order of a triangle's vertices and thus requires that
  /// the vertex order gets reversed to compensate.
  XII_FOUNDATION_DLL bool IsTriangleFlipRequired(const xiiMat3d& mTransformation);

  /// \brief Converts a projection or view-projection matrix from one depth-range convention to another
  XII_FOUNDATION_DLL void ConvertProjectionMatrixDepthRange(xiiMat4& inout_mMatrix, xiiClipSpaceDepthRange::Enum srcDepthRange, xiiClipSpaceDepthRange::Enum dstDepthRange); // [tested]

  /// \brief Converts a projection or view-projection matrix from one depth-range convention to another
  XII_FOUNDATION_DLL void ConvertProjectionMatrixDepthRange(xiiMat4d& inout_mMatrix, xiiClipSpaceDepthRange::Enum srcDepthRange, xiiClipSpaceDepthRange::Enum dstDepthRange); // [tested]

  /// \brief Retrieves the horizontal and vertical field-of-view angles from the perspective matrix.
  ///
  /// \note If an orthographic projection matrix is passed in, the returned angle values will be zero.
  XII_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const xiiMat4& mProjectionMatrix, xiiAngle& out_fovX, xiiAngle& out_fovY); // [tested]

  /// \brief Retrieves the horizontal and vertical field-of-view angles from the perspective matrix.
  ///
  /// \note If an orthographic projection matrix is passed in, the returned angle values will be zero.
  XII_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const xiiMat4d& mProjectionMatrix, xiiAngle& out_fovX, xiiAngle& out_fovY); // [tested]

  /// \brief Extracts the field of view angles from a perspective matrix.
  /// \param ProjectionMatrix Perspective projection matrix to be decomposed.
  /// \param out_fFovLeft Left angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovRight Right angle of the frustum.
  /// \param out_fFovBottom Bottom angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovTop Top angle of the frustum.
  /// \param yRange The Y range used to construct the perspective matrix.
  XII_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const xiiMat4& mProjectionMatrix, xiiAngle& out_fovLeft, xiiAngle& out_fovRight, xiiAngle& out_fovBottom, xiiAngle& out_fovTop, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular); // [tested]

  /// \brief Extracts the field of view angles from a perspective matrix.
  /// \param ProjectionMatrix Perspective projection matrix to be decomposed.
  /// \param out_fFovLeft Left angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovRight Right angle of the frustum.
  /// \param out_fFovBottom Bottom angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovTop Top angle of the frustum.
  /// \param yRange The Y range used to construct the perspective matrix.
  XII_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const xiiMat4d& mProjectionMatrix, xiiAngle& out_fovLeft, xiiAngle& out_fovRight, xiiAngle& out_fovBottom, xiiAngle& out_fovTop, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular); // [tested]

  /// \brief Extracts the field of view distances on the near plane from a perspective matrix.
  ///
  /// Convenience function that also extracts near / far values and returns the distances on the near plane to be the inverse of xiiGraphicsUtils::CreatePerspectiveProjectionMatrix.
  /// \sa xiiGraphicsUtils::CreatePerspectiveProjectionMatrix
  XII_FOUNDATION_DLL xiiResult ExtractPerspectiveMatrixFieldOfView(const xiiMat4& mProjectionMatrix, float& out_fLeft, float& out_fRight, float& out_fBottom, float& out_fTop, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular); // [tested]

  /// \brief Extracts the field of view distances on the near plane from a perspective matrix.
  ///
  /// Convenience function that also extracts near / far values and returns the distances on the near plane to be the inverse of xiiGraphicsUtils::CreatePerspectiveProjectionMatrix.
  /// \sa xiiGraphicsUtils::CreatePerspectiveProjectionMatrix
  XII_FOUNDATION_DLL xiiResult ExtractPerspectiveMatrixFieldOfView(const xiiMat4d& mProjectionMatrix, float& out_fLeft, float& out_fRight, float& out_fBottom, float& out_fTop, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular); // [tested]

  /// \brief Computes the distances of the near and far clip planes from the given perspective projection matrix.
  ///
  /// Returns XII_FAILURE when one of the values could not be computed, because it would result in a "division by zero".
  XII_FOUNDATION_DLL xiiResult ExtractNearAndFarClipPlaneDistances(float& out_fNear, float& out_fFar, const xiiMat4& mProjectionMatrix, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// \brief Computes the distances of the near and far clip planes from the given perspective projection matrix.
  ///
  /// Returns XII_FAILURE when one of the values could not be computed, because it would result in a "division by zero".
  XII_FOUNDATION_DLL xiiResult ExtractNearAndFarClipPlaneDistances(float& out_fNear, float& out_fFar, const xiiMat4d& mProjectionMatrix, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]


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
  XII_FOUNDATION_DLL xiiPlane ComputeInterpolatedFrustumPlane(FrustumPlaneInterpolation dir, float fLerpFactor, const xiiMat4& mProjectionMatrix, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// \brief Computes an interpolated frustum plane by using linear interpolation in normalized clip space.
  ///
  /// Along left/right, up/down this makes it easy to create a regular grid of planes.
  /// Along near/far creating planes at regular intervals will result in planes in world-space that represent
  /// the same amount of depth-precision.
  ///
  /// \param dir Specifies which planes to interpolate.
  /// \param fLerpFactor The interpolation coefficient (usually in the interval [0;1]).
  XII_FOUNDATION_DLL xiiPlaned ComputeInterpolatedFrustumPlane(FrustumPlaneInterpolation dir, float fLerpFactor, const xiiMat4d& mProjectionMatrix, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default); // [tested]

  /// \brief Creates a perspective projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  XII_FOUNDATION_DLL xiiMat4 CreatePerspectiveProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  XII_FOUNDATION_DLL xiiMat4 CreatePerspectiveProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  /// \param fFieldOfViewX    Horizontal field of view.
  XII_FOUNDATION_DLL xiiMat4 CreatePerspectiveProjectionMatrixFromFovX(xiiAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  /// \param fFieldOfViewY    Vertical field of view.
  XII_FOUNDATION_DLL xiiMat4 CreatePerspectiveProjectionMatrixFromFovY(xiiAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates an orthographic projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  XII_FOUNDATION_DLL xiiMat4 CreateOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates an orthographic projection matrix.
  XII_FOUNDATION_DLL xiiMat4 CreateOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  XII_FOUNDATION_DLL xiiMat4d CreatePerspectiveProjectionMatrixDouble(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  XII_FOUNDATION_DLL xiiMat4d CreatePerspectiveProjectionMatrixDouble(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  /// \param fFieldOfViewX    Horizontal field of view.
  XII_FOUNDATION_DLL xiiMat4d CreatePerspectiveProjectionMatrixDoubleFromFovX(xiiAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  /// \param fFieldOfViewY    Vertical field of view.
  XII_FOUNDATION_DLL xiiMat4d CreatePerspectiveProjectionMatrixDoubleFromFovY(xiiAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates an orthographic projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  XII_FOUNDATION_DLL xiiMat4d CreateOrthographicProjectionMatrixDouble(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates an orthographic projection matrix.
  XII_FOUNDATION_DLL xiiMat4d CreateOrthographicProjectionMatrixDouble(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, xiiClipSpaceDepthRange::Enum depthRange = xiiClipSpaceDepthRange::Default, xiiClipSpaceYMode::Enum range = xiiClipSpaceYMode::Regular, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Returns a look-at matrix (only direction, no translation).
  ///
  /// Since this only creates a rotation matrix, vTarget can be interpreted both as a position or a direction.
  XII_FOUNDATION_DLL xiiMat3 CreateLookAtViewMatrix(const xiiVec3& vTarget, const xiiVec3& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Returns a look-at matrix (only direction, no translation).
  ///
  /// Since this only creates a rotation matrix, vTarget can be interpreted both as a position or a direction.
  XII_FOUNDATION_DLL xiiMat3d CreateLookAtViewMatrix(const xiiVec3d& vTarget, const xiiVec3d& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Same as CreateLookAtViewMatrix() but returns the inverse matrix
  XII_FOUNDATION_DLL xiiMat3 CreateInverseLookAtViewMatrix(const xiiVec3& vTarget, const xiiVec3& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Same as CreateLookAtViewMatrix() but returns the inverse matrix
  XII_FOUNDATION_DLL xiiMat3d CreateInverseLookAtViewMatrix(const xiiVec3d& vTarget, const xiiVec3d& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Returns a look-at matrix with both rotation and translation
  XII_FOUNDATION_DLL xiiMat4 CreateLookAtViewMatrix(const xiiVec3& vEyePos, const xiiVec3& vLookAtPos, const xiiVec3& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Returns a look-at matrix with both rotation and translation
  XII_FOUNDATION_DLL xiiMat4d CreateLookAtViewMatrix(const xiiVec3d& vEyePos, const xiiVec3d& vLookAtPos, const xiiVec3d& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Same as CreateLookAtViewMatrix() but returns the inverse matrix
  XII_FOUNDATION_DLL xiiMat4 CreateInverseLookAtViewMatrix(const xiiVec3& vEyePos, const xiiVec3& vLookAtPos, const xiiVec3& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Same as CreateLookAtViewMatrix() but returns the inverse matrix
  XII_FOUNDATION_DLL xiiMat4d CreateInverseLookAtViewMatrix(const xiiVec3d& vEyePos, const xiiVec3d& vLookAtPos, const xiiVec3d& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates a view matrix from the given camera vectors.
  ///
  /// The vectors are put into the appropriate matrix rows and depending on the handedness negated where necessary.
  XII_FOUNDATION_DLL xiiMat4 CreateViewMatrix(const xiiVec3& vPosition, const xiiVec3& vForwardDir, const xiiVec3& vRightDir, const xiiVec3& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Creates a view matrix from the given camera vectors.
  ///
  /// The vectors are put into the appropriate matrix rows and depending on the handedness negated where necessary.
  XII_FOUNDATION_DLL xiiMat4d CreateViewMatrix(const xiiVec3d& vPosition, const xiiVec3d& vForwardDir, const xiiVec3d& vRightDir, const xiiVec3d& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Similar to CreateViewMatrix() but creates the inverse matrix.
  XII_FOUNDATION_DLL xiiMat4 CreateInverseViewMatrix(const xiiVec3& vPosition, const xiiVec3& vForwardDir, const xiiVec3& vRightDir, const xiiVec3& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Similar to CreateViewMatrix() but creates the inverse matrix.
  XII_FOUNDATION_DLL xiiMat4d CreateInverseViewMatrix(const xiiVec3d& vPosition, const xiiVec3d& vForwardDir, const xiiVec3d& vRightDir, const xiiVec3d& vUpDir, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Extracts the forward, right and up dir and camera position from the given view matrix.
  ///
  /// The handedness should be the same as used in CreateViewMatrix() or CreateLookAtViewMatrix().
  XII_FOUNDATION_DLL void DecomposeViewMatrix(xiiVec3& out_vPosition, xiiVec3& out_vForwardDir, xiiVec3& out_vRightDir, xiiVec3& out_vUpDir, const xiiMat4& mViewMatrix, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Extracts the forward, right and up dir and camera position from the given view matrix.
  ///
  /// The handedness should be the same as used in CreateViewMatrix() or CreateLookAtViewMatrix().
  XII_FOUNDATION_DLL void DecomposeViewMatrix(xiiVec3d& out_vPosition, xiiVec3d& out_vForwardDir, xiiVec3d& out_vRightDir, xiiVec3d& out_vUpDir, const xiiMat4d& mViewMatrix, xiiHandedness::Enum handedness = xiiHandedness::Default); // [tested]

  /// \brief Computes the barycentric coordinates of a point in a 3D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns XII_FAILURE.
  XII_FOUNDATION_DLL xiiResult ComputeBarycentricCoordinates(xiiVec3& out_vCoordinates, const xiiVec3& v0, const xiiVec3& v1, const xiiVec3& v2, const xiiVec3& vPos);

  /// \brief Computes the barycentric coordinates of a point in a 3D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns XII_FAILURE.
  XII_FOUNDATION_DLL xiiResult ComputeBarycentricCoordinates(xiiVec3d& out_vCoordinates, const xiiVec3d& v0, const xiiVec3d& v1, const xiiVec3d& v2, const xiiVec3d& vPos);

  /// \brief Computes the barycentric coordinates of a point in a 2D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns XII_FAILURE.
  XII_FOUNDATION_DLL xiiResult ComputeBarycentricCoordinates(xiiVec3& out_vCoordinates, const xiiVec2& v0, const xiiVec2& v1, const xiiVec2& v2, const xiiVec2& vPos);

  /// \brief Computes the barycentric coordinates of a point in a 2D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns XII_FAILURE.
  XII_FOUNDATION_DLL xiiResult ComputeBarycentricCoordinates(xiiVec3d& out_vCoordinates, const xiiVec2d& v0, const xiiVec2d& v1, const xiiVec2d& v2, const xiiVec2d& vPos);

} // namespace xiiGraphicsUtils
