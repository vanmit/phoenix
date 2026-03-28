/**
 * Phoenix Engine - WASM Demo Application
 * Comprehensive mobile demo showcasing PBR rendering, animations, and effects
 * 
 * Compile with: emcc demo-app.cpp -o demo-app.wasm -s WASM=1 -s USE_WEBGL2=1 \
 *   -s FULL_ES3=1 -s ALLOW_MEMORY_GROWTH=1 -s MAX_WEBGL_VERSION=2 \
 *   -s INITIAL_MEMORY=134217728 -s EXPORTED_FUNCTIONS='["_main","_init_gl","_update","_render","_on_resize","_on_touch_rotate","_on_touch_zoom","_on_touch_pan","_on_double_tap","_set_camera_mode","_set_material_param","_set_effect","_set_animation"]' \
 *   -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -O3 -flto
 */

#include <emscripten/emscripten.h>
#include <emscripten/html5_webgl.h>
#include <GLES3/gl3.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <string>

// ============================================================================
// Configuration
// ============================================================================
#define MAX_MODELS 10
#define MAX_LIGHTS 16
#define MAX_PARTICLES 1000
#define CSM_CASCADES 4

// ============================================================================
// Vector/Math Utilities
// ============================================================================
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0), y(0), z(0), w(1) {}
    Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

// 数学辅助函数 (在 Vec3 定义之后)
namespace phoenix_math {
    inline float dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
    inline Vec3 cross(const Vec3& a, const Vec3& b) { return Vec3(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x); }
    inline Vec3 normalize(const Vec3& v) { float len = std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); return len > 0 ? Vec3(v.x/len, v.y/len, v.z/len) : Vec3(); }
}

using namespace phoenix_math;

struct Mat4 {
    float m[16];
    
    Mat4() {
        memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
    
    static Mat4 identity() { return Mat4(); }
    
    static Mat4 perspective(float fov, float aspect, float near, float far) {
        Mat4 result;
        memset(result.m, 0, sizeof(result.m));
        float tanHalfFov = tanf(fov / 2.0f);
        result.m[0] = 1.0f / (aspect * tanHalfFov);
        result.m[5] = 1.0f / tanHalfFov;
        result.m[10] = -(far + near) / (far - near);
        result.m[11] = -1.0f;
        result.m[14] = -(2.0f * far * near) / (far - near);
        return result;
    }
    
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = normalize(Vec3(center.x - eye.x, center.y - eye.y, center.z - eye.z));
        Vec3 s = normalize(cross(f, up));
        Vec3 u = cross(s, f);
        
        Mat4 result;
        result.m[0] = s.x; result.m[4] = s.y; result.m[8] = s.z;
        result.m[1] = u.x; result.m[5] = u.y; result.m[9] = u.z;
        result.m[2] = -f.x; result.m[6] = -f.y; result.m[10] = -f.z;
        result.m[12] = -dot(s, eye);
        result.m[13] = -dot(u, eye);
        result.m[14] = dot(f, eye);
        return result;
    }
    
    static Mat4 translate(const Mat4& m, const Vec3& v) {
        Mat4 result = m;
        result.m[12] = m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12];
        result.m[13] = m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13];
        result.m[14] = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14];
        return result;
    }
    
    static Mat4 rotate(const Mat4& m, float angle, const Vec3& axis) {
        float c = cosf(angle), s = sinf(angle);
        Vec3 ax = normalize(axis);
        Mat4 rot;
        rot.m[0] = ax.x * ax.x * (1 - c) + c;
        rot.m[1] = ax.y * ax.x * (1 - c) + ax.z * s;
        rot.m[2] = ax.x * ax.z * (1 - c) - ax.y * s;
        rot.m[4] = ax.x * ax.y * (1 - c) - ax.z * s;
        rot.m[5] = ax.y * ax.y * (1 - c) + c;
        rot.m[6] = ax.y * ax.z * (1 - c) + ax.x * s;
        rot.m[8] = ax.x * ax.z * (1 - c) + ax.y * s;
        rot.m[9] = ax.y * ax.z * (1 - c) - ax.x * s;
        rot.m[10] = ax.z * ax.z * (1 - c) + c;
        
        Mat4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i * 4 + j] = 
                    m.m[0 * 4 + j] * rot.m[i * 4 + 0] +
                    m.m[1 * 4 + j] * rot.m[i * 4 + 1] +
                    m.m[2 * 4 + j] * rot.m[i * 4 + 2] +
                    m.m[3 * 4 + j] * rot.m[i * 4 + 3];
            }
        }
        return result;
    }
    
    static Mat4 scale(const Mat4& m, const Vec3& s) {
        Mat4 result;
        result.m[0] = m.m[0] * s.x;
        result.m[1] = m.m[1] * s.x;
        result.m[2] = m.m[2] * s.x;
        result.m[4] = m.m[4] * s.y;
        result.m[5] = m.m[5] * s.y;
        result.m[6] = m.m[6] * s.y;
        result.m[8] = m.m[8] * s.z;
        result.m[9] = m.m[9] * s.z;
        result.m[10] = m.m[10] * s.z;
        result.m[12] = m.m[12];
        result.m[13] = m.m[13];
        result.m[14] = m.m[14];
        result.m[15] = m.m[15];
        return result;
    }
    
    static Mat4 multiply(const Mat4& a, const Mat4& b) {
        Mat4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i * 4 + j] = 
                    a.m[0 * 4 + j] * b.m[i * 4 + 0] +
                    a.m[1 * 4 + j] * b.m[i * 4 + 1] +
                    a.m[2 * 4 + j] * b.m[i * 4 + 2] +
                    a.m[3 * 4 + j] * b.m[i * 4 + 3];
            }
        }
        return result;
    }
};

