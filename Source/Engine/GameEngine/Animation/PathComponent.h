#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Types/Bitflags.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/RendererCoreDLL.h>

struct xiiMsgTransformChanged;
struct xiiMsgParentChanged;

//////////////////////////////////////////////////////////////////////////

XII_DECLARE_FLAGS(xiiUInt32, xiiPathComponentFlags, VisualizePath, VisualizeUpDir);
XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiPathComponentFlags);

//////////////////////////////////////////////////////////////////////////

struct XII_GAMEENGINE_DLL xiiEventMsgPathChanged : public xiiEventMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiEventMsgPathChanged, xiiEventMessage);
};

//////////////////////////////////////////////////////////////////////////

class xiiPathComponentManager : public xiiComponentManager<class xiiPathComponent, xiiBlockStorageType::FreeList>
{
public:
  xiiPathComponentManager(xiiWorld* pWorld);

  void SetEnableUpdate(xiiPathComponent* pThis, bool bEnable);

protected:
  void Initialize() override;
  void Update(const xiiWorldModule::UpdateContext& context);

  xiiHybridArray<xiiPathComponent*, 32> m_NeedUpdate;
};

/// \brief Describes a path shape.
///
/// This can be used for moving things along the path (see xiiFollowPathComponent) or to describe the (complex) shape of an object, for example a rope.
///
/// The xiiPathComponent stores the shape as nodes with positions and tangents.
/// It can be asked to provide a 'linearized' representation, e.g. one that is made up of many short segments whose linear interpolation
/// is still reasonably close to the curved shape.
///
/// To set up the shape, attach child objects and attach a xiiPathNodeComponent to each. Also give each child object a distinct name.
/// Then reference these child objects by name through the "Nodes" property on the path shape.
///
/// During scene export, typically the child objects are automatically deleted (if they have no children and no other components).
/// Instead, the xiiPathComponent stores all necessary information in a more compact representation.
class XII_GAMEENGINE_DLL xiiPathComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPathComponent, xiiComponent, xiiPathComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& ref_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiPathComponent

public:
  xiiPathComponent();
  ~xiiPathComponent();

  /// \brief Informs the path component, that its shape has changed. Sent by path nodes when they are modified.
  void OnEventMsgPathChanged(xiiEventMsgPathChanged& ref_msg); // [ message handler ]

  /// \brief Whether the path end connects to the beginning.
  void SetClosed(bool bClosed);                // [ property ]
  bool GetClosed() const { return m_bClosed; } // [ property ]

  void                               SetPathFlags(xiiBitflags<xiiPathComponentFlags> flags); // [ property ]
  xiiBitflags<xiiPathComponentFlags> GetPathFlags() const { return m_PathFlags; }            // [ property ]


  /// \brief The 'raw' data for a single path control point
  struct ControlPoint
  {
    xiiVec3  m_vPosition   = xiiVec3::ZeroVector();
    xiiVec3  m_vTangentIn  = xiiVec3::ZeroVector();
    xiiVec3  m_vTangentOut = xiiVec3::ZeroVector();
    xiiAngle m_Roll;

    xiiResult Serialize(xiiStreamWriter& ref_writer) const;
    xiiResult Deserialize(xiiStreamReader& ref_reader);
  };

  /// \brief If the control points changed recently, this makes sure the local representation is synchronized. Call this before GetControlPointRepresentation(), if necessary.
  void EnsureControlPointRepresentationIsUpToDate();

  /// \brief Grants access to the control points that define the path's shape.
  const xiiArrayPtr<const xiiPathComponent::ControlPoint> GetControlPointRepresentation() const { return m_ControlPointRepresentation; }


  /// \brief If the path is linearized, this represents a single sample point
  struct LinearizedElement
  {
    xiiVec3 m_vPosition    = xiiVec3::ZeroVector();
    xiiVec3 m_vUpDirection = xiiVec3::UnitZAxis();
  };

  /// \brief If the control points changed recently, this makes sure the linearized representation gets recreated. Call this before GetLinearizedRepresentation(), if necessary.
  void EnsureLinearizedRepresentationIsUpToDate();

  /// \brief Grants access to the linearized representation that define the path's shape.
  const xiiArrayPtr<const xiiPathComponent::LinearizedElement> GetLinearizedRepresentation() const { return m_LinearizedRepresentation; }

  /// \brief Returns the total length of the linearized path representation.
  float GetLinearizedRepresentationLength() const { return m_fLinearizedLength; }

  /// \brief Forces that the current control point state is never updated in the future. Used as a work-around during serialization.
  void SetDisableControlPointUpdates(bool bDisable) { m_bDisableControlPointUpdates = bDisable; }

  /// \brief An object that keeps track of where one is sampling the path component.
  ///
  /// If you want to follow a path, keep this object alive and only advance its position,
  /// to not waste performance.
  struct LinearSampler
  {
    /// \brief Resets the sampler to point to the beginning of the path.
    void SetToStart();

  private:
    friend class xiiPathComponent;

    float     m_fSegmentFraction = 0.0f;
    xiiUInt32 m_uiSegmentNode    = 0;
  };

  /// \brief Sets the sampler to the desired distance along the path.
  ///
  /// For a long distance, this is a slow operation, because it has to follow the path
  /// from the beginning.
  void SetLinearSamplerTo(LinearSampler& ref_sampler, float fDistance) const;

  /// \brief Moves the sampler along the path by the desired distance.
  ///
  /// Prefer this over SetLinearSamplerTo().
  bool AdvanceLinearSamplerBy(LinearSampler& ref_sampler, float& inout_fAddDistance) const;

  /// \brief Samples the linearized path representation at the desired location and returns the interpolated values.
  xiiPathComponent::LinearizedElement SampleLinearizedRepresentation(const LinearSampler& sampler) const;

  /// \brief Specifies how large the error of the linearized path representation is allowed to be.
  ///
  /// The lower the allowed error, the more detailed the linearized path will be, to match the
  /// Bezier representation as closely as possible.
  ///
  /// The error is a distance measure. Thus a value of 0.01 means that the linearized representation
  /// may at most deviate a centimeter from the real curve.
  void  SetLinearizationError(float fError);                            // [ property ]
  float GetLinearizationError() const { return m_fLinearizationError; } // [ property ]

