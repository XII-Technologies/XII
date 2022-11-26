#pragma once

#include <Core/GameState/StateMap.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Variant.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

class xiiVisualScriptNode;
class xiiMessage;
struct xiiVisualScriptResourceDescriptor;
class xiiGameObject;
class xiiWorld;
struct xiiVisualScriptInstanceActivity;
struct xiiEventMessage;

typedef xiiUInt32 xiiVisualScriptNodeConnectionID;
typedef xiiUInt32 xiiVisualScriptPinConnectionID;
using xiiVisualScriptResourceHandle = xiiTypedResourceHandle<class xiiVisualScriptResource>;

typedef bool (*xiiVisualScriptDataPinAssignFunc)(const void* src, void* dst);

/// \brief An instance of a visual script resource. Stores the current script state and executes nodes.
class XII_GAMEENGINE_DLL xiiVisualScriptInstance
{
public:
  xiiVisualScriptInstance();
  ~xiiVisualScriptInstance();

  /// \brief Clears the current state and recreates the script instance from the given template.
  void Configure(const xiiVisualScriptResourceHandle& hScript, xiiComponent* pOwnerComponent);

  /// \brief Runs all nodes that are marked for execution. Typically nodes that handle events will mark themselves for execution in the next update.
  void ExecuteScript(xiiVisualScriptInstanceActivity* pActivity = nullptr);

  /// \brief The message is dispatched to all nodes, which may react on it, for instance by tagging themselves for execution in the next
  /// ExecuteScript() call.
  bool HandleMessage(xiiMessage& msg);

  /// \brief Called by xiiVisualScriptNode classes to pass the new value of an output pin to all connected nodes.
  void SetOutputPinValue(const xiiVisualScriptNode* pNode, xiiUInt8 uiPin, const void* pValue);

  /// \brief Called by xiiVisualScriptNode classes to execute the node that is connected on the given output execution pin.
  void ExecuteConnectedNodes(const xiiVisualScriptNode* pNode, xiiUInt16 uiNthTarget);

  /// \brief Returns the xiiGameObject that owns this script. May be invalid, if the instance is not attached to a game object.
  xiiGameObjectHandle GetOwner() const { return m_hOwnerObject; }

  /// \brief Returns the xiiComponent that owns this script. May be invalid, if the instance is not attached to a component.
  xiiComponentHandle GetOwnerComponent() const { return m_hOwnerComponent; }

  /// \brief Returns the world of the owner game object.
  xiiWorld* GetWorld() const { return m_pWorld; }

  /// \brief Returns the map that holds the local variables of the script.
  const xiiStateMap& GetLocalVariables() const { return m_LocalVariables; }

  /// \brief Returns the map that holds the local variables of the script.
  xiiStateMap& GetLocalVariables() { return m_LocalVariables; }

  /// \brief Needs to be called once to register the default data pin conversion functions.
  static void SetupPinDataTypeConversions();

  static void                             RegisterDataPinAssignFunction(xiiVisualScriptDataPinType::Enum sourceType, xiiVisualScriptDataPinType::Enum dstType, xiiVisualScriptDataPinAssignFunc func);
  static xiiVisualScriptDataPinAssignFunc FindDataPinAssignFunction(xiiVisualScriptDataPinType::Enum sourceType, xiiVisualScriptDataPinType::Enum dstType);

  /// \brief Returns whether this script has a node that handles this type of event message.
  bool HandlesEventMessage(const xiiEventMessage& msg) const;

private:
  friend class xiiVisualScriptNode;

  void Clear();
  void ComputeNodeDependencies();
  void ExecuteDependentNodes(xiiUInt16 uiNode);

  void ConnectExecutionPins(xiiUInt16 uiSourceNode, xiiUInt8 uiOutputSlot, xiiUInt16 uiTargetNode, xiiUInt8 uiTargetPin);
  void ConnectDataPins(xiiUInt16 uiSourceNode, xiiUInt8 uiSourcePin, xiiVisualScriptDataPinType::Enum sourcePinType, xiiUInt16 uiTargetNode, xiiUInt8 uiTargetPin, xiiVisualScriptDataPinType::Enum targetPinType);

  void                         CreateVisualScriptNode(xiiUInt32 uiNodeIdx, const xiiVisualScriptResourceDescriptor& resource);
  void                         CreateMessageSenderNode(xiiUInt32 uiNodeIdx, const xiiVisualScriptResourceDescriptor& resource);
  void                         CreateMessageHandlerNode(xiiUInt32 uiNodeIdx, const xiiVisualScriptResourceDescriptor& resource);
  void                         CreateFunctionCallNode(xiiUInt32 uiNodeIdx, const xiiVisualScriptResourceDescriptor& resource);
  xiiAbstractFunctionProperty* SearchForScriptableFunctionOnType(const xiiRTTI* pObjectType, xiiStringView sFuncName, const xiiScriptableFunctionAttribute*& out_pSfAttr) const;

  struct DataPinConnection
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt16                        m_uiTargetNode;
    xiiUInt8                         m_uiTargetPin;
    xiiVisualScriptDataPinAssignFunc m_AssignFunc  = nullptr;
    void*                            m_pTargetData = nullptr;
  };

  struct ExecPinConnection
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt16 m_uiTargetNode;
    xiiUInt8  m_uiTargetPin;
  };

  xiiVisualScriptResourceHandle                                                      m_hScriptResource;
  xiiGameObjectHandle                                                                m_hOwnerObject;
  xiiComponentHandle                                                                 m_hOwnerComponent;
  xiiWorld*                                                                          m_pWorld = nullptr;
  xiiDynamicArray<xiiVisualScriptNode*>                                              m_Nodes;
  xiiDynamicArray<xiiHybridArray<xiiUInt16, 2>>                                      m_NodeDependencies;
  xiiHashTable<xiiVisualScriptNodeConnectionID, ExecPinConnection>                   m_ExecutionConnections;
  xiiHashTable<xiiVisualScriptPinConnectionID, xiiHybridArray<DataPinConnection, 2>> m_DataConnections;
  xiiStateMap                                                                        m_LocalVariables;
  xiiVisualScriptInstanceActivity*                                                   m_pActivity        = nullptr;
  const xiiArrayMap<xiiMessageId, xiiUInt16>*                                        m_pMessageHandlers = nullptr;

  struct AssignFuncKey
  {
    XII_DECLARE_POD_TYPE();

    xiiVisualScriptDataPinType::Enum m_SourceType;
    xiiVisualScriptDataPinType::Enum m_DstType;

    XII_ALWAYS_INLINE bool operator==(const AssignFuncKey& rhs) const { return m_SourceType == rhs.m_SourceType && m_DstType == rhs.m_DstType; }

    XII_ALWAYS_INLINE bool operator<(const AssignFuncKey& rhs) const
    {
      if (m_SourceType < rhs.m_SourceType)
        return true;
      if (m_SourceType > rhs.m_SourceType)
        return false;

      return m_DstType < rhs.m_DstType;
    }
  };

  static xiiMap<AssignFuncKey, xiiVisualScriptDataPinAssignFunc> s_DataPinAssignFunctions;
};


struct XII_GAMEENGINE_DLL xiiVisualScriptInstanceActivity
{
  xiiHybridArray<xiiUInt32, 16> m_ActiveExecutionConnections;
  xiiHybridArray<xiiUInt32, 16> m_ActiveDataConnections;

  void Clear()
  {
    m_ActiveDataConnections.Clear();
    m_ActiveExecutionConnections.Clear();
  }

  bool IsEmpty() { return m_ActiveDataConnections.IsEmpty() && m_ActiveExecutionConnections.IsEmpty(); }
};
