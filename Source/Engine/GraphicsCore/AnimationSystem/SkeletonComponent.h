#pragma once

#include <Foundation/Math/Declarations.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsCore/Components/RenderComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>

struct xiiMsgQueryAnimationSkeleton;

using xiiVisualizeSkeletonComponentManager = xiiComponentManagerSimple<class xiiSkeletonComponent, xiiComponentUpdateType::Always, xiiBlockStorageType::Compact>;

/// \brief Uses debug rendering to visualize various aspects of an animation skeleton.
///
/// This is meant for visually inspecting skeletons. It is used by the main skeleton editor,
/// but can also be added to a scene or added to an animated mesh on-demand.
///
/// There are different options what to visualize and also to highlight certain bones.
class XII_GRAPHICSCORE_DLL xiiSkeletonComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkeletonComponent, xiiRenderComponent, xiiVisualizeSkeletonComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSkeletonComponent

public:
  xiiSkeletonComponent();
  ~xiiSkeletonComponent();

  void                             SetSkeleton(const xiiSkeletonResourceHandle& hResource); // [ property ]
  const xiiSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }              // [ property ]

  /// \brief Sets a semicolon-separated list of bone names that should be highlighted.
  ///
  /// Set it to "*" to highlight all bones.
  /// Set it to empty to not highlight any bone.
  /// Set it to "BoneA;BoneB" to highlight the bones with name "BoneA" and "BoneB".
  void        SetBonesToHighlight(const char* szFilter); // [ property ]
  const char* GetBonesToHighlight() const;               // [ property ]

  bool m_bVisualizeBones       = true;  // [ property ]
  bool m_bVisualizeColliders   = false; // [ property ]
  bool m_bVisualizeJoints      = false; // [ property ]
  bool m_bVisualizeSwingLimits = false; // [ property ]
  bool m_bVisualizeTwistLimits = false; // [ property ]

protected:
  void Update();
  void VisualizeSkeletonDefaultState();
  void OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& msg); // [ msg handler ]

  void BuildSkeletonVisualization(xiiMsgAnimationPoseUpdated& msg);
  void BuildColliderVisualization(xiiMsgAnimationPoseUpdated& msg);
  void BuildJointVisualization(xiiMsgAnimationPoseUpdated& msg);

  void                  OnQueryAnimationSkeleton(xiiMsgQueryAnimationSkeleton& msg);
  xiiDebugRendererLine& AddLine(const xiiVec3& vStart, const xiiVec3& vEnd, const xiiColor& color);

  xiiSkeletonResourceHandle m_hSkeleton;
  xiiTransform              m_RootTransform           = xiiTransform::MakeIdentity();
  xiiUInt32                 m_uiSkeletonChangeCounter = 0;
  xiiString                 m_sBonesToHighlight;

  xiiBoundingBox                        m_MaxBounds;
  xiiDynamicArray<xiiDebugRendererLine> m_LinesSkeleton;

  struct SphereShape
  {
    xiiTransform      m_Transform;
    xiiBoundingSphere m_Shape;
    xiiColor          m_Color;
  };

  struct BoxShape
  {
    xiiTransform   m_Transform;
    xiiBoundingBox m_Shape;
    xiiColor       m_Color;
  };

  struct CapsuleShape
  {
    xiiTransform m_Transform;
    float        m_fLength;
    float        m_fRadius;
    xiiColor     m_Color;
  };

  struct AngleShape
  {
    xiiTransform m_Transform;
    xiiColor     m_Color;
    xiiAngle     m_StartAngle;
    xiiAngle     m_EndAngle;
  };

  struct ConeLimitShape
  {
    xiiTransform m_Transform;
    xiiColor     m_Color;
    xiiAngle     m_Angle1;
    xiiAngle     m_Angle2;
  };

  struct CylinderShape
  {
    xiiTransform m_Transform;
    xiiColor     m_Color;
    float        m_fRadius1;
    float        m_fRadius2;
    float        m_fLength;
  };

  xiiDynamicArray<SphereShape>    m_SpheresShapes;
  xiiDynamicArray<BoxShape>       m_BoxShapes;
  xiiDynamicArray<CapsuleShape>   m_CapsuleShapes;
  xiiDynamicArray<AngleShape>     m_AngleShapes;
  xiiDynamicArray<ConeLimitShape> m_ConeLimitShapes;
  xiiDynamicArray<CylinderShape>  m_CylinderShapes;
};
