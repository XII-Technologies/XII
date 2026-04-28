/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>

class xiiVisualScriptCompiler
{
public:
  xiiVisualScriptCompiler(xiiVisualScriptNodeManager& ref_nodeManager);
  ~xiiVisualScriptCompiler();

  void InitModule(xiiStringView sBaseClassName, xiiStringView sScriptClassName);

  xiiResult AddFunction(xiiStringView sName, const xiiDocumentObject* pEntryObject, const xiiDocumentObject* pParentObject = nullptr);

  xiiResult Compile(xiiStringView sDebugAstOutputPath = xiiStringView());

  struct CompiledFunction
  {
    xiiString                                       m_sName;
    xiiEnum<xiiVisualScriptNodeDescription::Type>   m_Type;
    xiiEnum<xiiScriptCoroutineCreationMode>         m_CoroutineCreationMode;
    xiiDynamicArray<xiiVisualScriptNodeDescription> m_NodeDescriptions;
    xiiVisualScriptDataDescription                  m_LocalDataDesc;
  };

  struct CompiledModule
  {
    CompiledModule();

    xiiResult Serialize(xiiStreamWriter& inout_stream) const;

    xiiString                            m_sBaseClassName;
    xiiString                            m_sScriptClassName;
    xiiHybridArray<CompiledFunction, 16> m_Functions;

    xiiVisualScriptDataDescription     m_InstanceDataDesc;
    xiiVisualScriptInstanceDataMapping m_InstanceDataMapping;

    xiiVisualScriptDataDescription      m_ConstantDataDesc;
    xiiVisualScriptDataStorage          m_ConstantDataStorage;
    xiiHashTable<xiiVariant, xiiUInt32> m_ConstantDataToIndex;
  };

  const CompiledModule& GetCompiledModule() const { return m_Module; }

  using DataOffset = xiiVisualScriptDataDescription::DataOffset;
  struct AstNode;

  struct ExecInput
  {
    XII_DECLARE_POD_TYPE();

    AstNode*  m_pSourceNode      = nullptr;
    xiiUInt32 m_uiSourcePinIndex = 0;
#if XII_ENABLED(XII_PLATFORM_64BIT)
    xiiUInt32 m_uiPadding = 0;
#endif
  };

  struct ExecOutput
  {
    XII_DECLARE_POD_TYPE();

    AstNode* m_pTargetNode = nullptr;
  };

  struct DataInput
  {
    XII_DECLARE_POD_TYPE();

    AstNode*   m_pSourceNode      = nullptr;
    xiiUInt32  m_uiSourcePinIndex = 0;
    DataOffset m_DataOffset;

    XII_ALWAYS_INLINE bool IsConnected() const { return m_pSourceNode != nullptr; }
    XII_ALWAYS_INLINE bool IsConnectedAndLocal() const { return IsConnected() && m_DataOffset.GetSource() == DataOffset::Source::Local; }
  };

  struct DataOutput
  {
    XII_DECLARE_POD_TYPE();

    DataOffset m_DataOffset;

    XII_ALWAYS_INLINE bool IsValid() const { return m_DataOffset.IsValid(); }
    XII_ALWAYS_INLINE bool IsValidAndLocal() const { return IsValid() && m_DataOffset.GetSource() == DataOffset::Source::Local; }
  };

  struct AstNode
  {
    const xiiDocumentObject* m_pObject = nullptr;

    xiiEnum<xiiVisualScriptNodeDescription::Type> m_Type;
    xiiEnum<xiiVisualScriptDataType>              m_DeductedDataType;
    bool                                          m_bImplicitExecution = false;

    xiiHashedString m_sTargetTypeName;
    xiiVariant      m_Value;

    xiiSmallArray<ExecInput, 2>  m_ExecInputs;
    xiiSmallArray<ExecOutput, 2> m_ExecOutputs;
    xiiSmallArray<DataInput, 7>  m_DataInputs;
    xiiSmallArray<DataOutput, 4> m_DataOutputs;
  };

#if XII_ENABLED(XII_PLATFORM_64BIT)
  static_assert(sizeof(AstNode) == 272);
#endif

private:
  XII_ALWAYS_INLINE static xiiStringView GetNiceTypeName(const xiiDocumentObject* pObject)
  {
    return xiiVisualScriptNodeManager::GetNiceTypeName(pObject);
  }

  // Ast node creation
  AstNode&                   CreateAstNode(xiiVisualScriptNodeDescription::Type::Enum type, xiiVisualScriptDataType::Enum deductedDataType = xiiVisualScriptDataType::Invalid, bool bImplicitExecution = false);
  XII_ALWAYS_INLINE AstNode& CreateAstNode(xiiVisualScriptNodeDescription::Type::Enum type, bool bImplicitExecution)
  {
    return CreateAstNode(type, xiiVisualScriptDataType::Invalid, bImplicitExecution);
  }

  AstNode&  CreateJumpNode(AstNode* pTargetNode);
  AstNode*  CreateAstNodeFromObject(const xiiDocumentObject* pObject, const xiiVisualScriptNodeRegistry::NodeDesc* pNodeDesc, const xiiDocumentObject* pEntryObject, bool bImplicitOnly = false);
  DataInput GetOrCreateDefaultPointerNode(const AstNode& node, const xiiRTTI* pRtti);

