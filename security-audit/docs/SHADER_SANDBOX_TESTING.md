# Shader Sandbox Testing Configuration
# Tests shader compilation and execution in an isolated environment

## Overview

This document describes the security testing strategy for shader compilation
and execution in Phoenix Engine. Shaders are untrusted code that can potentially:
- Access unauthorized memory
- Execute infinite loops (DoS)
- Access external resources
- Leak sensitive information

## Threat Model

### Attack Vectors
1. **Malicious GLSL/HLSL Source Code**
   - Buffer overflows in shader compiler
   - Compiler bugs leading to code execution
   - Resource exhaustion (compile time/memory)

2. **Runtime Shader Execution**
   - Unauthorized memory access
   - GPU driver exploits
   - Side-channel attacks (timing, power)

3. **Resource Loading**
   - Path traversal in texture/buffer loads
   - Symlink attacks
   - Large file DoS

## Sandbox Architecture

```
┌─────────────────────────────────────────────────────┐
│                  Application Layer                   │
├─────────────────────────────────────────────────────┤
│              Shader Sandbox (WebAssembly)            │
│  ┌──────────────────────────────────────────────┐   │
│  │  Restricted glslang/shaderc API              │   │
│  │  - No file system access                     │   │
│  │  - No network access                         │   │
│  │  - Memory limits (128MB)                     │   │
│  │  - CPU time limits (5s)                      │   │
│  └──────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────┤
│              bgfx Rendering Backend                  │
└─────────────────────────────────────────────────────┘
```

## Test Cases

### 1. Shader Compilation Tests

```cpp
// test_shader_sandbox.cpp

TEST(ShaderSandboxTest, CompileValidShader) {
    const char* valid_vs = R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
        }
    )";
    
    ShaderSandbox sandbox;
    auto result = sandbox.compile_vertex_shader(valid_vs);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.errors.empty());
}

TEST(ShaderSandboxTest, RejectInfiniteLoop) {
    const char* malicious_vs = R"(
        #version 450
        void main() {
            while(true) { }  // Infinite loop
            gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
        }
    )";
    
    ShaderSandbox sandbox;
    sandbox.set_timeout(std::chrono::seconds(2));
    
    auto start = std::chrono::steady_clock::now();
    auto result = sandbox.compile_vertex_shader(malicious_vs);
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    EXPECT_LT(elapsed, std::chrono::seconds(3));  // Should timeout
}

TEST(ShaderSandboxTest, RejectLargeShader) {
    // Generate very large shader source
    std::string large_shader = "#version 450\nvoid main() {\n";
    for (int i = 0; i < 1000000; i++) {
        large_shader += "float v" + std::to_string(i) + " = " + 
                       std::to_string(i) + ".0;\n";
    }
    large_shader += "}";
    
    ShaderSandbox sandbox;
    sandbox.set_max_source_size(1024 * 1024);  // 1MB limit
    
    auto result = sandbox.compile_vertex_shader(large_shader);
    EXPECT_FALSE(result.success);
    EXPECT_THAT(result.errors, HasSubstr("source too large"));
}

TEST(ShaderSandboxTest, RejectFileOperations) {
    const char* malicious_vs = R"(
        #version 450
        #read_file("/etc/passwd")  // Attempt to read file
        void main() {
            gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
        }
    )";
    
    ShaderSandbox sandbox;
    auto result = sandbox.compile_vertex_shader(malicious_vs);
    EXPECT_FALSE(result.success);
}

TEST(ShaderSandboxTest, RejectExternalFunctions) {
    const char* malicious_vs = R"(
        #version 450
        #extension GL_ARB_external_functions : require
        void external_func();  // Attempt to call external function
        void main() {
            external_func();
            gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
        }
    )";
    
    ShaderSandbox sandbox;
    auto result = sandbox.compile_vertex_shader(malicious_vs);
    EXPECT_FALSE(result.success);
}
```

### 2. Resource Loading Tests

```cpp
// test_resource_sandbox.cpp

TEST(ResourceSandboxTest, LoadValidTexture) {
    ResourceSandbox sandbox;
    sandbox.set_allowed_directory("/assets/textures");
    
    auto texture = sandbox.load_texture("/assets/textures/test.png");
    EXPECT_TRUE(texture.valid());
}

TEST(ResourceSandboxTest, RejectPathTraversal) {
    ResourceSandbox sandbox;
    sandbox.set_allowed_directory("/assets");
    
    auto texture = sandbox.load_texture("/assets/../../../etc/passwd");
    EXPECT_FALSE(texture.valid());
    EXPECT_THAT(texture.error(), HasSubstr("path traversal"));
}

TEST(ResourceSandboxTest, RejectSymlinkAttack) {
    // Create symlink: /assets/malicious -> /etc/passwd
    ResourceSandbox sandbox;
    sandbox.set_allowed_directory("/assets");
    sandbox.set_follow_symlinks(false);
    
    auto resource = sandbox.load_file("/assets/malicious");
    EXPECT_FALSE(resource.valid());
}

TEST(ResourceSandboxTest, EnforceSizeLimit) {
    ResourceSandbox sandbox;
    sandbox.set_max_file_size(10 * 1024 * 1024);  // 10MB
    
    auto resource = sandbox.load_file("/assets/large_texture.png");
    if (resource.size() > 10 * 1024 * 1024) {
        EXPECT_FALSE(resource.valid());
        EXPECT_THAT(resource.error(), HasSubstr("too large"));
    }
}
```