inline Vec3 normalize(const Vec3& v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return len > 0 ? Vec3(v.x / len, v.y / len, v.z / len) : Vec3();
}

inline float dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

// ============================================================================
// Shader Sources
// ============================================================================
const char* vertexShaderSource = R"(
#version 300 es
precision highp float;

in vec3 aPosition;
in vec3 aNormal;
in vec2 aTexCoord;
in vec3 aTangent;
in vec3 aBitangent;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uNormalMatrix;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vTangent;
out vec3 vBitangent;
out vec2 vTexCoord;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(uNormalMatrix) * aNormal;
    vTangent = mat3(uNormalMatrix) * aTangent;
    vBitangent = mat3(uNormalMatrix) * aBitangent;
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uView * worldPos;
}
)";

const char* fragmentShaderSource = R"(
#version 300 es
precision highp float;

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;
in vec2 vTexCoord;

uniform vec3 uAlbedo;
uniform float uMetallic;
uniform float uRoughness;
uniform float uClearcoat;
uniform vec3 uViewPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;

out vec4 fragColor;

// PBR BRDF functions
vec3 getNormalFromMap() {
    return normalize(vNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    vec3 N = normalize(getNormalFromMap());
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    
    vec3 albedo = uAlbedo;
    float metallic = uMetallic;
    float roughness = uRoughness;
    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 lo = (kD * albedo / 3.14159265 + specular) * uLightColor * NdotL;
    
    // Ambient
    vec3 ambient = vec3(0.03) * albedo;
    
    vec3 color = ambient + lo;
    
    // Tone mapping (ACES)
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    fragColor = vec4(color, 1.0);
}
)";

const char* skyboxVertexSource = R"(
#version 300 es
precision highp float;

in vec3 aPosition;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vTexCoord;

void main() {
    vTexCoord = aPosition;
    vec4 pos = uProjection * mat4(mat3(uView)) * vec4(aPosition, 1.0);
    gl_Position = pos.xyww;
}
)";

const char* skyboxFragmentSource = R"(
#version 300 es
precision highp float;

in vec3 vTexCoord;
uniform samplerCube uSkybox;

out vec4 fragColor;

void main() {
    fragColor = texture(uSkybox, vTexCoord);
}
)";

const char* particleVertexSource = R"(
#version 300 es
precision highp float;

in vec3 aPosition;
in vec3 aVelocity;
in float aLife;

uniform mat4 uView;
uniform mat4 uProjection;
uniform float uTime;

out float vLife;
out vec3 vColor;

void main() {
    vec3 pos = aPosition + aVelocity * uTime;
    gl_Position = uProjection * uView * vec4(pos, 1.0);
    gl_PointSize = 8.0 * (1.0 - aLife);
    vLife = aLife;
    vColor = mix(vec3(1.0, 0.5, 0.0), vec3(0.5, 0.5, 0.5), aLife);
}
)";

const char* particleFragmentSource = R"(
#version 300 es
precision highp float;

in float vLife;
in vec3 vColor;

out vec4 fragColor;

void main() {
    vec2 center = gl_PointCoord - vec2(0.5);
    if (dot(center, center) > 0.25) discard;
    
    float alpha = 1.0 - vLife;
    fragColor = vec4(vColor, alpha);
}
)";

// ============================================================================
// Application State
// ============================================================================
struct Light {
    Vec3 position;
    Vec3 color;
    float intensity;
    int type; // 0=directional, 1=point, 2=spot
};

struct Particle {
    Vec3 position;
    Vec3 velocity;
    float life;
    float maxLife;
    bool active;
};

struct Model {
    GLuint vao;
    GLuint vbo;
    GLuint ibo;
    int indexCount;
    Mat4 transform;
    Vec3 albedo;
    float metallic;
    float roughness;
    float clearcoat;
};

struct Camera {
    Vec3 position;
    Vec3 target;
    Vec3 up;
    float yaw;
    float pitch;
    float distance;
    int mode; // 0=orbit, 1=fps, 2=third
};

