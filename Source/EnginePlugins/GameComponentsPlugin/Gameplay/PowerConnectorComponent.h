#include <Core/World/Component.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>
#include <GameEngine/AI/SensorComponent.h>

struct xiiMsgObjectGrabbed;
struct xiiMsgSensorDetectedObjectsChanged;

/// \brief This event is posted by xiiPowerConnectorComponent whenever the power input on a connector changes.
///
/// When a connector gets input through it's connection to another connector, this message is sent.
/// This can then be used by scripts on parent nodes to switch other functionality on or off.
/// Both the previous and new value are sent, so that the difference can be calculated.
/// This is useful in case a script has multiple connectors and needs to keep track of the total amount of power available.
class XII_GAMECOMPONENTS_DLL xiiEventMsgSetPowerInput : public xiiEventMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiEventMsgSetPowerInput, xiiEventMessage);

  xiiUInt16 m_uiPrevValue = 0;
  xiiUInt16 m_uiNewValue  = 0;
};

using xiiPowerConnectorComponentManager = xiiComponentManager<class xiiPowerConnectorComponent, xiiBlockStorageType::Compact>;

/// \brief This component is for propagating the flow of power in cables or fluid in pipes and determine whether it arrives at a receiver.
///
/// This component is meant for building puzzles where you have to connect the right objects to power something.
/// It uses physics constraints to physically connect two pieces and have them snap together.
/// It also reacts to being grabbed (xiiMsgObjectGrabbed) to disconnect.
///
/// On its own this component doesn't do anything.
/// However, it can be set to be 'connected' to another object with a xiiPowerConnectorComponent, in which case it would propagate its own
/// 'output' as the 'input' on that component.
/// If its output is non-zero and thus the input on the connected component is also non-zero, the other component will post xiiEventMsgSetPowerInput,
/// to which a script can react and for example switch a light on.
///
/// Connectors are bi-directional ("full duplex"), so they can have both an input and an output and the two values are independent of each other.
/// That means power can flow in both or just one direction and therefore it is not important with which end a cable gets connected to something.
///
/// To enable building things like cables, each xiiPowerConnectorComponent can also have a 'buddy', which is an object on which another
/// xiiPowerConnectorComponent exists.
/// If a connector gets input, that input value is propagated to the buddy as its output value. Thus when a cable gets input on one end,
/// the other end (if it is properly set as the buddy) will output that value. So if that end is also 'connected' to something, the output will
/// be further propagated as the 'input' on that object. This can go through many hops until the value reaches the final connector (if you build a
/// circular chain it will stop when it reaches the starting point).
///
/// The component automatically connects to another object when it receives a xiiMsgSensorDetectedObjectsChanged, so it should have a child
/// object with a sensor. The sensor should use a dedicated spatial category to search for markers where it can connect.
///
/// To have a sensor (or other effects) only active when the connector is grabbed, put them in a child object with the name "ActiveWhenGrabbed"
/// and disable the object by default. The parent xiiPowerConnectorComponent will toggle the active flag of that object when it gets grabbed or let go.
///
/// To build a cable, don't forget to set each end as the 'buddy' of the other end.
class XII_GAMECOMPONENTS_DLL xiiPowerConnectorComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiPowerConnectorComponent, xiiComponent, xiiPowerConnectorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiPowerConnectorComponent

public:
  /// \brief Sets how much output (of whatever kind) this connector produces.
  ///
  /// If this is zero, it is either a receiver, or a pass-through connector, e.g. a cable, or just currently inactive.
  /// If this is non-zero, it acts like a source, and when another connector gets connected to it, that output will be propagated
  /// through the connection/buddy chain.
  void      SetOutput(xiiUInt16 value);              // [ property ]
  xiiUInt16 GetOutput() const { return m_uiOutput; } // [ property ]

  void SetBuddy(xiiGameObjectHandle hObject);
  void SetConnectedTo(xiiGameObjectHandle hObject);

  /// \brief Whether the connector is currently connected to another connector.
  bool IsConnected() const; // [ scriptable ]

  /// \brief Whether the connector is physically attached to another connector.
  bool IsAttached() const; // [ scriptable ]

  void Detach(); // [ scriptable ]
  void Attach(xiiGameObjectHandle hObject);

protected:
  void SetBuddyReference(const char* szReference);       // [ property ]
  void SetConnectedToReference(const char* szReference); // [ property ]

  void ConnectToSocket(xiiGameObjectHandle hSocket);

  void SetInput(xiiUInt16 value);

  /// \brief Whenever a xiiMsgSensorDetectedObjectsChanged arrives, the connector attempts to connect to the reported object.
  void OnMsgSensorDetectedObjectsChanged(xiiMsgSensorDetectedObjectsChanged& msg); // [ message handler ]

  /// \brief Whenever the connector gets grabbed, it detaches from its current connection.
  ///
  /// It also toggles the active flag of the child object with the name "ActiveWhenGrabbed".
  /// So to only have it connect to other connectors when grabbed, put the sensor component into such a child object.
  void OnMsgObjectGrabbed(xiiMsgObjectGrabbed& msg); // [ message handler ]


  xiiTime             m_DetachTime;
  xiiGameObjectHandle m_hAttachPoint;
  xiiGameObjectHandle m_hGrabbedBy;

  xiiUInt16 m_uiOutput = 0;
  xiiUInt16 m_uiInput  = 0;

  xiiGameObjectHandle m_hBuddy;
  xiiGameObjectHandle m_hConnectedTo;

  void InputChanged(xiiUInt16 uiPrevInput, xiiUInt16 uiInput);
  void OutputChanged(xiiUInt16 uiOutput);

private:
  const char* DummyGetter() const { return nullptr; }
};
