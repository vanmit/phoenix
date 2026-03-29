/**
 * Phoenix Engine - WASM Demo Application
 * 
 * Refactored to use bgfx rendering backend (Phoenix Engine API)
 * 
 * Compile with:
 *   emcc demo-app.cpp -o phoenix-engine.wasm \
 *     -s WASM=1 -s USE_WEBGL2=1 -s FULL_ES3=1 \
 *     -s ALLOW_MEMORY_GROWTH=1 -s MAX_WEBGL_VERSION=2 \
 *     -s INITIAL_MEMORY=134217728 \
 *     -s EXPORTED_FUNCTIONS='["_main","_demo_init","_demo_update","_demo_render","_demo_shutdown","_demo_resize","_demo_touch_rotate","_demo_touch_zoom","_demo_touch_pan","_demo_double_tap","_demo_set_camera_mode","_demo_set_material_param","_demo_set_effect","_demo_set_animation"]' \
 *     -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
 *     -O3 -flto -I../include -I../third-party/bgfx/include
 */

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <cmath>
#include <cstring>
#include <cstdio>

// bgfx includes
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

// ============================================================================
// Configuration
// ============================================================================
#define MAX_MODELS 10
#define MAX_LIGHTS 16
#define MAX_PARTICLES 1000

// ============================================================================
// Vector/Math Utilities
// ============================================================================
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
};

struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0), y(0), z(0), w(1) {}
    Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

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
        Vec3 f = normalize(center - eye);
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
// Vertex Format for bgfx
// ============================================================================
struct PosColorVertex {
    float m_x;
    float m_y;
    float m_z;
    uint32_t m_abgr;
    
    static bgfx::VertexLayout ms_layout;
    
    static void init() {
        ms_layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
    }
};

bgfx::VertexLayout PosColorVertex::ms_layout;

// ============================================================================
// Application State
// ============================================================================
struct Camera {
    Vec3 position;
    Vec3 target;
    Vec3 up;
    float yaw;
    float pitch;
    float distance;
    int mode; // 0=orbit, 1=fps, 2=third
};

struct Model {
    bgfx::VertexBufferHandle vbh;
    bgfx::IndexBufferHandle ibh;
    uint32_t indexCount;
    Mat4 transform;
    Vec3 albedo;
    float metallic;
    float roughness;
    float clearcoat;
};

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

// ============================================================================
// Phoenix WASM Demo Class
// ============================================================================
class PhoenixWasmDemo {
private:
    int width;
    int height;
    
    Camera camera;
    Model models[MAX_MODELS];
    int modelCount;
    
    Light lights[MAX_LIGHTS];
    int lightCount;
    
    Particle particles[MAX_PARTICLES];
    int particleCount;
    
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
    int currentAnimation;
    float blendSpeed;
    