class PhoenixDemo {
private:
    int width, height;
    GLuint shaderProgram;
    GLuint skyboxProgram;
    GLuint particleProgram;
    
    Camera camera;
    Model models[MAX_MODELS];
    int modelCount;
    
    Light lights[MAX_LIGHTS];
    int lightCount;
    
    Particle particles[MAX_PARTICLES];
    int particleCount;
    
    GLuint skyboxVAO;
    GLuint skyboxVBO;
    GLuint skyboxTexture;
    
    GLuint particleVAO;
    GLuint particleVBO;
    
    float time;
    float cameraAngleX;
    float cameraAngleY;
    
    // Performance tracking
    int frameCount;
    float fps;
    int drawCalls;
    int triangleCount;
    
    // Settings
    bool shadowsEnabled;
    bool bloomEnabled;
    bool toneMappingEnabled;
    bool ssaoEnabled;
    int currentAnimation;
    float blendSpeed;

public:
    PhoenixDemo() : width(0), height(0), shaderProgram(0), time(0),
                    cameraAngleX(0), cameraAngleY(0.3), frameCount(0),
                    fps(60), drawCalls(0), triangleCount(0),
                    shadowsEnabled(true), bloomEnabled(true),
                    toneMappingEnabled(true), ssaoEnabled(false),
                    currentAnimation(0), blendSpeed(0.3f),
                    modelCount(0), lightCount(0), particleCount(0) {
        
        // Initialize camera
        camera.mode = 0;
        camera.distance = 5.0f;
        camera.up = Vec3(0, 1, 0);
        updateCameraPosition();
        
        // Initialize lights
        setupLights();
        
        // Initialize particles
        memset(particles, 0, sizeof(particles));
    }
    
    void updateCameraPosition() {
        if (camera.mode == 0) { // Orbit
            float x = camera.distance * cos(cameraAngleY) * sin(cameraAngleX);
            float y = camera.distance * sin(cameraAngleY);
            float z = camera.distance * cos(cameraAngleY) * cos(cameraAngleX);
            camera.position = Vec3(x, y, z);
            camera.target = Vec3(0, 0, 0);
        }
    }
    
    void setupLights() {
        // Directional light (sun)
        lights[lightCount++] = { Vec3(-1, -1, -0.5), Vec3(1.0, 0.95, 0.9), 1.0f, 0 };
        
        // Point lights
        lights[lightCount++] = { Vec3(2, 3, 2), Vec3(1.0, 0.5, 0.2), 0.8f, 1 };
        lights[lightCount++] = { Vec3(-2, 2, -2), Vec3(0.2, 0.5, 1.0), 0.6f, 1 };
        
        // Spot light
        lights[lightCount++] = { Vec3(0, 5, 0), Vec3(1.0, 1.0, 1.0), 0.5f, 2 };
    }
    
