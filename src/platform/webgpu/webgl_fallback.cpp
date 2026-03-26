#pragma once

/**
 * @file webgl_fallback.cpp
 * @brief WebGL 2.0 后备方案 - 当 WebGPU 不可用时使用
 * 
 * 本文件提供 WebGL 2.0 渲染后端，作为 WebGPU 的后备方案。
 * 支持所有主要浏览器，包括不支持 WebGPU 的旧版本。
 */

#include <GLES3/gl32.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <string>
#include <vector>
#include <functional>

namespace phoenix {
namespace platform {
namespace webgl {

// ============================================================================
// WebGL 配置
// ============================================================================

struct WebGLConfig {
    bool enableExtensions = true;
    bool enableVertexArrayObjects = true;
    bool enableInstancedArrays = true;
    bool enableDrawBuffers = true;
    bool enableColorFloat = true;
    bool enableDepthTexture = true;
    bool enableLoseContext = true;
    bool enableDebugRendererInfo = true;
    
    uint32_t maxTextureSize = 16384;
    uint32_t maxCubeMapTextureSize = 16384;
    uint32_t maxViewportDims = 16384;
    uint32_t maxArrayTextureLayers = 256;
    uint32_t maxVertexUniformVectors = 4096;
    uint32_t maxFragmentUniformVectors = 4096;
    uint32_t maxVertexAttribs = 16;
    uint32_t maxDrawBuffers = 8;
    
    bool antialias = true;
    bool alpha = true;
    bool depth = true;
    bool stencil = false;
    bool premultipliedAlpha = true;
    bool preserveDrawingBuffer = false;
};

// ============================================================================
// WebGL 扩展
// ============================================================================

struct WebGLExtensions {
    bool EXT_color_buffer_float = false;
    bool EXT_color_buffer_half_float = false;
    bool EXT_disjoint_timer_query = false;
    bool EXT_disjoint_timer_query_webgl2 = false;
    bool EXT_float_blend = false;
    bool EXT_frag_depth = false;
    bool EXT_shader_texture_lod = false;
    bool EXT_sRGB = false;
    bool EXT_texture_compression_bptc = false;
    bool EXT_texture_compression_rgtc = false;
    bool EXT_texture_filter_anisotropic = false;
    bool KHR_parallel_shader_compile = false;
    bool OES_element_index_uint = false;
    bool OES_fbo_render_mipmap = false;
    bool OES_standard_derivatives = false;
    bool OES_texture_float = false;
    bool OES_texture_float_linear = false;
    bool OES_texture_half_float = false;
    bool OES_texture_half_float_linear = false;
    bool OES_vertex_array_object = false;
    bool WEBGL_blend_func_extended = false;
    bool WEBGL_color_buffer_float = false;
    bool WEBGL_compressed_texture_astc = false;
    bool WEBGL_compressed_texture_etc = false;
    bool WEBGL_compressed_texture_etc1 = false;
    bool WEBGL_compressed_texture_pvrtc = false;
    bool WEBGL_compressed_texture_s3tc = false;
    bool WEBGL_compressed_texture_s3tc_srgb = false;
    bool WEBGL_debug_renderer_info = false;
    bool WEBGL_debug_shaders = false;
    bool WEBGL_depth_texture = false;
    bool WEBGL_draw_buffers = false;
    bool WEBGL_lose_context = false;
    bool WEBGL_multi_draw = false;
    bool WEBGL_provoking_vertex = false;
};

// ============================================================================
// WebGL 渲染设备
// ============================================================================

class WebGLRenderDevice {
public:
    WebGLRenderDevice();
    ~WebGLRenderDevice();
    
    /**
     * @brief 初始化设备
     */
    bool initialize(const WebGLConfig& config);
    
    /**
     * @brief 关闭设备
     */
    void shutdown();
    
    /**
     * @brief 检查设备是否有效
     */
    bool isValid() const { return initialized_; }
    
    /**
     * @brief 获取 WebGL 上下文
     */
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE getContext() const { return context_; }
    
    /**
     * @brief 获取扩展
     */
    const WebGLExtensions& getExtensions() const { return extensions_; }
    
