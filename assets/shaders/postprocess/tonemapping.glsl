// Tone Mapping Shader
// HDR → LDR 转换，支持多种色调映射算法

#version 450 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform ToneMappingUBO {
    float u_exposure;
    float u_gamma;
    float u_whitePoint;
    float u_contrast;
    float u_saturation;
    int u_algorithm; // 0=None, 1=Reinhard, 2=Reinhard2, 3=ACES, 4=ACESApprox, 5=Uncharted2, 6=HejlDawson, 7=Hable, 8=Neutral
    float u_padding1;
    float u_padding2;
} toneMapping;

layout(binding = 0) uniform sampler2D u_inputTexture;

// Reinhard 色调映射
vec3 reinhard(vec3 color, float exposure) {
    color *= exposure;
    return color / (1.0 + color);
}

// Reinhard2 (带白点)
vec3 reinhard2(vec3 color, float exposure, float whitePoint) {
    color *= exposure;
    float whiteSq = whitePoint * whitePoint;
    return color * (1.0 + color / whiteSq) / (1.0 + color * color / whiteSq);
}

// ACES 电影曲线
vec3 aces(vec3 color, float exposure) {
    color *= exposure;
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return (color * (a * color + b)) / (color * (c * color + d) + e);
}

// ACES 近似
vec3 acesApprox(vec3 color, float exposure) {
    color *= exposure;
    float a = 0.6 * 2.51;
    float b = 0.6 * 0.03;
    float c = 0.6 * 2.43;
    float d = 0.6 * 0.59;
    float e = 0.6 * 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

// Uncharted 2 色调映射
vec3 uncharted2(vec3 color, float exposure) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    float exposureBias = 2.0;
    
    color *= exposure * exposureBias;
    
    vec3 curr = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
    vec3 white = ((vec3(W) * (A * vec3(W) + C * B) + D * E) / (vec3(W) * (A * vec3(W) + B) + D * F)) - E / F;
    
    return curr / white;
}

// Hejl-Dawson 色调映射
vec3 hejlDawson(vec3 color, float exposure) {
    color *= exposure;
    return max(0.0, min(1.0, (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14)));
}

// Hable 色调映射 (UC2 改进)
vec3 hable(vec3 color, float exposure) {
    float A = 0.2;
    float B = 0.29;
    float C = 0.24;
    float D = 0.272;
    float E = 0.02;
    float F = 0.3;
    float W = 5.0;
    
    color *= exposure;
    
    auto hableTonemap = [=](float x) -> float {
        return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    };
    
    vec3 curr = vec3(hableTonemap(color.r), hableTonemap(color.g), hableTonemap(color.b));
    vec3 white = vec3(hableTonemap(W));
    
    return curr / white;
}

// 中性色调映射
vec3 neutral(vec3 color, float exposure) {
    color *= exposure;
    float startCompression = 0.8 - 0.04;
    float desaturation = 0.15;
    
    return vec3(
        color.r < startCompression ? color.r : startCompression + log2(color.r / startCompression) * (1.0 - desaturation),
        color.g < startCompression ? color.g : startCompression + log2(color.g / startCompression) * (1.0 - desaturation),
        color.b < startCompression ? color.b : startCompression + log2(color.b / startCompression) * (1.0 - desaturation)
    );
}

// 饱和度调整
vec3 adjustSaturation(vec3 color, float saturation) {
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    return mix(vec3(luminance), color, saturation);
}

// 对比度调整
vec3 adjustContrast(vec3 color, float contrast) {
    return (color - 0.5) * contrast + 0.5;
}

void main() {
    vec3 color = texture(u_inputTexture, inUV).rgb;
    
    // 应用曝光
    color *= toneMapping.u_exposure;
    
    // 应用色调映射算法
    if (toneMapping.u_algorithm == 1) {
        color = reinhard(color, 1.0);
    } else if (toneMapping.u_algorithm == 2) {
        color = reinhard2(color, 1.0, toneMapping.u_whitePoint);
    } else if (toneMapping.u_algorithm == 3) {
        color = aces(color, 1.0);
    } else if (toneMapping.u_algorithm == 4) {
        color = acesApprox(color, 1.0);
    } else if (toneMapping.u_algorithm == 5) {
        color = uncharted2(color, 1.0);
    } else if (toneMapping.u_algorithm == 6) {
        color = hejlDawson(color, 1.0);
    } else if (toneMapping.u_algorithm == 7) {
        color = hable(color, 1.0);
    } else if (toneMapping.u_algorithm == 8) {
        color = neutral(color, 1.0);
    }
    // algorithm 0 = None, 直接跳过
    
    // 应用对比度
    if (toneMapping.u_contrast != 1.0) {
        color = adjustContrast(color, toneMapping.u_contrast);
    }
    
    // 应用饱和度
    if (toneMapping.u_saturation != 1.0) {
        color = adjustSaturation(color, toneMapping.u_saturation);
    }
    
    // Gamma 校正
    float invGamma = 1.0 / toneMapping.u_gamma;
    color = pow(clamp(color, 0.0, 1.0), vec3(invGamma));
    
    outColor = vec4(color, 1.0);
}