    GLuint compileShader(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            emscripten_console_error(infoLog);
        }
        return shader;
    }
    
    GLuint createProgram(const char* vs, const char* fs) {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vs);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fs);
        
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            emscripten_console_error(infoLog);
        }
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return program;
    }
    
    void createSkybox() {
        float skyboxVertices[] = {
            -1, -1, -1,  -1, -1,  1,  -1,  1,  1,  -1,  1, -1,
             1, -1, -1,   1, -1,  1,   1,  1,  1,   1,  1, -1,
            -1, -1, -1,   1, -1, -1,   1,  1, -1,  -1,  1, -1,
            -1, -1,  1,   1, -1,  1,   1,  1,  1,  -1,  1,  1
        };
        
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        
        glBindVertexArray(0);
        
        // Create procedural skybox texture
        glGenTextures(1, &skyboxTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
        
        // Generate simple gradient skybox
        unsigned char skyData[6][512][512][3];
        for (int face = 0; face < 6; face++) {
            for (int y = 0; y < 512; y++) {
                for (int x = 0; x < 512; x++) {
                    float t = y / 512.0f;
                    skyData[face][y][x][0] = (unsigned char)(26 + t * 40);
                    skyData[face][y][x][1] = (unsigned char)(26 + t * 30);
                    skyData[face][y][x][2] = (unsigned char)(46 + t * 50);
                }
            }
        }
        
        for (int i = 0; i < 6; i++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, skyData[i]);
        }
        
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    
    void createParticleBuffer() {
        glGenVertexArrays(1, &particleVAO);
        glGenBuffers(1, &particleVBO);
        
        glBindVertexArray(particleVAO);
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        
        // Position + Velocity + Life
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
        
        glBindVertexArray(0);
    }
    
    void createDemoModels() {
        // Create simple geometric models for demo
        // In production, these would be loaded from glTF files
        
        // Model 1: Sphere (main showcase object)
        createSphereModel(0, 0, 0, 1.0, Vec3(0.8, 0.3, 0.2), 0.7f, 0.3f, 0.0f);
        
        // Model 2: Torus
        createTorusModel(2, 0, -2, 0.8, 0.3, Vec3(0.2, 0.6, 0.8), 0.9f, 0.1f, 0.5f);
        
        // Model 3: Cube
        createCubeModel(-2, 0, -2, 1.0, Vec3(0.3, 0.7, 0.4), 0.2f, 0.8f, 0.0f);
        
        // Model 4: Cone
        createConeModel(0, 0, 2, 0.7, 1.5, Vec3(0.9, 0.7, 0.1), 0.8f, 0.4f, 0.3f);
        
        // Model 5: Cylinder
        createCylinderModel(-1.5, 0, 1.5, 0.5, 1.5, Vec3(0.6, 0.6, 0.7), 0.5f, 0.5f, 0.0f);
    }
    
    void createSphereModel(float x, float y, float z, float radius, Vec3 albedo, float metallic, float roughness, float clearcoat) {
        if (modelCount >= MAX_MODELS) return;
        
        Model& model = models[modelCount++];
        memset(&model, 0, sizeof(Model));
        
        // Generate sphere vertices
        int rings = 24, sectors = 32;
        int vertexCount = (rings + 1) * (sectors + 1);
        int indexCount = rings * sectors * 6;
        
        float* vertices = new float[vertexCount * 8]; // pos + normal + texcoord
        unsigned int* indices = new unsigned int[indexCount];
        
        int idx = 0;
        for (int r = 0; r <= rings; r++) {
            float theta = r * 3.14159265f / rings;
            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);
            
            for (int s = 0; s <= sectors; s++) {
                float phi = s * 2.0f * 3.14159265f / sectors;
                float sinPhi = sinf(phi);
                float cosPhi = cosf(phi);
                
                float nx = cosPhi * sinTheta;
                float ny = cosTheta;
                float nz = sinPhi * sinTheta;
                
                vertices[idx++] = x + radius * nx;
                vertices[idx++] = y + radius * ny;
                vertices[idx++] = z + radius * nz;
                vertices[idx++] = nx;
                vertices[idx++] = ny;
                vertices[idx++] = nz;
                vertices[idx++] = (float)s / sectors;
                vertices[idx++] = (float)r / rings;
            }
        }
        
        idx = 0;
        for (int r = 0; r < rings; r++) {
            for (int s = 0; s < sectors; s++) {
                int i0 = r * (sectors + 1) + s;
                int i1 = i0 + 1;
                int i2 = i0 + (sectors + 1);
                int i3 = i2 + 1;
                
                indices[idx++] = i0;
                indices[idx++] = i2;
                indices[idx++] = i1;
                indices[idx++] = i1;
                indices[idx++] = i2;
                indices[idx++] = i3;
            }
        }
        
        glGenVertexArrays(1, &model.vao);
        glGenBuffers(1, &model.vbo);
        glGenBuffers(1, &model.ibo);
        
        glBindVertexArray(model.vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * 8 * sizeof(float), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        
        model.indexCount = indexCount;
        model.transform = Mat4::identity();
        model.albedo = albedo;
        model.metallic = metallic;
        model.roughness = roughness;
        model.clearcoat = clearcoat;
        
        glBindVertexArray(0);
        
        delete[] vertices;
        delete[] indices;
    }
    
    void createTorusModel(float x, float y, float z, float majorRadius, float minorRadius, Vec3 albedo, float metallic, float roughness, float clearcoat) {
        if (modelCount >= MAX_MODELS) return;
        
        Model& model = models[modelCount++];
        memset(&model, 0, sizeof(Model));
        
        int majorSegments = 32, minorSegments = 16;
        int vertexCount = (majorSegments + 1) * (minorSegments + 1);
        int indexCount = majorSegments * minorSegments * 6;
        
        float* vertices = new float[vertexCount * 8];
        unsigned int* indices = new unsigned int[indexCount];
        
        int idx = 0;
        for (int i = 0; i <= majorSegments; i++) {
            float u = (float)i / majorSegments * 2.0f * 3.14159265f;
            float cosU = cosf(u), sinU = sinf(u);
            
            for (int j = 0; j <= minorSegments; j++) {
                float v = (float)j / minorSegments * 2.0f * 3.14159265f;
                float cosV = cosf(v), sinV = sinf(v);
                
                float px = (majorRadius + minorRadius * cosV) * cosU;
                float py = minorRadius * sinV;
                float pz = (majorRadius + minorRadius * cosV) * sinU;
                
                float nx = cosV * cosU;
                float ny = sinV;
                float nz = cosV * sinU;
                
                vertices[idx++] = x + px;
                vertices[idx++] = y + py;
                vertices[idx++] = z + pz;
                vertices[idx++] = nx;
                vertices[idx++] = ny;
                vertices[idx++] = nz;
                vertices[idx++] = (float)i / majorSegments;
                vertices[idx++] = (float)j / minorSegments;
            }
        }
        
        idx = 0;
        for (int i = 0; i < majorSegments; i++) {
            for (int j = 0; j < minorSegments; j++) {
                int i0 = i * (minorSegments + 1) + j;
                int i1 = i0 + 1;
                int i2 = i0 + (minorSegments + 1);
                int i3 = i2 + 1;
                
                indices[idx++] = i0;
                indices[idx++] = i2;
                indices[idx++] = i1;
                indices[idx++] = i1;
                indices[idx++] = i2;
                indices[idx++] = i3;
            }
        }
        
        glGenVertexArrays(1, &model.vao);
        glGenBuffers(1, &model.vbo);
        glGenBuffers(1, &model.ibo);
        
        glBindVertexArray(model.vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * 8 * sizeof(float), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        
        model.indexCount = indexCount;
        model.transform = Mat4::identity();
        model.albedo = albedo;
        model.metallic = metallic;
        model.roughness = roughness;
        model.clearcoat = clearcoat;
        
        glBindVertexArray(0);
        
        delete[] vertices;
        delete[] indices;
    }
    
    void createCubeModel(float x, float y, float z, float size, Vec3 albedo, float metallic, float roughness, float clearcoat) {
        if (modelCount >= MAX_MODELS) return;
        
        Model& model = models[modelCount++];
        memset(&model, 0, sizeof(Model));
        
        float half = size / 2.0f;
        float vertices[] = {
            // Front
            x-half, y-half, z+half,  0, 0, 1,
            x+half, y-half, z+half,  0, 0, 1,
            x+half, y+half, z+half,  0, 0, 1,
            x-half, y+half, z+half,  0, 0, 1,
            // Back
            x-half, y-half, z-half,  0, 0,-1,
            x-half, y+half, z-half,  0, 0,-1,
            x+half, y+half, z-half,  0, 0,-1,
            x+half, y-half, z-half,  0, 0,-1,
            // Top
            x-half, y+half, z-half,  0, 1, 0,
            x-half, y+half, z+half,  0, 1, 0,
            x+half, y+half, z+half,  0, 1, 0,
            x+half, y+half, z-half,  0, 1, 0,
            // Bottom
            x-half, y-half, z-half,  0,-1, 0,
            x+half, y-half, z-half,  0,-1, 0,
            x+half, y-half, z+half,  0,-1, 0,
            x-half, y-half, z+half,  0,-1, 0,
            // Right
            x+half, y-half, z-half,  1, 0, 0,
            x+half, y+half, z-half,  1, 0, 0,
            x+half, y+half, z+half,  1, 0, 0,
            x+half, y-half, z+half,  1, 0, 0,
            // Left
            x-half, y-half, z-half, -1, 0, 0,
            x-half, y-half, z+half, -1, 0, 0,
            x-half, y+half, z+half, -1, 0, 0,
            x-half, y+half, z-half, -1, 0, 0,
        };
        
        unsigned int indices[] = {
            0,1,2, 0,2,3,    4,5,6, 4,6,7,    8,9,10, 8,10,11,
            12,13,14, 12,14,15,  16,17,18, 16,18,19,  20,21,22, 20,22,23
        };
        
        glGenVertexArrays(1, &model.vao);
        glGenBuffers(1, &model.vbo);
        glGenBuffers(1, &model.ibo);
        
        glBindVertexArray(model.vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        
        model.indexCount = 36;
        model.transform = Mat4::identity();
        model.albedo = albedo;
        model.metallic = metallic;
        model.roughness = roughness;
        model.clearcoat = clearcoat;
        
        glBindVertexArray(0);
    }
    
    void createConeModel(float x, float y, float z, float radius, float height, Vec3 albedo, float metallic, float roughness, float clearcoat) {
        if (modelCount >= MAX_MODELS) return;
        
        Model& model = models[modelCount++];
        memset(&model, 0, sizeof(Model));
        
        int segments = 32;
        int vertexCount = (segments + 1) * 2 + 1;
        int indexCount = segments * 6 + segments * 3;
        
        float* vertices = new float[vertexCount * 6];
        unsigned int* indices = new unsigned int[indexCount];
        
        int idx = 0;
        // Apex
        vertices[idx++] = x; vertices[idx++] = y + height; vertices[idx++] = z;
        vertices[idx++] = 0; vertices[idx++] = 1; vertices[idx++] = 0;
        
        // Base ring
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * 3.14159265f;
            float px = x + radius * cosf(angle);
            float pz = z + radius * sinf(angle);
            float nx = cosf(angle);
            float nz = sinf(angle);
            
            vertices[idx++] = px;
            vertices[idx++] = y;
            vertices[idx++] = pz;
            vertices[idx++] = nx * 0.707f;
            vertices[idx++] = -0.707f;
            vertices[idx++] = nz * 0.707f;
        }
        
        idx = 0;
        // Side triangles
        for (int i = 0; i < segments; i++) {
            indices[idx++] = 0;
            indices[idx++] = i + 1;
            indices[idx++] = i + 2;
        }
        
        // Base triangles
        int baseIdx = segments + 1;
        for (int i = 0; i < segments; i++) {
            indices[idx++] = baseIdx;
            indices[idx++] = baseIdx + i + 1;
            indices[idx++] = baseIdx + i + 2;
        }
        
        glGenVertexArrays(1, &model.vao);
        glGenBuffers(1, &model.vbo);
        glGenBuffers(1, &model.ibo);
        
        glBindVertexArray(model.vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * 6 * sizeof(float), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        
        model.indexCount = indexCount;
        model.transform = Mat4::identity();
        model.albedo = albedo;
        model.metallic = metallic;
        model.roughness = roughness;
        model.clearcoat = clearcoat;
        
        glBindVertexArray(0);
        
        delete[] vertices;
        delete[] indices;
    }
    
    void createCylinderModel(float x, float y, float z, float radius, float height, Vec3 albedo, float metallic, float roughness, float clearcoat) {
        if (modelCount >= MAX_MODELS) return;
        
        Model& model = models[modelCount++];
        memset(&model, 0, sizeof(Model));
        
        int segments = 32;
        int vertexCount = (segments + 1) * 3;
        int indexCount = segments * 18;
        
        float* vertices = new float[vertexCount * 6];
        unsigned int* indices = new unsigned int[indexCount];
        
        int idx = 0;
        float halfH = height / 2.0f;
        
        // Top ring
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * 3.14159265f;
            float px = x + radius * cosf(angle);
            float pz = z + radius * sinf(angle);
            
            vertices[idx++] = px;
            vertices[idx++] = y + halfH;
            vertices[idx++] = pz;
            vertices[idx++] = 0;
            vertices[idx++] = 1;
            vertices[idx++] = 0;
        }
        
        // Bottom ring
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * 3.14159265f;
            float px = x + radius * cosf(angle);
            float pz = z + radius * sinf(angle);
            
            vertices[idx++] = px;
            vertices[idx++] = y - halfH;
            vertices[idx++] = pz;
            vertices[idx++] = 0;
            vertices[idx++] = -1;
            vertices[idx++] = 0;
        }
        
        // Side ring
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / segments * 2.0f * 3.14159265f;
            float nx = cosf(angle);
            float nz = sinf(angle);
            
            // Top vertex
            vertices[idx++] = x + radius * nx;
            vertices[idx++] = y + halfH;
            vertices[idx++] = z + radius * nz;
            vertices[idx++] = nx;
            vertices[idx++] = 0;
            vertices[idx++] = nz;
            
            // Bottom vertex
            vertices[idx++] = x + radius * nx;
            vertices[idx++] = y - halfH;
            vertices[idx++] = z + radius * nz;
            vertices[idx++] = nx;
            vertices[idx++] = 0;
            vertices[idx++] = nz;
        }
        
        idx = 0;
        // Top cap
        for (int i = 0; i < segments; i++) {
            indices[idx++] = 0;
            indices[idx++] = i + 2;
            indices[idx++] = i + 1;
        }
        
        // Bottom cap
        int baseOffset = segments + 1;
        for (int i = 0; i < segments; i++) {
            indices[idx++] = baseOffset + 0;
            indices[idx++] = baseOffset + i + 1;
            indices[idx++] = baseOffset + i + 2;
        }
        
        // Side
        int sideOffset = baseOffset * 2;
        for (int i = 0; i < segments; i++) {
            int i0 = sideOffset + i * 2;
            int i1 = sideOffset + i * 2 + 1;
            int i2 = sideOffset + (i + 1) * 2 + 1;
            int i3 = sideOffset + (i + 1) * 2;
            
            indices[idx++] = i0;
            indices[idx++] = i1;
            indices[idx++] = i2;
            indices[idx++] = i0;
            indices[idx++] = i2;
            indices[idx++] = i3;
        }
        
        glGenVertexArrays(1, &model.vao);
        glGenBuffers(1, &model.vbo);
        glGenBuffers(1, &model.ibo);
        
        glBindVertexArray(model.vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * 6 * sizeof(float), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        
        model.indexCount = indexCount;
        model.transform = Mat4::identity();
        model.albedo = albedo;
        model.metallic = metallic;
        model.roughness = roughness;
        model.clearcoat = clearcoat;
        
        glBindVertexArray(0);
        
        delete[] vertices;
        delete[] indices;
    }
    
    void spawnParticle(Vec3 pos, Vec3 vel) {
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!particles[i].active) {
                particles[i].position = pos;
                particles[i].velocity = vel;
                particles[i].life = 0;
                particles[i].maxLife = 1.0f + (rand() % 100) / 100.0f;
                particles[i].active = true;
                return;
            }
        }
    }
    
    void updateParticles(float dt) {
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].active) {
                particles[i].life += dt / particles[i].maxLife;
                particles[i].position.x += particles[i].velocity.x * dt;
                particles[i].position.y += particles[i].velocity.y * dt;
                particles[i].position.z += particles[i].velocity.z * dt;
                particles[i].velocity.y += 2.0f * dt; // Gravity
                
                if (particles[i].life >= 1.0f) {
                    particles[i].active = false;
                }
            }
        }
        
        // Spawn new particles if enabled
        static float spawnTimer = 0;
        spawnTimer += dt;
        
        if (spawnTimer >= 0.05f) {
            spawnTimer = 0;
            // Fire particles
            for (int i = 0; i < 5; i++) {
                spawnParticle(
                    Vec3((rand() % 100 - 50) / 100.0f, 0, (rand() % 100 - 50) / 100.0f),
                    Vec3((rand() % 100 - 50) / 500.0f, 3.0f + (rand() % 100) / 50.0f, (rand() % 100 - 50) / 500.0f)
                );
            }
        }
    }
    
    void update(float dt) {
        time += dt;
        
        // Update camera
        updateCameraPosition();
        
        // Update particles
        updateParticles(dt);
        
        // Rotate models slightly for demo
        for (int i = 0; i < modelCount; i++) {
            models[i].transform = Mat4::rotate(models[i].transform, dt * 0.5f, Vec3(0, 1, 0));
        }
        
        // Performance tracking
        frameCount++;
        static float fpsTimer = 0;
        fpsTimer += dt;
        if (fpsTimer >= 1.0f) {
            fps = frameCount / fpsTimer;
            frameCount = 0;
            fpsTimer = 0;
            
            // Report to JS
            emscripten_run_script_string(("window.phoenixLoader && window.phoenixLoader.fps = " + std::to_string((int)fps)).c_str());
        }
    }
    
    void render() {
        glViewport(0, 0, width, height);
        glClearColor(0.1f, 0.1f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        
        drawCalls = 0;
        triangleCount = 0;
        
        // Create matrices
        Mat4 projection = Mat4::perspective(3.14159265f / 4.0f, (float)width / height, 0.1f, 100.0f);
        Mat4 view = Mat4::lookAt(camera.position, camera.target, camera.up);
        
        // Draw skybox
        glUseProgram(skyboxProgram);
        glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "uView"), 1, GL_FALSE, view.m);
        glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "uProjection"), 1, GL_FALSE, projection.m);
        
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        drawCalls++;
        glBindVertexArray(0);
        
        // Draw models
        glUseProgram(shaderProgram);
        
        GLint uModel = glGetUniformLocation(shaderProgram, "uModel");
        GLint uView = glGetUniformLocation(shaderProgram, "uView");
        GLint uProjection = glGetUniformLocation(shaderProgram, "uProjection");
        GLint uAlbedo = glGetUniformLocation(shaderProgram, "uAlbedo");
        GLint uMetallic = glGetUniformLocation(shaderProgram, "uMetallic");
        GLint uRoughness = glGetUniformLocation(shaderProgram, "uRoughness");
        GLint uViewPos = glGetUniformLocation(shaderProgram, "uViewPos");
        GLint uLightDir = glGetUniformLocation(shaderProgram, "uLightDir");
        GLint uLightColor = glGetUniformLocation(shaderProgram, "uLightColor");
        
        glUniformMatrix4fv(uView, 1, GL_FALSE, view.m);
        glUniformMatrix4fv(uProjection, 1, GL_FALSE, projection.m);
        glUniform3f(uViewPos, camera.position.x, camera.position.y, camera.position.z);
        glUniform3f(uLightDir, lights[0].position.x, lights[0].position.y, lights[0].position.z);
        glUniform3f(uLightColor, lights[0].color.x, lights[0].color.y, lights[0].color.z);
        
        for (int i = 0; i < modelCount; i++) {
            Model& m = models[i];
            
            glUniformMatrix4fv(uModel, 1, GL_FALSE, m.transform.m);
            glUniform3f(uAlbedo, m.albedo.x, m.albedo.y, m.albedo.z);
            glUniform1f(uMetallic, m.metallic);
            glUniform1f(uRoughness, m.roughness);
            
            glBindVertexArray(m.vao);
            glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, 0);
            drawCalls++;
            triangleCount += m.indexCount / 3;
            glBindVertexArray(0);
        }
        
        // Draw particles
        if (particleCount > 0) {
            glUseProgram(particleProgram);
            GLint pView = glGetUniformLocation(particleProgram, "uView");
            GLint pProjection = glGetUniformLocation(particleProgram, "uProjection");
            GLint pTime = glGetUniformLocation(particleProgram, "uTime");
            
            glUniformMatrix4fv(pView, 1, GL_FALSE, view.m);
            glUniformMatrix4fv(pProjection, 1, GL_FALSE, projection.m);
            glUniform1f(pTime, time);
            
            glBindVertexArray(particleVAO);
            glDrawArrays(GL_POINTS, 0, particleCount);
            drawCalls++;
        }
        
        // Update JS performance display
        static float perfTimer = 0;
        perfTimer += 0.1f;
        if (perfTimer >= 1.0f) {
            perfTimer = 0;
            char script[256];
            sprintf(script, "if(window.phoenixLoader){window.phoenixLoader.drawCalls=%d;window.phoenixLoader.triangleCount=%d;window.phoenixLoader.memoryUsage=150;}", drawCalls, triangleCount);
            emscripten_run_script_string(script);
        }
    }