    /**
     * @brief 获取 GL 版本信息
     */
    const std::string& getGLVersion() const { return glVersion_; }
    const std::string& getGLSLVersion() const { return glslVersion_; }
    const std::string& getRenderer() const { return renderer_; }
    const std::string& getVendor() const { return vendor_; }
    
    /**
     * @brief 获取能力
     */
    uint32_t getMaxTextureSize() const { return config_.maxTextureSize; }
    uint32_t getMaxVertexAttribs() const { return config_.maxVertexAttribs; }
    uint32_t getMaxVertexUniforms() const { return config_.maxVertexUniformVectors; }
    uint32_t getMaxFragmentUniforms() const { return config_.maxFragmentUniformVectors; }
    
    // ==================== 上下文管理 ====================
    
    /**
     * @brief 创建 WebGL 上下文
     */
    bool createContext(int32_t canvasId = 0);
    
    /**
     * @brief 使上下文当前
     */
    bool makeCurrent();
    
    /**
     * @brief 释放上下文
     */
    void releaseContext();
    
    /**
     * @brief 调整画布大小
     */
    void resizeCanvas(uint32_t width, uint32_t height);
    
    // ==================== 着色器 ====================
    
    /**
     * @brief 创建着色器
     */
    GLuint createShader(GLenum type, const char* source);
    
    /**
     * @brief 创建程序
     */
    GLuint createProgram(GLuint vertexShader, GLuint fragmentShader);
    
    /**
     * @brief 创建程序 (带几何着色器)
     */
    GLuint createProgram(GLuint vertexShader, GLuint geometryShader, GLuint fragmentShader);
    
    /**
     * @brief 删除程序
     */
    void deleteProgram(GLuint program);
    
    /**
     * @brief 使用程序
     */
    void useProgram(GLuint program);
    
    // ==================== 缓冲区 ====================
    
    /**
     * @brief 创建缓冲区
     */
    GLuint createBuffer();
    
    /**
     * @brief 绑定缓冲区
     */
    void bindBuffer(GLenum target, GLuint buffer);
    
    /**
     * @brief 填充缓冲区数据
     */
    void bufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
    
    /**
     * @brief 更新缓冲区数据
     */
    void bufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data);
    
    /**
     * @brief 删除缓冲区
     */
    void deleteBuffer(GLuint buffer);
    
    // ==================== 顶点数组 ====================
    
    /**
     * @brief 创建顶点数组对象
     */
    GLuint createVertexArray();
    
    /**
     * @brief 绑定顶点数组对象
     */
    void bindVertexArray(GLuint vao);
    
    /**
     * @brief 删除顶点数组对象
     */
    void deleteVertexArray(GLuint vao);
    
    /**
     * @brief 启用顶点属性
     */
    void enableVertexAttribArray(GLuint index);
    
    /**
     * @brief 设置顶点属性指针
     */
    void vertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
    
    /**
     * @brief 设置顶点属性除数 (实例化)
     */
    void vertexAttribDivisor(GLuint index, GLuint divisor);
    
    // ==================== 纹理 ====================
    
    /**
     * @brief 创建纹理
     */
    GLuint createTexture();
    
    /**
     * @brief 绑定纹理
     */
    void bindTexture(GLenum target, GLuint texture);
    
    /**
     * @brief 设置 2D 纹理图像
     */
    void texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, 
                    GLint border, GLenum format, GLenum type, const void* pixels);
    
    /**
     * @brief 设置纹理参数
     */
    void texParameteri(GLenum target, GLenum pname, GLint param);
    
    /**
     * @brief 生成 Mipmaps
     */
    void generateMipmap(GLenum target);
    
    /**
     * @brief 删除纹理
     */
    void deleteTexture(GLuint texture);
    
    // ==================== 帧缓冲 ====================
    
    /**
     * @brief 创建帧缓冲
     */
    GLuint createFramebuffer();
    
    /**
     * @brief 绑定帧缓冲
     */
    void bindFramebuffer(GLenum target, GLuint framebuffer);
    
