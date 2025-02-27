#pragma once

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>

class xiiVisualScriptCompiler
{
public:
  xiiVisualScriptCompiler();
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

    xiiVisualScriptDataDescription m_ConstantDataDesc;
    xiiVisualScriptDataStorage     m_ConstantDataStorage;
  };

  const CompiledModule& GetCompiledModule() const { return m_Module; }

  struct AstNode;

  struct DataInput
  {
    XII_DECLARE_POD_TYPE();

    AstNode*                         m_pSourceNode      = nullptr;
    xiiUInt32                        m_uiId             = 0;
    xiiUInt8                         m_uiSourcePinIndex = 0;
    xiiEnum<xiiVisualScriptDataType> m_DataType;
  };

  struct DataOutput
  {
    xiiSmallArray<AstNode*, 3>       m_TargetNodes;
    xiiUInt32                        m_uiId = 0;
    xiiEnum<xiiVisualScriptDataType> m_DataType;
  };

  struct AstNode
  {
    xiiEnum<xiiVisualScriptNodeDescription::Type> m_Type;
    xiiEnum<xiiVisualScriptDataType>              m_DeductedDataType;
    bool                                          m_bImplicitExecution = false;

    xiiHashedString m_sTargetTypeName;
    xiiVariant      m_Value;

    xiiSmallArray<AstNode*, 4>   m_Next;
    xiiSmallArray<DataInput, 5>  m_Inputs;
    xiiSmallArray<DataOutput, 2> m_Outputs;
  };

#if XII_ENABLED(XII_PLATFORM_64BIT)
  static_assert(sizeof(AstNode) == 272);
#endif

private:
  using DataOffset = xiiVisualScriptDataDescription::DataOffset;

  XII_ALWAYS_INLINE static xiiStringView GetNiceTypeName(const xiiDocumentObject* pObject)
  {
    return xiiVisualScriptNodeManager::GetNiceTypeName(pObject);
  }

  XII_ALWAYS_INLINE xiiVisualScriptDataType::Enum GetDeductedType(const xiiDocumentObject* pObject) const
  {
    return m_pManager->GetDeductedType(pObject);
  }

  xiiUInt32         GetPinId(const xiiVisualScriptPin* pPin);
  DataOutput&       GetDataOutput(const DataInput& dataInput);
  AstNode&          CreateAstNode(xiiVisualScriptNodeDescription::Type::Enum type, xiiVisualScriptDataType::Enum deductedDataType = xiiVisualScriptDataType::Invalid, bool bImplicitExecution = false);
  XII_ALWAYS_INLINE AstNode& CreateAstNode(xiiVisualScriptNodeDescription::Type::Enum type, bool bImplicitExecution)
  {
    return CreateAstNode(type, xiiVisualScriptDataType::Invalid, bImplicitExecution);
  }

  void AddDataInput(AstNode& node, AstNode* pSourceNode, xiiUInt8 uiSourcePinIndex, xiiVisualScriptDataType::Enum dataType);
  void AddDataOutput(AstNode& node, xiiVisualScriptDataType::Enum dataType);

  struct DefaultInput
  {
    AstNode* m_pSourceNode      = nullptr;
    xiiUInt8 m_uiSourcePinIndex = 0;
  };

  DefaultInput GetDefaultPointerInput(const xiiRTTI* pDataType);
  AstNode*     CreateConstantNode(const xiiVariant& value);
  AstNode*     CreateJumpNode(AstNode* pTargetNode);

  DataOffset GetInstanceDataOffset(xiiHashedString sName, xiiVisualScriptDataType::Enum dataType);

  struct ConnectionType
  {
    enum Enum
    {
      Execution,
      Data,
    };
  };

  struct Connection
  {
    AstNode*             m_pSource          = nullptr;
    AstNode*             m_pTarget          = nullptr;
    ConnectionType::Enum m_Type             = ConnectionType::Execution;
    xiiUInt32            m_uiSourcePinIndex = 0;
  };

  AstNode*  BuildAST(const xiiDocumentObject* pEntryNode);
  void      MarkAsCoroutine(AstNode* pEntryAstNode);
  xiiResult ReplaceUnsupportedNodes(AstNode* pEntryAstNode);
  xiiResult ReplaceLoop(Connection& connection);
  xiiResult InsertTypeConversions(AstNode* pEntryAstNode);
  xiiResult InlineConstants(AstNode* pEntryAstNode);
  xiiResult InlineVariables(AstNode* pEntryAstNode);
  xiiResult BuildDataStack(AstNode* pEntryAstNode, xiiDynamicArray<AstNode*>& out_Stack);
  xiiResult BuildDataExecutions(AstNode* pEntryAstNode);
  xiiResult FillDataOutputConnections(AstNode* pEntryAstNode);
  xiiResult AssignLocalVariables(AstNode* pEntryAstNode, xiiVisualScriptDataDescription& inout_localDataDesc);
  xiiResult BuildNodeDescriptions(AstNode* pEntryAstNode, xiiDynamicArray<xiiVisualScriptNodeDescription>& out_NodeDescriptions);

  struct ConnectionHasher
  {
    static xiiUInt32 Hash(const Connection& c);
    static bool      Equal(const Connection& a, const Connection& b);
  };

  enum class VisitorResult
  {
    Continue,
    Skip,
    Stop,
    Error,
  };

  using AstNodeVisitorFunc = xiiDelegate<VisitorResult(Connection& connection)>;
  xiiResult TraverseExecutionConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate = true);
  xiiResult TraverseDataConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate = true, bool bClearReportedConnections = true);
  xiiResult TraverseAllConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate = true);

  xiiResult FinalizeConstantData();

  void DumpAST(AstNode* pEntryAstNode, xiiStringView sOutputPath, xiiStringView sFunctionName, xiiStringView sSuffix);
  void DumpGraph(xiiArrayPtr<const xiiVisualScriptNodeDescription> nodeDescriptions, xiiStringView sOutputPath, xiiStringView sFunctionName, xiiStringView sSuffix);

  const xiiVisualScriptNodeManager* m_pManager = nullptr;

  xiiDeque<AstNode>                          m_AstNodes;
  xiiHybridArray<AstNode*, 8>                m_EntryAstNodes;
  xiiHashTable<const xiiRTTI*, DefaultInput> m_DefaultInputs;

  xiiHashSet<Connection, ConnectionHasher> m_ReportedConnections;

  xiiHashTable<const xiiVisualScriptPin*, xiiUInt32> m_PinToId;
  xiiUInt32                                          m_uiNextPinId = 0;

  struct DataDesc
  {
    XII_DECLARE_POD_TYPE();

    DataOffset m_DataOffset;
    xiiUInt32  m_uiUsageCounter = 0;
  };

  xiiHashTable<xiiUInt32, DataDesc> m_PinIdToDataDesc;

  xiiHashTable<xiiVariant, xiiUInt32> m_ConstantDataToIndex;

  CompiledModule m_Module;
};
