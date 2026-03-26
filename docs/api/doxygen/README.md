# Doxygen API 文档

Phoenix Engine 的完整 API 参考文档，使用 Doxygen 生成。

## 📋 生成文档

### 前置要求

```bash
# Ubuntu/Debian
sudo apt-get install doxygen graphviz

# macOS
brew install doxygen graphviz

# Windows
# 从 https://www.doxygen.nl/download.html 下载
```

### 生成 HTML 文档

```bash
cd docs/api/doxygen
doxygen Doxyfile
# 输出在 html/ 目录
```

### 生成 PDF 文档

```bash
cd docs/api/doxygen
doxygen Doxyfile
cd latex
make
# 输出 refman.pdf
```

## 📁 Doxyfile 配置

```
# Doxyfile 1.9.8

# 项目设置
PROJECT_NAME           = "Phoenix Engine"
PROJECT_NUMBER         = "1.0.0"
PROJECT_BRIEF          = "高性能跨平台 3D 渲染引擎"

# 输入设置
INPUT                  = ../../include
RECURSIVE              = YES
FILE_PATTERNS          = *.h *.hpp

# 输出设置
GENERATE_HTML          = YES
GENERATE_LATEX         = YES
GENERATE_XML           = YES
HTML_OUTPUT            = html
LATEX_OUTPUT           = latex

# 文档优化
EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = NO
EXTRACT_STATIC         = YES
SOURCE_BROWSER         = YES
INLINE_SOURCES         = NO
STRIP_CODE_COMMENTS    = NO

# 图表
HAVE_DOT               = YES
GENERATE_CLASSGRAPH    = YES
GENERATE_MODULEGRAPH   = YES
GENERATE_INCLUDEGRAPH  = YES
CALL_GRAPH             = YES
CALLER_GRAPH           = YES

# 搜索功能
SEARCHENGINE           = YES
SERVER_BASED_SEARCH    = NO

# 警告
WARNINGS               = YES
WARN_IF_UNDOCUMENTED   = YES
WARN_IF_DOC_ERROR      = YES
WARN_NO_PARAMDOC       = YES
```

## 📊 API 模块

### 核心模块
- **Platform** - 平台抽象层
- **Render** - 渲染系统
- **Scene** - 场景管理
- **Resource** - 资源管理
- **Math** - 数学库
- **Mobile** - 移动端优化

### 命名空间
- `phoenix::platform` - 平台相关功能
- `phoenix::render` - 渲染功能
- `phoenix::scene` - 场景功能
- `phoenix::resource` - 资源功能
- `phoenix::math` - 数学功能

## 🔍 搜索功能

Doxygen 生成的文档包含完整的搜索功能：
- 类名搜索
- 函数名搜索
- 变量搜索
- 全文搜索

## 📈 类图

Doxygen 使用 Graphviz 生成：
- 类继承图
- 协作图
- 包含关系图
- 调用关系图

## 📝 文档规范

### 类文档示例

```cpp
/// @brief 渲染设备基类
/// @namespace phoenix::render
/// 
/// 提供跨平台的渲染设备抽象，支持 Vulkan、Metal、DX12 和 WebGPU
/// 
/// @example examples/basic-render.cpp
/// @see RenderDevice, Pipeline, Shader
class RenderDevice {
public:
    /// @brief 初始化渲染设备
    /// @param config 设备配置
    /// @return 成功返回 true，失败返回 false
    virtual bool initialize(const DeviceConfig& config) = 0;
    
    /// @brief 创建图形管线
    /// @param desc 管线描述
    /// @return 新创建的管线对象
    virtual Pipeline* createPipeline(const PipelineDesc& desc) = 0;
};
```

### 函数文档示例

```cpp
/// @brief 创建纹理资源
/// @param path 纹理文件路径
/// @param flags 加载标志
/// @return 纹理对象指针，失败返回 nullptr
/// 
/// @note 支持的格式：PNG, JPG, BMP, TGA, DDS
/// @warning 大纹理 (>4096) 可能影响性能
/// 
/// @code
/// Texture* tex = ResourceManager::loadTexture("textures/diffuse.png");
/// if (tex) {
///     renderer.bindTexture(tex, 0);
/// }
/// @endcode
Texture* loadTexture(const std::string& path, LoadFlags flags = LoadFlags::None);
```

## 🔗 链接

- [Doxygen 官方文档](https://www.doxygen.nl/manual/index.html)
- [Graphviz 文档](https://graphviz.org/documentation/)

---
*最后更新：2026-03-26*
