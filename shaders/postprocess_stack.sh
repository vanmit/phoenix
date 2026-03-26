// Post-Process Stack Shader
// Phoenix Engine Phase 3

$include "common.sh"

// ==================== Uniforms ====================

$constants
{
    // 通用
    vec2 u_texelSize;
    float u_time;
    int u_passType;
    
    // Bloom
    float u_bloomThreshold;
    float u_bloomIntensity;
    vec3 u_bloomTint;
    float u_bloomKnee;
    
    // 色调映射
    float u_exposure;
    float u_gamma;
    float u_whitePoint;
    int u_toneMappingAlgo;
    
    // SSAO
    float u_ssaoRadius;
    float u_ssaoBias;
    float u_ssaoIntensity;
    float u_ssaoScale;
    
    // FXAA
    float u_fxaaSubpixel;
    float u_fxaaEdgeThreshold;
    float u_fxaaEdgeThresholdMin;
    
    // 颜色分级
    vec3 u_colorTemperature;
    float u_saturation;
    float u_contrast;
    float u_brightness;
};

// 纹理
SAMPLER2D(s_input, 0);
SAMPLER2D(s_bloom, 1);
SAMPLER2D(s_ssao, 2);
SAMPLER2D(s_normal, 3);
SAMPLER2D(s_depth, 4);
SAMPLER2D(s_noise, 5);
SAMPLERCUBE(s_environment, 6);

// ==================== 色调映射函数 ====================

vec3 toneMappingReinhard(vec3 hdr) {
    return hdr / (hdr + vec3(1.0));
}

vec3 toneMappingACES(vec3 hdr) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return (hdr * (a * hdr + b)) / (hdr * (c * hdr + d) + e);
}

vec3 toneMappingHable(vec3 hdr) {
    float A = 0.2;
    float B = 0.29;
    float C = 0.24;
    float D = 0.272;
    float E = 0.02;
    float F = 0.3;
    
    auto hable = [&](float x) {
        return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    };
    
    float W = 5.0;
    return vec3(
        hable(hdr.r) / hable(W),
        hable(hdr.g) / hable(W),
        hable(hdr.b) / hable(W)
    );
}

vec3 applyToneMapping(vec3 hdr, int algorithm, float exposure, float gamma) {
    hdr *= exposure;
    
    vec3 mapped;
    if (algorithm == 0) {
        mapped = toneMappingReinhard(hdr);
    } else if (algorithm == 1) {
        mapped = toneMappingACES(hdr);
    } else {
        mapped = toneMappingHable(hdr);
    }
    
    // Gamma 校正
    return pow(clamp(mapped, 0.0, 1.0), vec3(1.0/gamma));
}

// ==================== Bloom ====================

vec3 extractBright(vec2 uv) {
    vec3 color = texture2D(s_input, uv).rgb;
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    // 阈值提取
    float threshold = max(luminance - u_bloomThreshold, 0.0);
    threshold /= max(luminance, 0.0001);
    threshold = clamp(threshold, 0.0, 1.0);
    
    return color * threshold * threshold;
}

vec3 gaussianBlur(vec2 uv, vec2 texelSize, float radius) {
    vec3 result = vec3(0.0);
    float weightSum = 0.0;
    
    const int kernelSize = 5;
    const float kernel[kernelSize] = float[](0.05, 0.25, 0.4, 0.25, 0.05);
    
    for (int i = 0; i < kernelSize; ++i) {
        float offset = (float(i) - float(kernelSize/2)) * radius;
        result += texture2D(s_input, uv + texelSize * offset).rgb * kernel[i];
        weightSum += kernel[i];
    }
    
    return result / weightSum;
}

// ==================== SSAO ====================

