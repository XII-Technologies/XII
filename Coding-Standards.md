# XII Engine Coding Guidelines

The following guidelines help maintain a consistent and readable codebase for XII Engine. By following these standards, each developer quickly understands the purpose and type of any variable or class member. Enforcement is built-in through **clang-tidy** and our Continuous Integration (CI) process, which automatically inspects pull requests. When clang-tidy finds deviations, it supplies a git patch with suggested fixes. **Always verify that your project still compiles after applying those changes**, as automatic fixes are not guaranteed to work perfectly in every situation.

---

## 1. Automated Code Quality & CI Integration

**clang-tidy Enforcement**  
Clang-tidy acts as our first line of defense against inconsistencies. You can run it locally before committing changes. Alternatively, every Pull Request automatically goes through our CI, which:
- Checks for guideline compliance.
- Generates a git patch with recommended changes.
- Prompts you to review and apply these changes manually.

**Example Workflow Snippet:**

```yaml
steps:
  - uses: actions/checkout@v3
  - name: Run clang-tidy
    run: clang-tidy -p build
```

This integration ensures that even if human errors slip by, our code quality remains high.

---

## 2. Type-Dependent Prefixes

To promote clarity and avoid naming collisions, XII Engine mandates type-dependent prefixes **for member variables only**. Local variables in function bodies or parameter names remain at the programmer’s discretion. The prefix indicates the type of the variable, allowing for a glance-based understanding of its data type.

Below are the rules with some illustrative examples:

- **Signed Integers:**  
  For types such as `xiiInt8`, `xiiInt16`, `xiiInt32`, `xiiInt64`, `xiiAtomicInteger32`, `xiiAtomicInteger64`, `ptrdiff_t`, or any other signed integer type, **use the prefix `i`**.

  ```cpp
  xiiInt32 iFrameCount;  // a signed 32-bit integer representing a frame count
  ```

- **Unsigned Integers:**  
  For types like `xiiUint8`, `xiiUInt16`, `xiiUInt32`, `xiiUInt64`, `size_t`, or other unsigned integer types, **use `ui`**.

  ```cpp
  xiiUInt32 uiPixelCount;  // an unsigned integer used for counting pixels
  ```

- **Floating Point Numbers:**  
  Variables of type `float` or `double` should begin with **`f`**.

  ```cpp
  float fDeltaTime;   // delta time between frames
  ```

- **Booleans:**  
  For `bool` or `xiiAtomicBool`, the prefix is **`b`**.

  ```cpp
  bool bIsInitialized;  // a flag indicating if initialization is complete
  ```

- **Handles:**  
  Handles, such as a hypothetical `xiiSpatialDataHandle`, must have the prefix **`h`**.

  ```cpp
  xiiSpatialDataHandle hParentObject;  // a handle to a parent object in spatial data
  ```

- **Pointers:**  
  For raw pointers or smart pointers (`xiiSharedPtr`, `xiiUniquePtr`, `std::shared_ptr`, `std::unique_ptr`, `QPointer`), **use `p`**.

  ```cpp
  xiiUInt32* pVertexBuffer;   // pointer to a vertex buffer
  ```

- **C-style Strings:**  
  For `const char*`, if representing a zero-terminated string then **use `sz`**. If not, then use **`p`**.

  ```cpp
  const char* szName = "XII Engine";  // zero-terminated string for the engine name
  ```

- **XII Engine String Objects:**  
  For strings like `xiiString` or `xiiStringView`, the prefix is **`s`**.

  ```cpp
  xiiString sFilePath;  // a string holding a file path
  ```

- **Vectors:**  
  For vector types (`xiiVec3`, `xiiVec4`, `xiiSimdVec4f`, etc.), **use `v`**.

  ```cpp
  xiiVec3 vPosition;  // a 3D position vector
  ```

- **Quaternions:**  
  For quaternions (`xiiQuat`, `xiiQuatd`, `xiiSimdQuat`), **use `q`**.

  ```cpp
  xiiQuat qRotation;  // a quaternion representing orientation
  ```

- **Matrices:**  
  For matrices (`xiiMat3`, `xiiMat4`, `xiiSimdMat4f`, etc.), **use `m`**.

  ```cpp
  xiiMat3 mRotationMatrix;  // a 3x3 rotation matrix
  ```

- **Fixed Size Arrays:**  
  For fixed-size arrays, you may use a prefix of your choosing. For instance, either of these styles is acceptable:

  ```cpp
  bool m_bActiveFlags[3];   // using a prefix indicating booleans and their member status
  bool mActiveFlags[3];     // or a more relaxed choice if the array is self-explanatory
  ```

- **Default:**  
  For all other types where a type-dependent prefix does not apply, **do not use any prefix**.

---

## 3. Naming Members of Structs and Classes

Consistency in member naming helps us quickly identify where variables are declared, whether they are static, non-static, public, or private. The rules differ slightly depending on these attributes.

### 3.1 Non-Static Members

- **Public Members:**  
  There is flexibility for public non-static members. No special naming prefix is required.

- **Private and Protected Members:**  
  All private or protected non-static members **must be prefixed with `m_`**. If a type-dependent prefix is applicable, it follows the `m_`.

  **Example:**  
  Suppose you have a private signed integer member counting iterations; it should be written as:

  ```cpp
  class Timer
  {
  private:
      xiiInt32 m_iIterationCount;  // 'm_' indicates a member variable and 'i' signifies a signed integer type
  public:
      // Public function declarations...
  };
  ```

  Notice that the `m_` comes first, ensuring that all internal state variables are quickly identifiable.

### 3.2 Static Members

Static members have their own naming conventions to distinguish them from instance members.

- **Public Static Members:**  
  Public static members are exempt from the naming rules.

- **Private and Protected Static Members:**  
  - **Constants:**  
    For static constants, mark them as `constexpr` and name them in **PascalCase** (with no `m_` or `s_` prefix).

    ```cpp
    class Constants
    {
    private:
        static constexpr xiiInt32 MaxIterations = 100;  // clearly named constant
    };
    ```

  - **Non-Constant Static Members:**  
    If the static member is not a constant, it must be prefixed with `s_` (where the prefix goes before any type-dependent prefix) and must be in **PascalCase**.

    **Example:**  
    For a private static member representing a logging level, you might write:

    ```cpp
    class Logger
    {
    private:
        static xiiInt32 s_iLogLevel;  // 's_' indicates a static member, and 'i' notes a signed integer type
    public:
        // Static methods...
    };
    ```

    Notice the combined usage: the prefix `s_` signals that this member is shared across all instances, and the type-dependent prefix `i` follows, maintaining clarity.

---

## General Advice & Rationale

These guidelines aren’t just arbitrary rules, they’re designed to help you, your teammates, and future maintainers of the code. By visually encoding type information into your variable names:

- **Readability is increased:** A glance at `fDeltaTime` instantly communicates that this is a floating point variable related to time.
  
- **Maintenance is easier:** Uniform naming reduces ambiguity. When searching for member variables, the prefixes easily isolate which variables belong to a class instance or are static.
  
- **Tooling Integration:** Automated tools like clang-tidy depend on these consistent patterns to detect mistakes or enforce code style, reducing the friction of manual code reviews.

Remember, while these guidelines are enforced at the level of member variables, local variables within functions or method parameters can be named more liberally. This flexibility allows the developer to choose names that best describe the immediate context without cluttering the overall structure of the class.

---

By adhering to these conventions, you help maintain the professional quality and clarity of XII Engine's codebase. If you’re ever unsure about a naming decision, refer back to this guide or discuss it with your team. The goal is to keep the code expressive, self-documenting, and consistent.