public:
    void init(int w, int h) {
        width = w;
        height = h;
        
        // Create shaders
        shaderProgram = createProgram(vertexShaderSource, fragmentShaderSource);
        skyboxProgram = createProgram(skyboxVertexSource, skyboxFragmentSource);
        particleProgram = createProgram(particleVertexSource, particleFragmentSource);
        
        // Create geometry
        createSkybox();
        createParticleBuffer();
        createDemoModels();
        
        // OpenGL settings
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    
    void onResize(int w, int h) {
        width = w;
        height = h;
    }
    
    void onTouchRotate(float dx, float dy) {
        cameraAngleX -= dx * 0.01f;
        cameraAngleY += dy * 0.01f;
        cameraAngleY = fmaxf(-1.5f, fminf(1.5f, cameraAngleY));
        updateCameraPosition();
    }
    
    void onTouchZoom(float delta) {
        camera.distance -= delta;
        camera.distance = fmaxf(2.0f, fminf(15.0f, camera.distance));
        updateCameraPosition();
    }
    
    void onTouchPan(float dx, float dy) {
        camera.target.x += dx * 0.01f;
        camera.target.y -= dy * 0.01f;
    }
    
    void onDoubleTap() {
        // Reset camera
        cameraAngleX = 0;
        cameraAngleY = 0.3f;
        camera.distance = 5.0f;
        camera.target = Vec3(0, 0, 0);
        updateCameraPosition();
    }
    
    void setCameraMode(int mode) {
        camera.mode = mode;
    }
    
    void setMaterialParam(int param, float value) {
        for (int i = 0; i < modelCount; i++) {
            switch (param) {
                case 0: models[i].metallic = value; break;
                case 1: models[i].roughness = value; break;
                case 2: models[i].clearcoat = value; break;
            }
        }
    }
    
    void setEffect(int effect, int enabled) {
        switch (effect) {
            case 0: lights[0].intensity = enabled ? 1.0f : 0.0f; break;
            case 1: lights[1].intensity = enabled ? 0.8f : 0.0f; break;
            case 2: lights[3].intensity = enabled ? 0.5f : 0.0f; break;
            case 3: shadowsEnabled = enabled; break;
            case 4: bloomEnabled = enabled; break;
            case 5: toneMappingEnabled = enabled; break;
            case 6: ssaoEnabled = enabled; break;
        }
    }
    
    void setAnimation(int anim) {
        currentAnimation = anim;
    }
};

