// Depth of Field (DoF) Fragment Shader
// 实现基于薄透镜模型的景深效果，支持 Bokeh
#version 450 core

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 v_texcoord;

// Uniforms
layout(std140, binding = 0) uniform DoFUniforms {
    vec2 u_texelSize;           // 纹素大小
    float u_focalDistance;      // 焦距 (聚焦距离)
    float u_aperture;           // 光圈大小 (f-stop)
    float u_focalLength;        // 焦距 (mm)
    float u_maxCoC;             // 最大弥散圆
    int u_sampleCount;          // 采样数
    float u_nearBlur;           // 前景模糊强度
    float u_farBlur;            // 背景模糊强度
    vec2 u_sensorSize;          // 传感器尺寸 (mm)
    int u_bokehShape;           // Bokeh 形状 (0=圆形，1=六边形，2=八边形)
    float u_bokehRotation;      // Bokeh 旋转
    float u_vignette;           // 渐晕效果
};

// 纹理绑定
layout(binding = 1) uniform sampler2D u_color;          // 颜色纹理
layout(binding = 2) uniform sampler2D u_depth;          // 深度纹理
layout(binding = 3) uniform sampler2D u_coC;            // CoC 纹理 (可选预计算)

// 常量
const float PI = 3.14159265359;
const float EPSILON = 1e-5;
const int MAX_SAMPLES = 64;

/**
 * @brief 从深度重建线性深度
 */
float reconstructLinearDepth(float depth) {
    // 假设深度在 [0, 1] 范围
    float near = 0.1;
    float far = 1000.0;
    return (2.0 * near * far) / (far + near - depth * (far - near));
}

/**
 * @brief 计算 Circle of Confusion (弥散圆)
 * 基于薄透镜模型
 */
float calculateCoC(float depth) {
    float linearDepth = reconstructLinearDepth(depth);
    
    // 薄透镜公式
    // CoC = A * (f * (d - F)) / (F * (d - f))
    // A = 光圈直径，f = 焦距，F = 聚焦距离，d = 物体距离
    
    float apertureDiameter = u_focalLength / u_aperture;  // 光圈直径 (mm)
    float focalLengthM = u_focalLength / 1000.0;          // 焦距 (m)
    
    // 简化的 CoC 计算
    float coc = (apertureDiameter * focalLengthM * (linearDepth - u_focalDistance)) / 
                (u_focalDistance * (linearDepth - focalLengthM));
    
    // 转换为像素单位
    coc = coc * 100.0;  // 缩放因子
    
    // 限制 CoC 范围
    return clamp(coc, -u_maxCoC, u_maxCoC);
}

/**
 * @brief 生成 Bokeh 形状
 */
float generateBokehShape(vec2 samplePos, int shape) {
    float r = length(samplePos);
    
    if (shape == 0) {
        // 圆形
        return 1.0;
    } else if (shape == 1) {
        // 六边形
        float angle = atan(samplePos.y, samplePos.x);
        float hex = abs(cos(angle)) * abs(cos(angle + PI/3.0)) * abs(cos(angle - PI/3.0));
        return smoothstep(0.4, 0.5, hex);
    } else if (shape == 2) {
        // 八边形
        float angle = atan(samplePos.y, samplePos.x);
        float oct = abs(cos(angle)) * abs(cos(angle + PI/4.0)) * 
                    abs(cos(angle + PI/2.0)) * abs(cos(angle + PI*3.0/4.0));
        return smoothstep(0.3, 0.4, oct);
    }
    
    return 1.0;
}

/**
 * @brief 泊松圆盘采样模式
 */
const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44328806, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790)
);

/**
 * @brief 高质量 DoF 渲染 (分离式模糊)
 */
vec4 renderDoF(vec2 uv, float coc) {
    float absCoC = abs(coc);
    
    // 如果 CoC 太小，不需要模糊
    if (absCoC < 0.5) {
        return texture(u_color, uv);
    }
    
    // 根据 CoC 调整采样半径
    float radius = absCoC * u_texelSize.x;
    
    vec4 result = vec4(0.0);
    float totalWeight = 0.0;
    
    // 使用泊松圆盘采样
    int samples = min(u_sampleCount, MAX_SAMPLES);
    
    for (int i = 0; i < samples; i++) {
        // 计算采样位置
        vec2 sampleOffset = poissonDisk[i] * radius;
        vec2 sampleUV = uv + sampleOffset;
        
        // 采样深度
        float sampleDepth = texture(u_depth, sampleUV).r;
        float sampleCoC = calculateCoC(sampleDepth);
        
        // 只混合相似深度的像素 (避免光晕)
        float depthWeight = 1.0;
        if (coc > 0.0) {
            // 背景模糊：只混合背景像素
            depthWeight = step(0.0, sampleCoC);
        } else {
            // 前景模糊：只混合前景像素
            depthWeight = step(sampleCoC, 0.0);
        }
        
        // Bokeh 形状
        vec2 normalizedOffset = sampleOffset / radius;
        float bokehWeight = generateBokehShape(normalizedOffset, u_bokehShape);
        
        // 应用旋转
        if (u_bokehRotation != 0.0) {
            float cosA = cos(u_bokehRotation);
            float sinA = sin(u_bokehRotation);
            vec2 rotatedOffset = vec2(
                normalizedOffset.x * cosA - normalizedOffset.y * sinA,
                normalizedOffset.x * sinA + normalizedOffset.y * cosA
            );
            bokehWeight = generateBokehShape(rotatedOffset, u_bokehShape);
        }
        
        // 采样颜色
        vec4 sampleColor = texture(u_color, sampleUV);
        
        // 高斯权重
        float gaussianWeight = exp(-0.5 * dot(normalizedOffset, normalizedOffset) / 0.5);
        
        float weight = depthWeight * bokehWeight * gaussianWeight;
        result += sampleColor * weight;
        totalWeight += weight;
    }
    
    if (totalWeight > EPSILON) {
        result /= totalWeight;
    } else {
        result = texture(u_color, uv);
    }
    
    return result;
}

/**
 * @brief 优化的 DoF (使用 mipmaps 近似)
 */
vec4 fastDoF(vec2 uv, float coc) {
    float absCoC = abs(coc);
    
    if (absCoC < 1.0) {
        return texture(u_color, uv);
    }
    
    // 使用 mipmap LOD 近似模糊
    float lod = log2(absCoC);
    vec4 blurred = textureLod(u_color, uv, lod);
    
    // 混合原始和模糊
    float blendFactor = clamp((absCoC - 1.0) / 4.0, 0.0, 1.0);
    return mix(texture(u_color, uv), blurred, blendFactor);
}

/**
 * @brief 应用渐晕效果
 */
float applyVignette(vec2 uv) {
    if (u_vignette <= 0.0) return 1.0;
    
    vec2 center = uv - 0.5;
    float dist = length(center) * 2.0;
    float vignette = 1.0 - smoothstep(0.5, 1.0, dist);
    return mix(1.0, vignette, u_vignette);
}

void main() {
    vec2 uv = v_texcoord;
    
    // 采样深度
    float depth = texture(u_depth, uv).r;
    
    // 计算 CoC
    float coc = calculateCoC(depth);
    
    // 渲染景深
    vec4 result = renderDoF(uv, coc);
    
    // 应用渐晕
    float vignetteFactor = applyVignette(uv);
    result.rgb *= vignetteFactor;
    
    fragColor = result;
}