    // bgfx handles
    bgfx::ProgramHandle program;
    bgfx::UniformHandle u_time;
    bgfx::UniformHandle u_viewProj;
    bgfx::UniformHandle u_albedo;
    bgfx::UniformHandle u_metallic;
    bgfx::UniformHandle u_roughness;

public:
    PhoenixWasmDemo() : width(0), height(0), time(0),
                    cameraAngleX(0), cameraAngleY(0.3f), frameCount(0),
                    fps(60), drawCalls(0), triangleCount(0),
                    shadowsEnabled(true), bloomEnabled(true),
                    toneMappingEnabled(true), currentAnimation(0), blendSpeed(0.3f),
                    modelCount(0), lightCount(0), particleCount(0), program(BGFX_INVALID_HANDLE) {
        
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
    
    ~PhoenixWasmDemo() {
        shutdown();
    }
    
    void updateCameraPosition() {
        if (camera.mode == 0) { // Orbit
            float x = camera.distance * cosf(cameraAngleY) * sinf(cameraAngleX);
            float y = camera.distance * sinf(cameraAngleY);
            float z = camera.distance * cosf(cameraAngleY) * cosf(cameraAngleX);
            camera.position = Vec3(x, y, z);
            camera.target = Vec3(0, 0, 0);
        }
    }
    
    void setupLights() {
        // Directional light (sun)
        lights[lightCount++] = { Vec3(-1, -1, -0.5f), Vec3(1.0f, 0.95f, 0.9f), 1.0f, 0 };
        
        // Point lights
        lights[lightCount++] = { Vec3(2, 3, 2), Vec3(1.0f, 0.5f, 0.2f), 0.8f, 1 };
        lights[lightCount++] = { Vec3(-2, 2, -2), Vec3(0.2f, 0.5f, 1.0f), 0.6f, 1 };
    }
    
    void createCube(float x, float y, float z, float size, uint32_t color) {
        if (modelCount >= MAX_MODELS) return;
        
        Model& model = models[modelCount++];
        
        float half = size / 2.0f;
        PosColorVertex vertices[24] = {
            // Front
            { x-half, y-half, z+half, color },
            { x+half, y-half, z+half, color },
            { x+half, y+half, z+half, color },
            { x-half, y+half, z+half, color },
            // Back
            { x-half, y-half, z-half, color },
            { x-half, y+half, z-half, color },
            { x+half, y+half, z-half, color },
            { x+half, y-half, z-half, color },
            // Top
            { x-half, y+half, z-half, color },
            { x-half, y+half, z+half, color },
            { x+half, y+half, z+half, color },
            { x+half, y+half, z-half, color },
            // Bottom
            { x-half, y-half, z-half, color },
            { x+half, y-half, z-half, color },
            { x+half, y-half, z+half, color },
            { x-half, y-half, z+half, color },
            // Right
            { x+half, y-half, z-half, color },
            { x+half, y+half, z-half, color },
            { x+half, y+half, z+half, color },
            { x+half, y-half, z+half, color },
            // Left
            { x-half, y-half, z-half, color },
            { x-half, y-half, z+half, color },
            { x-half, y+half, z+half, color },
            { x-half, y+half, z-half, color },
        };
        
        uint16_t indices[36] = {
            0,  1,  2,  0,  2,  3,    // Front
            4,  5,  6,  4,  6,  7,    // Back
            8,  9,  10, 8,  10, 11,   // Top
            12, 13, 14, 12, 14, 15,   // Bottom
            16, 17, 18, 16, 18, 19,   // Right
            20, 21, 22, 20, 22, 23    // Left
        };
        
        model.vbh = bgfx::createVertexBuffer(
            bgfx::makeRef(vertices, sizeof(vertices))
        );
        
        model.ibh = bgfx::createIndexBuffer(
            bgfx::makeRef(indices, sizeof(indices))
        );
        
        model.indexCount = 36;
        model.transform = Mat4::identity();
        model.albedo = Vec3(
            ((color >> 24) & 0xFF) / 255.0f,
            ((color >> 16) & 0xFF) / 255.0f,
            ((color >> 8) & 0xFF) / 255.0f
        );
        model.metallic = 0.5f;
        model.roughness = 0.5f;
        model.clearcoat = 0.0f;
    }
    
    void createSphere(float x, float y, float z, float radius, uint32_t color) {
        if (modelCount >= MAX_MODELS) return;
        
        Model& model = models[modelCount++];
        
        // Generate sphere vertices
        int rings = 16, sectors = 32;
        int vertexCount = (rings + 1) * (sectors + 1);
        int indexCount = rings * sectors * 6;
        
        PosColorVertex* vertices = new PosColorVertex[vertexCount];
        uint16_t* indices = new uint16_t[indexCount];
        
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
                
                vertices[idx].m_x = x + radius * nx;
                vertices[idx].m_y = y + radius * ny;
                vertices[idx].m_z = z + radius * nz;
                vertices[idx].m_abgr = color;
                idx++;
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
        
        model.vbh = bgfx::createVertexBuffer(
            bgfx::makeRef(vertices, vertexCount * sizeof(PosColorVertex))
        );
        
        model.ibh = bgfx::createIndexBuffer(
            bgfx::makeRef(indices, indexCount * sizeof(uint16_t))
        );
        
        model.indexCount = indexCount;
        model.transform = Mat4::identity();
        model.albedo = Vec3(
            ((color >> 24) & 0xFF) / 255.0f,
            ((color >> 16) & 0xFF) / 255.0f,
            ((color >> 8) & 0xFF) / 255.0f
        );
        model.metallic = 0.7f;
        model.roughness = 0.3f;
        model.clearcoat = 0.0f;
        
        delete[] vertices;
        delete[] indices;
    }
    
    void createDemoScene() {
        // Create simple geometric models for demo
        // Main showcase object - sphere
        createSphere(0, 0, 0, 1.0f, 0xFF804020);
        
        // Torus-like (using multiple cubes for simplicity)
        createCube(2, 0, -2, 0.8f, 0xFF206080);
        
        // Cube
        createCube(-2, 0, -2, 1.0f, 0xFF307040);
        
        // Pyramid (using cube for simplicity)
        createCube(0, 0, 2, 0.7f, 0xFF907010);
        
        // Cylinder (using cube for simplicity)
        createCube(-1.5f, 0, 1.5f, 0.5f, 0xFF606070);
    }
    
    bool initShader() {
        // Simple vertex shader
        const char* vs_source =
            "#version 100\n"
            "attribute vec3 a_position;\n"
            "attribute vec4 a_color;\n"
            "uniform mat4 u_viewProj;\n"
            "varying vec4 v_color;\n"
            "void main() {\n"
            "  gl_Position = u_viewProj * vec4(a_position, 1.0);\n"
            "  v_color = a_color;\n"
            "}\n";
        
        // Simple fragment shader
        const char* fs_source =
            "#version 100\n"
            "precision mediump float;\n"
            "varying vec4 v_color;\n"
            "void main() {\n"
            "  gl_FragColor = v_color;\n"
            "}\n";
        
        // Compile shaders
        bgfx::ShaderHandle vs = bgfx::createShader(
            bgfx::makeRef(vs_source, (uint32_t)strlen(vs_source))
        );
        bgfx::ShaderHandle fs = bgfx::createShader(
            bgfx::makeRef(fs_source, (uint32_t)strlen(fs_source))
        );
        
        // Create program
        program = bgfx::createProgram(vs, fs, true);
        
        if (!bgfx::isValid(program)) {
            emscripten_console_error("Failed to create shader program");
            return false;
        }
        
        // Create uniforms
        u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
        u_viewProj = bgfx::createUniform("u_viewProj", bgfx::UniformType::Mat4);
        u_albedo = bgfx::createUniform("u_albedo", bgfx::UniformType::Vec4);
        u_metallic = bgfx::createUniform("u_metallic", bgfx::UniformType::Vec4);
        u_roughness = bgfx::createUniform("u_roughness", bgfx::UniformType::Vec4);
        
        return true;
    }
    
    bool init(int w, int h) {
        width = w;
        height = h;
        
        // Initialize bgfx
        bgfx::Init init;
        init.type = bgfx::RendererType::WebGL2;
        init.resolution.width = width;
        init.resolution.height = height;
        init.resolution.reset = BGFX_RESET_VSYNC;
        init.platformData.nwh = nullptr; // WebGL doesn't need native window handle
        
        if (!bgfx::init(init)) {
            emscripten_console_error("Failed to initialize bgfx");
            return false;
        }
        
        // Initialize vertex layout
        PosColorVertex::init();
        
        // Create shaders
        if (!initShader()) {
            return false;
        }
        
        // Create demo scene
        createDemoScene();
        
        // Set debug flags
        bgfx::setDebug(BGFX_DEBUG_TEXT);
        
        // Set view clear color
        bgfx::setViewClear(0
            , BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
            , 0xFF1a1a2e  // Dark blue background
            , 1.0f
            , 0
        );
        
        emscripten_console_log("Phoenix WASM Demo initialized with bgfx");
        return true;
    }
    
    void shutdown() {
        // Destroy models
        for (int i = 0; i < modelCount; i++) {
            bgfx::destroy(models[i].ibh);
            bgfx::destroy(models[i].vbh);
        }
        
        // Destroy program and uniforms
        if (bgfx::isValid(program)) {
            bgfx::destroy(program);
        }
        bgfx::destroy(u_time);
        bgfx::destroy(u_viewProj);
        bgfx::destroy(u_albedo);
        bgfx::destroy(u_metallic);
        bgfx::destroy(u_roughness);
        
        // Shutdown bgfx
        bgfx::shutdown();
        
        emscripten_console_log("Phoenix WASM Demo shutdown complete");
    }
    
    void update(float dt) {
        time += dt;
        
        // Update camera
        updateCameraPosition();
        
        // Update particles (simple simulation)
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
            char script[256];
            sprintf(script, "if(window.phoenixLoader){window.phoenixLoader.fps=%d;}", (int)fps);
            emscripten_run_script_string(script);
        }
    }
    