    /**
     * @brief 附加纹理到帧缓冲
     */
    void framebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    
    /**
     * @brief 检查帧缓冲状态
     */
    GLenum checkFramebufferStatus(GLenum target);
    
    /**
     * @brief 删除帧缓冲
     */
    void deleteFramebuffer(GLuint framebuffer);
    
    // ==================== 渲染状态 ====================
    
    /**
     * @brief 设置视口
     */
    void viewport(GLint x, GLint y, GLsizei width, GLsizei height);
    
    /**
     * @brief 设置清除颜色
     */
    void clearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    
    /**
     * @brief 清除缓冲区
     */
    void clear(GLbitfield mask);
    
    /**
     * @brief 启用/禁用功能
     */
    void enable(GLenum cap);
    void disable(GLenum cap);
    
    /**
     * @brief 设置混合
     */
    void blendFunc(GLenum sfactor, GLenum dfactor);
    void blendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    
    /**
     * @brief 设置深度测试
     */
    void depthFunc(GLenum func);
    void depthMask(GLboolean flag);
    
    /**
     * @brief 设置面剔除
     */
    void cullFace(GLenum mode);
    void frontFace(GLenum mode);
    
    // ==================== 绘制 ====================
    
    /**
     * @brief 绘制数组
     */
    void drawArrays(GLenum mode, GLint first, GLsizei count);
    
    /**
     * @brief 绘制元素
     */
    void drawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);
    
    /**
     * @brief 绘制数组实例化
     */
    void drawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount);
    
    /**
     * @brief 绘制元素实例化
     */
    void drawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instanceCount);
    
    // ==================== Uniforms ====================
    
    /**
     * @brief 获取 Uniform 位置
     */
    GLint getUniformLocation(GLuint program, const char* name);
    
    /**
     * @brief 设置 Uniform
     */
    void uniform1i(GLint location, GLint v0);
    void uniform2i(GLint location, GLint v0, GLint v1);
    void uniform3i(GLint location, GLint v0, GLint v1, GLint v2);
    void uniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
    void uniform1f(GLint location, GLfloat v0);
    void uniform2f(GLint location, GLfloat v0, GLfloat v1);
    void uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
    void uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    void uniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    
    // ==================== 查询 ====================
    
    /**
     * @brief 创建查询
     */
    GLuint createQuery();
    
    /**
     * @brief 开始查询
     */
    void beginQuery(GLenum target, GLuint id);
    
    /**
     * @brief 结束查询
     */
    void endQuery(GLenum target);
    
    /**
     * @brief 获取查询结果
     */
    void getQueryObjectuiv(GLuint id, GLenum pname, GLuint* params);
    
    /**
     * @brief 删除查询
     */
    void deleteQuery(GLuint id);
    
    // ==================== 调试 ====================
    
    /**
     * @brief 插入调试标记
     */
    void insertEventMarker(GLsizei length, const char* marker);
    
    /**
     * @brief 推送调试组
     */
    void pushGroupMarker(GLsizei length, const char* marker);
    
    /**
     * @brief 弹出调试组
     */
    void popGroupMarker();
    
    // ==================== 错误处理 ====================
    
    /**
     * @brief 获取错误
     */
    GLenum getError();
    
    /**
     * @brief 检查错误并打印
     */
    void checkError(const char* location);

private:
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context_ = 0;
    WebGLConfig config_;
    WebGLExtensions extensions_;
    std::string glVersion_;
    std::string glslVersion_;
    std::string renderer_;
    std::string vendor_;
    bool initialized_ = false;
    
    bool detectExtensions();
    bool initGLInfo();
};

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 检查 WebGL 支持
 */
inline bool isWebGLSupported() {
    return emscripten_webgl_is_supported();
}

/**
 * @brief 检查 WebGL 2.0 支持
 */
inline bool isWebGL2Supported() {
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;
    return emscripten_webgl_get_supported_context_size(&attrs) > 0;
}

/**
 * @brief 获取当前画布 ID
 */
inline int32_t getCurrentCanvas() {
    return emscripten_webgpu_get_current_canvas();
}

} // namespace webgl
} // namespace platform
} // namespace phoenix
