#pragma once

#include <cstdint>
#include <type_traits>
#include <functional>

namespace phoenix {
namespace render {

/**
 * @brief 强类型句柄系统 - 防止悬空引用和类型混淆
 * @tparam T 句柄类型标签
 * @tparam IndexType 底层索引类型
 * 
 * 安全特性:
 * - 16 位索引 + 16 位代数，防止悬空引用
 * - 编译期类型检查，防止类型混淆
 * - 无效值检查，防止空句柄使用
 */
template<typename T, typename IndexType = uint32_t>
class Handle {
public:
    using index_type = IndexType;
    static constexpr IndexType INVALID_INDEX = static_cast<IndexType>(-1);
    static constexpr IndexType INDEX_MASK = static_cast<IndexType>(0xFFFF);
    static constexpr IndexType GENERATION_MASK = static_cast<IndexType>(0xFFFF0000);
    static constexpr int INDEX_BITS = 16;
    static constexpr int GENERATION_BITS = 16;

    Handle() noexcept : index_(INVALID_INDEX), generation_(0) {}
    explicit Handle(IndexType index, IndexType generation = 0) noexcept 
        : index_(index & INDEX_MASK), generation_(generation & 0xFFFF) {}

    [[nodiscard]] bool valid() const noexcept { return index_ != INVALID_INDEX; }
    [[nodiscard]] IndexType index() const noexcept { return index_; }
    [[nodiscard]] IndexType generation() const noexcept { return generation_; }
    
    [[nodiscard]] bool operator==(const Handle& other) const noexcept { 
        return index_ == other.index_ && generation_ == other.generation_; 
    }
    [[nodiscard]] bool operator!=(const Handle& other) const noexcept { 
        return !(*this == other); 
    }
    [[nodiscard]] bool operator<(const Handle& other) const noexcept { 
        return index_ < other.index_; 
    }

    explicit operator bool() const noexcept { return valid(); }
    
    /**
     * @brief 检查句柄是否与另一个句柄指向同一资源 (忽略代数)
     */
    [[nodiscard]] bool sameResource(const Handle& other) const noexcept {
        return index_ == other.index_;
    }
    
    /**
     * @brief 检查句柄是否有效且代数匹配
     */
    [[nodiscard]] bool isValidAndMatches(const Handle& other) const noexcept {
        return valid() && *this == other;
    }

private:
    IndexType index_ : 16;       // 16 位资源索引
    IndexType generation_ : 16;  // 16 位代数，防止悬空引用
};

// 渲染资源句柄类型
using DeviceHandle      = Handle<struct DeviceTag>;
using ShaderHandle      = Handle<struct ShaderTag>;
using ProgramHandle     = Handle<struct ProgramTag>;
using BufferHandle      = Handle<struct BufferTag>;
using TextureHandle     = Handle<struct TextureTag>;
using FrameBufferHandle = Handle<struct FrameBufferTag>;
using VertexLayoutHandle= Handle<struct VertexLayoutTag>;
using UniformHandle     = Handle<struct UniformTag>;

/**
 * @brief 顶点属性格式
 */
enum class VertexAttribFormat : uint8_t {
    Float1,
    Float2,
    Float3,
    Float4,
    Uint8,
    Uint8_4,
    Int16,
    Int16_4,
    Int32,
    Int32_4
};

/**
 * @brief 顶点属性语义
 */
enum class VertexAttribSemantic : uint8_t {
    Position,
    Normal,
    Tangent,
    Bitangent,
    Color0,
    Color1,
    TexCoord0,
    TexCoord1,
    TexCoord2,
    TexCoord3,
    Indices,
    Weight,
    Custom0,
    Custom1,
    Custom2,
    Custom3,
    Custom4,
    Custom5,
    Custom6,
    Custom7
};

/**
 * @brief 顶点属性描述
 */
struct VertexAttrib {
    VertexAttribSemantic semantic;
    uint8_t stream;
    VertexAttribFormat format;
    bool normalized;
    uint8_t offset;
};

/**
 * @brief 缓冲区类型
 */
enum class BufferType : uint8_t {
    Vertex,
    Index,
    Uniform,
    Storage,
    Index16,
    Index32
};

/**
 * @brief 纹理类型
 */
enum class TextureType : uint8_t {
    Texture2D,
    Texture3D,
    TextureCube,
    Texture2DArray,
    Texture2DMS
};

/**
 * @brief 纹理格式
 */
enum class TextureFormat : uint8_t {
    RGBA8,
    RGBA16F,
    RGBA32F,
    RGB10A2,
    R8,
    R16F,
    R32F,
    DepthStencil,
    Depth16,
    Depth24,
    Depth32F,
    BC1,
    BC3,
    BC5,
    BC7,
    ETC1,
    ETC2,
    ASTC4x4,
    ASTC8x8
};

/**
 * @brief 混合因子
 */
enum class BlendFactor : uint8_t {
    Zero,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DstAlpha,
    InvDstAlpha,
    DstColor,
    InvDstColor,
    SrcAlphaSaturate,
    BlendColor,
    InvBlendColor
};

/**
 * @brief 混合操作
 */
enum class BlendOp : uint8_t {
    Add,
    Sub,
    RevSub,
    Min,
    Max
};

/**
 * @brief 深度测试函数
 */
enum class DepthFunc : uint8_t {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

/**
 * @brief 模板测试函数
 */
enum class StencilFunc : uint8_t {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

/**
 * @brief 模板操作
 */
enum class StencilOp : uint8_t {
    Zero,
    Keep,
    Replace,
    Incr,
    Decr,
    Invert,
    IncrWrap,
    DecrWrap
};

/**
 * @brief 剔除模式
 */
enum class CullMode : uint8_t {
    None,
    Front,
    Back
};

/**
 * @brief 正面顺序
 */
enum class FrontFace : uint8_t {
    CW,
    CCW
};

/**
 * @brief 渲染状态描述
 */
struct RenderState {
    // 混合状态
    bool blendEnable = false;
    BlendFactor srcBlend = BlendFactor::One;
    BlendFactor dstBlend = BlendFactor::Zero;
    BlendOp blendOp = BlendOp::Add;
    BlendFactor srcBlendAlpha = BlendFactor::One;
    BlendFactor dstBlendAlpha = BlendFactor::Zero;
    BlendOp blendOpAlpha = BlendOp::Add;
    uint8_t blendMask = 0xFF;