    void updateParticles(float dt) {
        // Simple particle simulation
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].active) {
                particles[i].life += dt / particles[i].maxLife;
                particles[i].position = particles[i].position + particles[i].velocity * dt;
                particles[i].velocity.y += 2.0f * dt; // Gravity
                
                if (particles[i].life >= 1.0f) {
                    particles[i].active = false;
                }
            }
        }
        
        // Spawn new particles
        static float spawnTimer = 0;
        spawnTimer += dt;
        
        if (spawnTimer >= 0.05f) {
            spawnTimer = 0;
            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (!particles[i].active) {
                    particles[i].position = Vec3(
                        (rand() % 100 - 50) / 100.0f,
                        0,
                        (rand() % 100 - 50) / 100.0f
                    );
                    particles[i].velocity = Vec3(
                        (rand() % 100 - 50) / 500.0f,
                        3.0f + (rand() % 100) / 50.0f,
                        (rand() % 100 - 50) / 500.0f
                    );
                    particles[i].life = 0;
                    particles[i].maxLife = 1.0f + (rand() % 100) / 100.0f;
                    particles[i].active = true;
                    break;
                }
            }
        }
    }
    
    void render() {
        // Set viewport
        bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));
        
        // Touch frame
        bgfx::touch(0);
        
        // Create matrices
        Mat4 projection = Mat4::perspective(3.14159265f / 4.0f, (float)width / height, 0.1f, 100.0f);
        Mat4 view = Mat4::lookAt(camera.position, camera.target, camera.up);
        Mat4 viewProj = Mat4::multiply(projection, view);
        
        // Update uniforms
        float timeVec[4] = { time, 0, 0, 0 };
        bgfx::setUniform(u_time, timeVec);
        bgfx::setUniform(u_viewProj, viewProj.m);
        
        drawCalls = 0;
        triangleCount = 0;
        
        // Render models
        for (int i = 0; i < modelCount; i++) {
            Model& m = models[i];
            
            float albedoVec[4] = { m.albedo.x, m.albedo.y, m.albedo.z, 1.0f };
            float metallicVec[4] = { m.metallic, 0, 0, 0 };
            float roughnessVec[4] = { m.roughness, 0, 0, 0 };
            
            bgfx::setUniform(u_albedo, albedoVec);
            bgfx::setUniform(u_metallic, metallicVec);
            bgfx::setUniform(u_roughness, roughnessVec);
            
            bgfx::setVertexBuffer(0, m.vbh);
            bgfx::setIndexBuffer(m.ibh);
            bgfx::submit(0, program);
            
            drawCalls++;
            triangleCount += m.indexCount / 3;
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
    
    void onResize(int w, int h) {
        width = w;
        height = h;
        bgfx::reset(uint32_t(width), uint32_t(height), BGFX_RESET_VSYNC);
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
        // For now, just log - full implementation would update shader uniforms
        switch (effect) {
            case 0: lights[0].intensity = enabled ? 1.0f : 0.0f; break;
            case 1: lights[1].intensity = enabled ? 0.8f : 0.0f; break;
            case 2: lights[2].intensity = enabled ? 0.6f : 0.0f; break;
            case 3: shadowsEnabled = enabled; break;
            case 4: bloomEnabled = enabled; break;
            case 5: toneMappingEnabled = enabled; break;
        }
    }
    
    void setAnimation(int anim) {
        currentAnimation = anim;
    }
};