float calculateSSAO(vec2 uv, vec3 fragPos, vec3 normal) {
    float depth = texture2D(s_depth, uv).r;
    float radius = u_ssaoRadius;
    float bias = u_ssaoBias;
    
    float occlusion = 0.0;
    const int kernelSize = 16;
    
    for (int i = 0; i < kernelSize; ++i) {
        // 随机采样
        vec2 offset = vec2(
            sin(float(i) * 2.4) * u_texelSize.x,
            cos(float(i) * 1.7) * u_texelSize.y
        ) * radius;
        
        float sampleDepth = texture2D(s_depth, uv + offset).r;
        
        // 深度差
        float diff = sampleDepth - depth;
        
        if (diff > bias) {
            occlusion += 1.0;
        }
    }
    
    return 1.0 - (occlusion / float(kernelSize));
}

// ==================== FXAA ====================

vec3 applyFXAA(vec2 uv) {
    vec3 rgbNW = texture2D(s_input, uv + vec2(-1.0, -1.0) * u_texelSize).rgb;
    vec3 rgbNE = texture2D(s_input, uv + vec2(1.0, -1.0) * u_texelSize).rgb;
    vec3 rgbSW = texture2D(s_input, uv + vec2(-1.0, 1.0) * u_texelSize).rgb;
    vec3 rgbSE = texture2D(s_input, uv + vec2(1.0, 1.0) * u_texelSize).rgb;
    vec3 rgbM  = texture2D(s_input, uv).rgb;
    
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM, luma);
    
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    float lumaRange = lumaMax - lumaMin;
    
    if (lumaRange < max(lumaMax * u_fxaaEdgeThreshold, u_fxaaEdgeThresholdMin)) {
        return rgbM;
    }
    
    // 边缘检测和抗锯齿
    vec2 dir = vec2(
        -((lumaNW + lumaNE) - (lumaSW + lumaSE)),
        ((lumaNW + lumaSW) - (lumaNE + lumaSE))
    );
    
    dir = normalize(dir) * u_texelSize * 0.5;
    
    vec3 rgbA = texture2D(s_input, uv + dir).rgb;
    vec3 rgbB = texture2D(s_input, uv - dir).rgb;
    
    return (rgbA + rgbB) * 0.5;
}

// ==================== 颜色分级 ====================

vec3 applyColorGrading(vec3 color) {
    // 色温
    color.rgb *= u_colorTemperature;
    
    // 饱和度
    float gray = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(gray), color, u_saturation);
    
    // 对比度
    color = (color - 0.5) * u_contrast + 0.5;
    
    // 亮度
    color += u_brightness;
    
    return clamp(color, 0.0, 1.0);
}

// ==================== 顶点着色器 ====================

void main() {
    v_texcoord = a_texcoord0;
    gl_Position = vec4(a_position, 1.0);
}

// ==================== 片元着色器 ====================

void main() {
    vec2 uv = v_texcoord;
    vec3 color = texture2D(s_input, uv).rgb;
    
    // Pass 0: 亮度提取 (Bloom)
    if (u_passType == 0) {
        color = extractBright(uv);
    }
    // Pass 1: 高斯模糊 (Bloom)
    else if (u_passType == 1) {
        color = gaussianBlur(uv, u_texelSize, 2.0);
    }
    // Pass 2: SSAO
    else if (u_passType == 2) {
        vec3 normal = texture2D(s_normal, uv).rgb * 2.0 - 1.0;
        vec3 fragPos = vec3(uv * 2.0 - 1.0, texture2D(s_depth, uv).r);
        color = vec3(calculateSSAO(uv, fragPos, normal));
    }
    // Pass 3: Bloom 合成
    else if (u_passType == 3) {
        vec3 bloom = texture2D(s_bloom, uv).rgb * u_bloomIntensity * u_bloomTint;
        color += bloom;
    }
    // Pass 4: FXAA
    else if (u_passType == 4) {
        color = applyFXAA(uv);
    }
    // Pass 5: 色调映射
    else if (u_passType == 5) {
        color = applyToneMapping(color, u_toneMappingAlgo, u_exposure, u_gamma);
    }
    // Pass 6: 颜色分级
    else if (u_passType == 6) {
        color = applyColorGrading(color);
    }
    // 默认：直接输出
    else {
        // 应用 SSAO
        float ao = texture2D(s_ssao, uv).r;
        color *= ao;
    }
    
    gl_FragColor = vec4(color, 1.0);
}
