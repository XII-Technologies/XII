#pragma once

#include <Foundation/Math/Declarations.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsCore/Components/RenderComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>

struct xiiMsgQueryAnimationSkeleton;

using xiiVisualizeSkeletonComponentManager = xiiComponentManagerSimple<class xiiSkeletonComponent, xiiComponentUpdateType::Always, xiiBlockStorageType::Compact>;

class XII_RENDERERCORE_DLL xiiSkeletonComponent : public xiiRenderComponent
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

  void        SetSkeletonFile(const char* szFile); // [ property ]
  const char* GetSkeletonFile() const;             // [ property ]

  void                             SetSkeleton(const xiiSkeletonResourceHandle& hResource);
  const xiiSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

  void        SetBonesToHighlight(const char* szFilter); // [ property ]
  const char* GetBonesToHighlight() const;               // [ property ]

  void VisualizeSkeletonDefaultState();

  bool m_bVisualizeBones       = true;
  bool m_bVisualizeColliders   = false;
  bool m_bVisualizeJoints      = false;
  bool m_bVisualizeSwingLimits = false;
  bool m_bVisualizeTwistLimits = false;

protected:
  void Update();
  void OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& msg); // [ msg handler ]

  void BuildSkeletonVisualization(xiiMsgAnimationPoseUpdated& msg);
  void BuildColliderVisualization(xiiMsgAnimationPoseUpdated& msg);
  void BuildJointVisualization(xiiMsgAnimationPoseUpdated& msg);

  void                    OnQueryAnimationSkeleton(xiiMsgQueryAnimationSkeleton& msg);
  xiiDebugRenderer::Line& AddLine(const xiiVec3& vStart, const xiiVec3& vEnd, const xiiColor& color);

  xiiSkeletonResourceHandle m_hSkeleton;
  xiiTransform              m_RootTransform           = xiiTransform::MakeIdentity();
  xiiUInt32                 m_uiSkeletonChangeCounter = 0;
  xiiString                 m_sBonesToHighlight;

  xiiBoundingBox                          m_MaxBounds;
  xiiDynamicArray<xiiDebugRenderer::Line> m_LinesSkeleton;

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