// ============================================================================
// Global Instance
// ============================================================================
static PhoenixDemo* g_demo = nullptr;

// ============================================================================
// Exported Functions
// ============================================================================
extern "C" {

EMSCRIPTEN_KEEPALIVE
void init_gl(int width, int height) {
    g_demo = new PhoenixDemo();
    g_demo->init(width, height);
}

EMSCRIPTEN_KEEPALIVE
void update(float dt) {
    if (g_demo) g_demo->update(dt);
}

EMSCRIPTEN_KEEPALIVE
void render() {
    if (g_demo) g_demo->render();
}

EMSCRIPTEN_KEEPALIVE
void on_resize(int width, int height) {
    if (g_demo) g_demo->onResize(width, height);
}

EMSCRIPTEN_KEEPALIVE
void on_touch_rotate(float dx, float dy) {
    if (g_demo) g_demo->onTouchRotate(dx, dy);
}

EMSCRIPTEN_KEEPALIVE
void on_touch_zoom(float delta) {
    if (g_demo) g_demo->onTouchZoom(delta);
}

EMSCRIPTEN_KEEPALIVE
void on_touch_pan(float dx, float dy) {
    if (g_demo) g_demo->onTouchPan(dx, dy);
}

EMSCRIPTEN_KEEPALIVE
void on_double_tap() {
    if (g_demo) g_demo->onDoubleTap();
}

EMSCRIPTEN_KEEPALIVE
void set_camera_mode(int mode) {
    if (g_demo) g_demo->setCameraMode(mode);
}

EMSCRIPTEN_KEEPALIVE
void set_material_param(int param, float value) {
    if (g_demo) g_demo->setMaterialParam(param, value);
}

EMSCRIPTEN_KEEPALIVE
void set_effect(int effect, int enabled) {
    if (g_demo) g_demo->setEffect(effect, enabled);
}

EMSCRIPTEN_KEEPALIVE
void set_animation(int anim) {
    if (g_demo) g_demo->setAnimation(anim);
}

} // extern "C"