// ============================================================================
// Global Instance
// ============================================================================
static PhoenixWasmDemo* g_demo = nullptr;

// ============================================================================
// Exported Functions (C interface for WASM)
// ============================================================================
extern "C" {

EMSCRIPTEN_KEEPALIVE
int demo_init(int width, int height) {
    if (g_demo) {
        return -1; // Already initialized
    }
    
    g_demo = new PhoenixWasmDemo();
    if (!g_demo->init(width, height)) {
        delete g_demo;
        g_demo = nullptr;
        return -2; // Initialization failed
    }
    
    return 0; // Success
}

EMSCRIPTEN_KEEPALIVE
void demo_update(float dt) {
    if (g_demo) {
        g_demo->update(dt);
    }
}

EMSCRIPTEN_KEEPALIVE
void demo_render() {
    if (g_demo) {
        g_demo->render();
    }
}

EMSCRIPTEN_KEEPALIVE
void demo_shutdown() {
    if (g_demo) {
        g_demo->shutdown();
        delete g_demo;
        g_demo = nullptr;
    }
}

EMSCRIPTEN_KEEPALIVE
void demo_resize(int width, int height) {
    if (g_demo) {
        g_demo->onResize(width, height);
    }
}

EMSCRIPTEN_KEEPALIVE
void demo_touch_rotate(float dx, float dy) {
    if (g_demo) {
        g_demo->onTouchRotate(dx, dy);
    }
}

EMSCRIPTEN_KEEPALIVE
void demo_touch_zoom(float delta) {
    if (g_demo) {
        g_demo->onTouchZoom(delta);
    }
}

EMSCRIPTEN_KEEPALIVE
void demo_touch_pan(float dx, float dy) {
    if (g_demo) {
        g_demo->onTouchPan(dx, deltaY);
    }
}

EMSCRIPTEN_KEEPALIVE
void demo_double_tap() {
    if (g_demo) {
        g_demo->onDoubleTap();
    }
}

EMSCRIPTEN_KEEPALIVE
void demo_set_camera_mode(int mode) {
    if (g_demo) {
        g_demo->setCameraMode(mode);
    }
}

EMSCRIPTEN_KEEPALIVE
void demo_set_material_param(int param, float value) {
    if (g_demo) {
        g_demo->setMaterialParam(param, value);
    }
}

EMSCRIPTEN_KEEPALIVE
void demo_set_effect(int effect, int enabled) {
    if (g_demo) {
        g_demo->setEffect(effect, enabled);
    }
}

EMSCRIPTEN_KEEPALIVE
void demo_set_animation(int anim) {
    if (g_demo) {
        g_demo->setAnimation(anim);
    }
}

} // extern "C"

// ============================================================================
// Main Entry Point
// ============================================================================
int main() {
    emscripten_console_log("Phoenix WASM Demo starting...");
    // Initialization happens via demo_init() export
    return 0;
}
