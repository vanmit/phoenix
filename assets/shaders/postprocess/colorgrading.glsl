// Color Grading Shader
// 色彩分级：色相/饱和度/对比度/色彩平衡/LUT 等

#version 450 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) uniform ColorGradingUBO {
    // 基本调整
    float u_temperature;    // 色温
    float u_tint;           // 色调
    float u_saturation;     // 饱和度
    float u_contrast;       // 对比度
    float u_brightness;     // 亮度
    float u_gamma;          // Gamma
    
    // Lift/Gamma/Gain
    vec4 u_lift;
    vec4 u_gammaColor;
    vec4 u_gain;
    
    // 阴影/中间调/高光
    vec3 u_shadows;
    float u_shadowsPadding;
    vec3 u_midtones;
    float u_midtonesPadding;
    vec3 u_highlights;
    float u_highlightsPadding;
    
    // LUT
    int u_useLUT;
    float u_lutIntensity;
    float u_padding1;
    float u_padding2;
    
    // 通道混合
    mat3 u_channelMix;
} colorGrading;

layout(binding = 0) uniform sampler2D u_inputTexture;
layout(binding = 1) uniform sampler3D u_lutTexture;

// 色温调整 (蓝←→黄)
vec3 adjustTemperature(vec3 color, float temperature) {
    if (temperature > 0.0) {
        // 暖色调 (增加红色/黄色)
        color.r += temperature * 0.1;
        color.g += temperature * 0.05;
        color.b -= temperature * 0.15;
    } else {
        // 冷色调 (增加蓝色)
        color.r += temperature * 0.15;
        color.g -= temperature * 0.05;
        color.b -= temperature * 0.1;
    }
    return clamp(color, 0.0, 1.0);
}

// 色调调整 (绿←→品红)
vec3 adjustTint(vec3 color, float tint) {
    color.g -= tint * 0.1;
    color.r += tint * 0.1;
    return clamp(color, 0.0, 1.0);
}

// 饱和度调整
vec3 adjustSaturation(vec3 color, float saturation) {
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    return mix(vec3(luminance), color, 1.0 + saturation);
}

// 对比度调整
vec3 adjustContrast(vec3 color, float contrast) {
    return (color - 0.5) * (1.0 + contrast) + 0.5;
}

// 亮度调整
vec3 adjustBrightness(vec3 color, float brightness) {
    return color + brightness;
}

// Lift/Gamma/Gain 调整
vec3 applyLiftGammaGain(vec3 color, vec3 lift, vec3 gammaColor, vec3 gain) {
    // 简化的 LGG 模型
    return color * gain + pow(color, vec3(1.0 / (1.0 + gammaColor))) + lift;
}

// 三向色彩平衡 (阴影/中间调/高光)
vec3 threeWayColorBalance(vec3 color, vec3 shadows, vec3 midtones, vec3 highlights) {
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    // 根据亮度混合三个调整
    float shadowWeight = smoothstep(0.5, 0.0, luminance);
    float highlightWeight = smoothstep(0.5, 1.0, luminance);
    float midtoneWeight = 1.0 - shadowWeight - highlightWeight;
    
    vec3 result = color;
    result += shadows * shadowWeight;
    result += midtones * midtoneWeight;
    result += highlights * highlightWeight;
    
    return result;
}

// 通道混合
vec3 applyChannelMix(vec3 color, mat3 mixMatrix) {
    return mixMatrix * color;
}

void main() {
    vec3 color = texture(u_inputTexture, inUV).rgb;
    
    // 1. 色温调整
    if (colorGrading.u_temperature != 0.0) {
        color = adjustTemperature(color, colorGrading.u_temperature);
    }
    
    // 2. 色调调整
    if (colorGrading.u_tint != 0.0) {
        color = adjustTint(color, colorGrading.u_tint);
    }
    
    // 3. 饱和度调整
    if (colorGrading.u_saturation != 0.0) {
        color = adjustSaturation(color, colorGrading.u_saturation);
    }
    
    // 4. 对比度调整
    if (colorGrading.u_contrast != 0.0) {
        color = adjustContrast(color, colorGrading.u_contrast);
    }
    
    // 5. 亮度调整
    if (colorGrading.u_brightness != 0.0) {
        color = adjustBrightness(color, colorGrading.u_brightness);
    }
    
    // 6. Lift/Gamma/Gain
    if (any(notEqual(colorGrading.u_lift.rgb, vec3(0.0))) ||
        any(notEqual(colorGrading.u_gammaColor.rgb, vec3(0.0))) ||
        any(notEqual(colorGrading.u_gain.rgb, vec3(0.0)))) {
        color = applyLiftGammaGain(color, colorGrading.u_lift.rgb, 
                                   colorGrading.u_gammaColor.rgb, 
                                   colorGrading.u_gain.rgb);
    }
    
    // 7. 三向色彩平衡
    if (any(notEqual(colorGrading.u_shadows, vec3(0.0))) ||
        any(notEqual(colorGrading.u_midtones, vec3(0.0))) ||
        any(notEqual(colorGrading.u_highlights, vec3(0.0)))) {
        color = threeWayColorBalance(color, colorGrading.u_shadows,
                                     colorGrading.u_midtones,
                                     colorGrading.u_highlights);
    }
    
    // 8. 通道混合
    if (colorGrading.u_channelMix != mat3(1.0)) {
        color = applyChannelMix(color, colorGrading.u_channelMix);
    }
    
    // 9. LUT 应用
    if (colorGrading.u_useLUT > 0) {
        vec3 lutColor = texture(u_lutTexture, color).rgb;
        color = mix(color, lutColor, colorGrading.u_lutIntensity);
    }
    
    // 10. Gamma 校正
    if (colorGrading.u_gamma != 0.0) {
        float invGamma = 1.0 / (1.0 + colorGrading.u_gamma);
        color = pow(clamp(color, 0.0, 1.0), vec3(invGamma));
    }
    
    outColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
