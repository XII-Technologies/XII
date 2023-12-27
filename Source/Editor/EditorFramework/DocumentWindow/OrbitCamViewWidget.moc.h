#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <Foundation/Types/UniquePtr.h>

class xiiOrbitCameraContext;
class xiiSelectionContext;

class XII_EDITORFRAMEWORK_DLL xiiQtOrbitCamViewWidget : public xiiQtEngineViewWidget
{
  Q_OBJECT
public:
  xiiQtOrbitCamViewWidget(xiiQtEngineDocumentWindow* pOwnerWindow, xiiEngineViewConfig* pViewConfig, bool bPicking = false);
  ~xiiQtOrbitCamViewWidget();

  void ConfigureFixed(const xiiVec3& vCenterPos, const xiiVec3& vHalfBoxSize, const xiiVec3& vCamPosition);
  void ConfigureRelative(const xiiVec3& vCenterPos, const xiiVec3& vHalfBoxSize, const xiiVec3& vCamDirection, float fCamDistanceScale);

  void SetOrbitVolume(const xiiVec3& vCenterPos, const xiiVec3& vHalfBoxSize);

  xiiOrbitCameraContext* GetOrbitCamera();

  virtual void SyncToEngine() override;

private:
  bool m_bSetDefaultCamPos = true;

  xiiUniquePtr<xiiOrbitCameraContext> m_pOrbitCameraContext;
  xiiUniquePtr<xiiSelectionContext>   m_pSelectionContext;
};