protected:
  xiiUInt32        Nodes_GetCount() const { return m_Nodes.GetCount(); }   // [ property ]
  const xiiString& Nodes_GetNode(xiiUInt32 i) const { return m_Nodes[i]; } // [ property ]
  void             Nodes_SetNode(xiiUInt32 i, const xiiString& node);      // [ property ]
  void             Nodes_Insert(xiiUInt32 uiIndex, const xiiString& node); // [ property ]
  void             Nodes_Remove(xiiUInt32 uiIndex);                        // [ property ]

  void FindControlPoints(xiiDynamicArray<ControlPoint>& out_ControlPoints) const;
  void CreateLinearizedPathRepresentation(const xiiDynamicArray<ControlPoint>& points);

  void DrawDebugVisualizations();

  xiiBitflags<xiiPathComponentFlags> m_PathFlags;                                // [ property ]
  float                              m_fLinearizationError              = 0.05f; // [ property ]
  float                              m_fLinearizedLength                = 0.0f;  //
  bool                               m_bDisableControlPointUpdates      = false; //
  bool                               m_bControlPointsChanged            = true;  //
  bool                               m_bLinearizedRepresentationChanged = true;  //
  bool                               m_bClosed                          = false; // [ property ]
  xiiDynamicArray<xiiString>         m_Nodes;                                    // [ property ]
  xiiDynamicArray<LinearizedElement> m_LinearizedRepresentation;                 //
  xiiDynamicArray<ControlPoint>      m_ControlPointRepresentation;               //
};

//////////////////////////////////////////////////////////////////////////

using xiiPathNodeComponentManager = xiiComponentManager<class xiiPathNodeComponent, xiiBlockStorageType::Compact>;

/// \brief The different modes that tangents may use in a path node.
struct xiiPathNodeTangentMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Auto,   ///< The curvature through the node is automatically computed to be smooth.
    Linear, ///< There is no curvature through this node/tangent. Creates sharp corners.

    Default = Auto
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiPathNodeTangentMode);


/// \brief Attach this to child object of a xiiPathComponent to turn them into viable path nodes.
///
/// See xiiPathComponent for details on how to create a path.
///
/// This component allows to specify the mode of the tangents (linear, curved),
/// and also to adjust the 'roll' that the path will have at this location (rotation around the forward axis).
class XII_GAMEENGINE_DLL xiiPathNodeComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPathNodeComponent, xiiComponent, xiiPathNodeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiPathNodeComponent

public:
  xiiPathNodeComponent();
  ~xiiPathNodeComponent();

  /// \brief Sets the rotation along the forward axis, that the path shall have at this location.
  void     SetRoll(xiiAngle roll);                                                       // [ property ]
  xiiAngle GetRoll() const { return m_Roll; }                                            // [ property ]
                                                                                         //
  void                            SetTangentMode1(xiiEnum<xiiPathNodeTangentMode> mode); // [ property ]
  xiiEnum<xiiPathNodeTangentMode> GetTangentMode1() const { return m_TangentMode1; }     // [ property ]
                                                                                         //
  void                            SetTangentMode2(xiiEnum<xiiPathNodeTangentMode> mode); // [ property ]
  xiiEnum<xiiPathNodeTangentMode> GetTangentMode2() const { return m_TangentMode2; }     // [ property ]

protected:
  void OnMsgTransformChanged(xiiMsgTransformChanged& msg);
  void OnMsgParentChanged(xiiMsgParentChanged& msg);

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void PathChanged();

  xiiAngle                        m_Roll;
  xiiEnum<xiiPathNodeTangentMode> m_TangentMode1;
  xiiEnum<xiiPathNodeTangentMode> m_TangentMode2;
};