  void MarkAsCoroutine(AstNode* pEntryAstNode);

  // Pins, inputs and outputs
  void        AddConstantDataInput(AstNode& node, const xiiVariant& value);
  xiiResult   AddConstantDataInput(AstNode& node, const xiiDocumentObject* pObject, const xiiVisualScriptPin* pPin, xiiVisualScriptDataType::Enum dataType);
  void        AddDataInput(AstNode& node, AstNode* pSourceNode, xiiUInt32 uiSourcePinIndex, xiiVisualScriptDataType::Enum dataType);
  void        AddDataOutput(AstNode& node, xiiVisualScriptDataType::Enum dataType);
  DataOutput& GetDataOutputFromInput(const DataInput& dataInput);

  void ConnectExecution(AstNode& sourceNode, AstNode& targetNode, xiiUInt32 uiSourcePinIndex = xiiInvalidIndex);
  void DisconnectExecution(AstNode& sourceNode, AstNode& targetNode, xiiUInt32 uiSourcePinIndex);
  void ExecuteBefore(AstNode& node, AstNode& firstNewNode, AstNode& lastNewNode);
  void ExecuteAfter(AstNode& node, AstNode& firstNewNode, AstNode& lastNewNode);
  void ReplaceExecution(AstNode& oldNode, AstNode& newNode);

  DataOffset GetInstanceDataOffset(xiiHashedString sName, xiiVisualScriptDataType::Enum dataType);

  // Compilation steps
  xiiResult BuildInstanceDataMapping();
  AstNode*  BuildExecutionFlow(const xiiDocumentObject* pEntryObject);

  xiiResult BuildDataStack(AstNode* pEntryAstNode, AstNode*& out_pFirstDataNode, AstNode*& out_pLastDataNode);
  xiiResult BuildDataExecutions(AstNode* pEntryAstNode);

  xiiResult InsertTypeConversions(AstNode* pEntryAstNode);

  xiiResult ReplaceLoop(AstNode* pEntryAstNode);
  xiiResult ReplaceUnsupportedNodes(AstNode* pEntryAstNode);

  xiiResult AssignInstanceVariables(AstNode* pEntryAstNode);
  xiiResult AssignLocalVariables(AstNode* pEntryAstNode, xiiVisualScriptDataDescription& inout_localDataDesc);
  xiiResult CopyOutputsToInputs(AstNode* pEntryAstNode);

  xiiResult BuildNodeDescriptions(AstNode* pEntryAstNode, xiiDynamicArray<xiiVisualScriptNodeDescription>& out_NodeDescriptions);

  xiiResult FinalizeConstantData();

  enum class VisitorResult
  {
    Continue,
    Skip,
    Error,
  };

  // Does allow modifications to the AST structure while iterating
  xiiResult TraverseAstDepthFirst(AstNode* pEntryAstNode, xiiDelegate<VisitorResult(AstNode*& pAstNode)> func);

  // Does NOT allow modifications to the AST structure while iterating
  xiiResult TraverseAstTopologicalOrder(const AstNode* pEntryAstNode, xiiDelegate<VisitorResult(const AstNode* pAstNode)> func);

  void DumpAST(AstNode* pEntryAstNode, xiiStringView sOutputPath, xiiStringView sFunctionName, xiiStringView sSuffix);
  void DumpGraph(xiiArrayPtr<const xiiVisualScriptNodeDescription> nodeDescriptions, xiiStringView sOutputPath, xiiStringView sFunctionName, xiiStringView sSuffix);

  xiiVisualScriptNodeManager& m_NodeManager;

  xiiDeque<AstNode>                           m_AstNodes;
  xiiHybridArray<const xiiDocumentObject*, 8> m_EntryObjects;

  struct LiveLocalVar
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32  m_uiId = xiiInvalidIndex;
    DataOffset m_DataOffset;

    xiiUInt32 m_uiStart = xiiInvalidIndex;
    xiiUInt32 m_uiEnd   = 0;
  };

  struct CompilationState
  {
    xiiHashTable<const xiiDocumentObject*, AstNode*> m_ExecObjectToAstNode;
    xiiHashTable<const xiiDocumentObject*, AstNode*> m_DataObjectToAstNode;

    xiiHashSet<const AstNode*> m_VisitedNodes;

    xiiDynamicArray<LiveLocalVar> m_LiveLocalVars;
    xiiUInt32                     m_uiNextLocalVarId = 0;

    AstNode* m_pGetScriptOwnerNode = nullptr;

    void Clear()
    {
      m_ExecObjectToAstNode.Clear();
      m_DataObjectToAstNode.Clear();

      m_VisitedNodes.Clear();

      m_LiveLocalVars.Clear();
      m_uiNextLocalVarId = 0;

      m_pGetScriptOwnerNode = nullptr;
    }
  };

  CompilationState m_CompilationState;

  CompiledModule m_Module;
};