    // 深度状态
    bool depthTest = true;
    bool depthWrite = true;
    DepthFunc depthFunc = DepthFunc::LessEqual;

    // 模板状态
    bool stencilTest = false;
    uint8_t stencilReadMask = 0xFF;
    uint8_t stencilWriteMask = 0xFF;
    StencilFunc stencilFunc = StencilFunc::Always;
    StencilOp stencilPass = StencilOp::Keep;
    StencilOp stencilFail = StencilOp::Keep;
    StencilOp stencilDepthFail = StencilOp::Keep;
    int32_t stencilRef = 0;

    // 光栅化状态
    CullMode cullMode = CullMode::Back;
    FrontFace frontFace = FrontFace::CCW;
    bool scissorTest = false;

    // 默认状态
    static RenderState defaultState() {
        return RenderState{};
    }

    static RenderState opaqueState() {
        RenderState state;
        state.blendEnable = false;
        state.depthTest = true;
        state.depthWrite = true;
        state.cullMode = CullMode::Back;
        return state;
    }

    static RenderState transparentState() {
        RenderState state;
        state.blendEnable = true;
        state.srcBlend = BlendFactor::SrcAlpha;
        state.dstBlend = BlendFactor::InvSrcAlpha;
        state.depthTest = true;
        state.depthWrite = false;
        state.cullMode = CullMode::Back;
        return state;
    }
};

/**
 * @brief 视口
 */
struct Viewport {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

/**
 * @brief 矩形区域 (用于 scissor)
 */
struct Rect {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

/**
 * @brief 颜色值
 */
struct Color {
    float r, g, b, a;

    Color() : r(0), g(0), b(0), a(1) {}
    Color(float r_, float g_, float b_, float a_ = 1.0f) 
        : r(r_), g(g_), b(b_), a(a_) {}

    static Color black() { return Color(0, 0, 0, 1); }
    static Color white() { return Color(1, 1, 1, 1); }
    static Color red() { return Color(1, 0, 0, 1); }
    static Color green() { return Color(0, 1, 0, 1); }
    static Color blue() { return Color(0, 0, 1, 1); }
};

/**
 * @brief 清除标志
 */
enum class ClearFlags : uint8_t {
    None = 0,
    Color = 1 << 0,
    Depth = 1 << 1,
    Stencil = 1 << 2,
    All = Color | Depth | Stencil
};

inline ClearFlags operator|(ClearFlags a, ClearFlags b) {
    return static_cast<ClearFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline ClearFlags operator&(ClearFlags a, ClearFlags b) {
    return static_cast<ClearFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

/**
 * @brief 提交标志
 */
enum class SubmitFlags : uint8_t {
    None = 0,
    Stereo = 1 << 0,
    Capture = 1 << 1,
    Compute = 1 << 2
};

inline SubmitFlags operator|(SubmitFlags a, SubmitFlags b) {
    return static_cast<SubmitFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

} // namespace render
} // namespace phoenix
