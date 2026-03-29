// FXAA (Fast Approximate Anti-Aliasing) Shader
// 快速近似抗锯齿

#version 450 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform FXAAUBO {
    vec2 u_texelSize;
    float u_subpixelQuality;
    float u_edgeThreshold;
    float u_edgeThresholdMin;
    float u_iterations;
    float u_padding1;
    float u_padding2;
} fxaa;

layout(binding = 0) uniform sampler2D u_inputTexture;

// FXAA 核心算法
vec4 fxaaFilter(vec2 uv, sampler2D tex, vec2 texelSize) {
    // 采样中心像素
    vec3 rgbM = texture(tex, uv).rgb;
    
    // 采样周围像素
    vec3 rgbN = texture(tex, uv + vec2(0.0, -texelSize.y)).rgb;
    vec3 rgbS = texture(tex, uv + vec2(0.0, texelSize.y)).rgb;
    vec3 rgbW = texture(tex, uv + vec2(-texelSize.x, 0.0)).rgb;
    vec3 rgbE = texture(tex, uv + vec2(texelSize.x, 0.0)).rgb;
    vec3 rgbNW = texture(tex, uv + vec2(-texelSize.x, -texelSize.y)).rgb;
    vec3 rgbNE = texture(tex, uv + vec2(texelSize.x, -texelSize.y)).rgb;
    vec3 rgbSW = texture(tex, uv + vec2(-texelSize.x, texelSize.y)).rgb;
    vec3 rgbSE = texture(tex, uv + vec2(texelSize.x, texelSize.y)).rgb;
    
    // 计算亮度
    float lumaM = dot(rgbM, vec3(0.299, 0.587, 0.114));
    float lumaN = dot(rgbN, vec3(0.299, 0.587, 0.114));
    float lumaS = dot(rgbS, vec3(0.299, 0.587, 0.114));
    float lumaW = dot(rgbW, vec3(0.299, 0.587, 0.114));
    float lumaE = dot(rgbE, vec3(0.299, 0.587, 0.114));
    float lumaNW = dot(rgbNW, vec3(0.299, 0.587, 0.114));
    float lumaNE = dot(rgbNE, vec3(0.299, 0.587, 0.114));
    float lumaSW = dot(rgbSW, vec3(0.299, 0.587, 0.114));
    float lumaSE = dot(rgbSE, vec3(0.299, 0.587, 0.114));
    
    // 计算亮度范围
    float lumaMin = min(lumaM, min(min(lumaN, lumaS), min(lumaW, lumaE)));
    float lumaMax = max(lumaM, max(max(lumaN, lumaS), max(lumaW, lumaE)));
    float lumaRange = lumaMax - lumaMin;
    
    // 边缘检测阈值
    float edgeThreshold = max(fxaa.u_edgeThresholdMin, lumaMax * fxaa.u_edgeThreshold);
    
    // 如果不是边缘，直接返回
    if (lumaRange < edgeThreshold) {
        return vec4(rgbM, 1.0);
    }
    
    // 计算梯度
    float lumaNS = lumaN + lumaS;
    float lumaWE = lumaW + lumaE;
    float lumaNWSE = lumaNW + lumaSE;
    float lumaNESW = lumaNE + lumaSW;
    
    // 检测边缘方向
    bool isHorizontal = (lumaNS > lumaWE);
    float luma1 = isHorizontal ? lumaNS : lumaWE;
    float luma2 = isHorizontal ? lumaNWSE : lumaNESW;
    
    // 计算边缘位置
    vec2 dir = isHorizontal ? vec2(0.0, texelSize.y) : vec2(texelSize.x, 0.0);
    
    // 子像素偏移
    float edgeStart = luma1 * 0.5 - lumaM;
    float edgeEnd = luma1 * 0.5 - luma2;
    
    // 计算混合权重
    float blendOffset = edgeStart / (edgeStart + edgeEnd);
    blendOffset = clamp(blendOffset, 0.0, 1.0);
    
    // 应用 FXAA
    vec2 finalUV = uv - dir * blendOffset;
    vec3 result = texture(tex, finalUV).rgb;
    
    // 子像素质量调整
    float subpixelBlend = 1.0 - ((lumaMax - lumaMin) / lumaMax);
    subpixelBlend = pow(subpixelBlend, fxaa.u_subpixelQuality);
    
    result = mix(result, rgbM, subpixelBlend);
    
    return vec4(result, 1.0);
}

void main() {
    outColor = fxaaFilter(inUV, u_inputTexture, fxaa.u_texelSize);
}