### 3. Memory Safety Tests

```cpp
// test_shader_memory.cpp

TEST(ShaderMemoryTest, DetectBufferOverflow) {
    const char* shader = R"(
        #version 450
        uniform float values[10];
        void main() {
            float x = values[100];  // Out of bounds access
            gl_Position = vec4(x, 0.0, 0.0, 1.0);
        }
    )";
    
    ShaderSandbox sandbox;
    sandbox.enable_bounds_checking(true);
    
    auto result = sandbox.compile_and_validate(shader);
    EXPECT_TRUE(result.bounds_violations_detected);
}

TEST(ShaderMemoryTest, DetectUseAfterFree) {
    // This tests the shader compiler's memory handling
    // not the shader itself
    
    std::string shader_source = generate_complex_shader();
    
    ShaderSandbox sandbox;
    auto compiled = sandbox.compile_vertex_shader(shader_source.c_str());
    
    // Free the source while compiled shader still exists
    shader_source.clear();
    shader_source.shrink_to_fit();
    
    // Use the compiled shader
    auto result = sandbox.use_shader(compiled);
    EXPECT_TRUE(result.success);  // Should not crash
}
```

## Implementation Guidelines

### 1. WebAssembly Sandboxing

```rust
// shader_sandbox.rs

use wasmtime::*;

pub struct ShaderSandbox {
    engine: Engine,
    store: Store<()>,
    max_memory_bytes: usize,
    timeout: Option<Duration>,
}

impl ShaderSandbox {
    pub fn new() -> Self {
        let mut config = Config::new();
        config.consume_fuel(true);
        config.max_wasm_memory(128 * 1024 * 1024);  // 128MB limit
        
        let engine = Engine::new(&config).unwrap();
        let store = Store::new(&engine, ());
        
        ShaderSandbox {
            engine,
            store,
            max_memory_bytes: 128 * 1024 * 1024,
            timeout: Some(Duration::from_secs(5)),
        }
    }
    
    pub fn compile_shader(&mut self, source: &str) -> Result<CompiledShader, Error> {
        // Set timeout via fuel consumption
        if let Some(timeout) = self.timeout {
            self.store.set_fuel(timeout.as_millis() as u64)?;
        }
        
        // Compile with glslang in WASM
        let module = Module::from_binary(&self.engine, &self.compile_glsl(source)?)?;
        
        Ok(CompiledShader { module })
    }
    
    fn compile_glsl(&self, source: &str) -> Result<Vec<u8>, Error> {
        // Use glslang validator in WASM sandbox
        // Reject any shader that:
        // - Uses disallowed extensions
        // - Has excessive complexity
        // - Contains suspicious patterns
        ...
    }
}
```

### 2. Resource Validation

```rust
// resource_validator.rs

pub struct ResourceValidator {
    allowed_roots: Vec<PathBuf>,
    max_file_size: u64,
    follow_symlinks: bool,
    allowed_extensions: HashSet<String>,
}

impl ResourceValidator {
    pub fn validate_path(&self, path: &Path) -> Result<(), ValidationError> {
        // Check for path traversal
        let canonical = path.canonicalize()
            .map_err(|_| ValidationError::InvalidPath)?;
        
        // Verify path is within allowed roots
        let is_allowed = self.allowed_roots.iter()
            .any(|root| canonical.starts_with(root));
        
        if !is_allowed {
            return Err(ValidationError::PathTraversal);
        }
        
        // Check symlinks
        if !self.follow_symlinks && path.is_symlink() {
            return Err(ValidationError::SymlinkNotAllowed);
        }
        
        // Check file size
        let metadata = std::fs::metadata(&canonical)?;
        if metadata.len() > self.max_file_size {
            return Err(ValidationError::FileTooLarge);
        }
        
        // Check extension
        if let Some(ext) = path.extension() {
            if !self.allowed_extensions.contains(ext.to_str().unwrap_or("")) {
                return Err(ValidationError::InvalidExtension);
            }
        }
        
        Ok(())
    }
}
```

## CI/CD Integration

```yaml
# .github/workflows/shader-security.yml

name: Shader Security Tests

on: [push, pull_request]

jobs:
  shader-sandbox:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Build with sanitizers
        run: |
          cmake -DENABLE_SANITIZERS=ON \
                -DSANITIZER_TYPES="address,undefined" \
                -DCMAKE_BUILD_TYPE=Debug ..
          cmake --build .
      
      - name: Run shader sandbox tests
        run: ./phoenix_tests --gtest_filter="ShaderSandboxTest.*"
        
      - name: Run resource sandbox tests
        run: ./phoenix_tests --gtest_filter="ResourceSandboxTest.*"
        
      - name: Fuzz shader compiler
        run: |
          cd security-audit/fuzz
          cargo fuzz run shader_compile_fuzzer -- -max_total_time=60
```

## References

- [WebAssembly Security](https://webassembly.org/docs/security/)
- [glslang Validator](https://github.com/KhronosGroup/glslang)
- [WASM Sandboxing Best Practices](https://github.com/bytecodealliance/wasmtime/blob/main/docs/examples-sandboxing.md)
