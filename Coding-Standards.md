# Coding Guidelines

The coding guidelines in XII Engine are enforced through clang-tidy. You can either run clang-tidy locally or use the automated CI process that runs in every PR into XII Engine. CI will provide a git patch with all suggested changes which you can apply locally. After applying the suggested changes please make sure everything still compiles. The fixes done by clang-tidy are not garantueed to work in all cases.

## Type dependent prefixes 

These type dependent prefix is currently only mandatory for member variables. Function / Method parameter names and variables in Function / Method bodies can be named to the programmers liking.

 * If the variable / member is a `xiiInt8`, `xiiInt16`, `xiiInt32`, `xiiInt64`, `xiiAtomicInteger32`, `xiiAtomicInteger64`, `ptrdiff_t` or any other signed integer type the prefix is 'i': `xiiInt32 iMyVar;`
 * If the variable / member is a `xiiUint8`, `xiiUInt16,` `xiiUInt32`, `xiiUInt64`, `size_t` or any other unsigned integer type the prefix is 'ui': `xiiUInt32 uiMyVar;`
 * If the variable / member is a `float` or `double` the prefix is 'f': `float fMyVar;`
 * If the variable / member is a `bool` or `xiiAtomicBool` the prefix is 'b': `bool bMyVar;`
 * If the variable / member is a handle, the prefix is 'h': `xiiSpatialDataHandle hMyVar;`
 * If the variable / member is a raw pointer, `xiiSharedPtr`,`xiiUniquePtr`,`std::shared_ptr`,`std::unique_ptr` or `QPointer` the prefix is 'p': `xiiUInt32* pMyVar;`
 * If the variable / member is a `const char*` the prefix is 'sz' if it represent a zero terminated string, 'p' otherwise: `const char* szMyVar;`
 * If the variable / member is a XII Engine string (`xiiString`, `xiiStringView`, etc) the prefix is 's': `xiiString sMyVar;`
 * If the variable / member is a XII Engine vector (`xiiVec3`, `xiiVec4`, `xiiSimdVec4f`, etc) the prefix is 'v': `xiiVec3 vMyVar;`
 * If the variable / member is a XII Engine quaternion (`xiiQuat`, `xiiQuatd`, `xiiSimdQuat`) the prefix is 'q': `eqQuat qMyVar;`
 * If the variable / member is a XII Engine matrix (`xiiMat3`, `xiiMat4`, `xiiSimdMat4f`, etc) the prefix is 'm': `xiiMat3 mMyVar;`
 * If the variable / member is a fixed size array the prefix can be choosen freely. E.g. `bool m_bSomeBools[3];` or `bool m_SomeBools[3];`
 * In all other cases no prefix should be used.

## Members of structs and Classes

### Non-Static

If the member is public, no rules apply.

For private and protected members, the following rules apply:
 * All members must start with 'm_' (this comes before the type dependent prefix)
 * The name of the static member must be in PascalCase: `m_MyMember` 



 ### Static members
If the member is public, no rules apply.

For private and protected members, the following rules apply:
 * If the member is a constant, it should be marked `constexpr`: `static constexpr xiiInt32 MyConstant = 5;`
 * Otherwise the member must start with 's_' (this comes before the type dependent prefix)
 * The name of the static member must be in PascalCase: `s_MyMember` 
