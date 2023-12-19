#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class xiiSkeletonAssetDocument;
struct xiiSkeletonAssetEvent;

class xiiSkeletonActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(xiiStringView sMapping);

  static xiiActionDescriptorHandle s_hCategory;
  static xiiActionDescriptorHandle s_hRenderBones;
  static xiiActionDescriptorHandle s_hRenderColliders;
  static xiiActionDescriptorHandle s_hRenderJoints;
  static xiiActionDescriptorHandle s_hRenderSwingLimits;
  static xiiActionDescriptorHandle s_hRenderTwistLimits;
  static xiiActionDescriptorHandle s_hRenderPreviewMesh;
};

class xiiSkeletonAction : public xiiButtonAction
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkeletonAction, xiiButtonAction);

public:
  enum class ActionType
  {
    RenderBones,
    RenderColliders,
    RenderJoints,
    RenderSwingLimits,
    RenderTwistLimits,
    RenderPreviewMesh,
  };

  xiiSkeletonAction(const xiiActionContext& context, const char* szName, ActionType type);
  ~xiiSkeletonAction();

  virtual void Execute(const xiiVariant& value) override;

private:
  void AssetEventHandler(const xiiSkeletonAssetEvent& e);
  void UpdateState();

  xiiSkeletonAssetDocument* m_pSkeletonpDocument = nullptr;
  ActionType                m_Type;
};
