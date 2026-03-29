// Phoenix Engine - IBL Equirectangular to Cubemap Shader
// 将等距柱状投影 HDR 转换为立方体贴图

#version 450

// Uniform buffers
layout(std140, binding = 0) uniform UniformBuffer {
    mat4 u_viewProjection[6];
} ubo;

// Input from vertex shader
layout(location = 0) in vec3 v_texcoord;

// Output color
layout(location = 0) out vec4 o_color;

// Equirectangular texture sampler
layout(binding = 0) uniform sampler2D u_equirectangularMap;

// Constants
const float PI = 3.14159265359;

vec2 sampleSphericalMap(const vec3 direction) {
    vec2 uv = vec2(
        atan(direction.z, direction.x),
        asin(direction.y)
    );
    uv *= vec2(1.0 / (2.0 * PI), 1.0 / PI);
    uv += 0.5;
    return uv;
}

void main() {
    // Normalize the texture coordinate (it's the direction vector for cubemap)
    vec3 direction = normalize(v_texcoord);
    
    // Sample the equirectangular map
    vec2 uv = sampleSphericalMap(direction);
    
    // Sample with gamma correction (HDR is linear)
    vec3 color = texture(u_equirectangularMap, uv).rgb;
    
    o_color = vec4(color, 1.0);
}
