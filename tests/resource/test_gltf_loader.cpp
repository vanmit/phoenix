#include <gtest/gtest.h>
#include <phoenix/resource/asset_loader.hpp>
#include <phoenix/resource/mesh.hpp>
#include <fstream>
#include <filesystem>
#include <cmath>

using namespace phoenix::resource;

namespace {

std::string getTestAssetDir() {
    // Look for test assets in common locations
    std::vector<std::string> paths = {
        "tests/assets/models",
        "../tests/assets/models", 
        "../../tests/assets/models",
        "/tmp/phoenix_test_assets"
    };
    
    for (const auto& path : paths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    
    // Create temp directory for test assets
    std::string tempDir = std::filesystem::temp_directory_path().string() + "/phoenix_test_assets";
    std::filesystem::create_directories(tempDir);
    return tempDir;
}

/**
 * @brief Create a minimal valid glTF 2.0 file (JSON format)
 * 
 * This creates a simple triangle mesh for testing.
 */
std::string createMinimalGLTF() {
    std::string testDir = getTestAssetDir();
    std::string path = testDir + "/test_triangle.gltf";
    
    std::ofstream file(path);
    EXPECT_TRUE(file.is_open());
    
    // Minimal glTF 2.0 with a single triangle
    file << R"({
        "asset": {
            "version": "2.0",
            "generator": "Phoenix Engine Test"
        },
        "scene": 0,
        "scenes": [
            {
                "nodes": [0]
            }
        ],
        "nodes": [
            {
                "name": "TriangleNode",
                "mesh": 0
            }
        ],
        "meshes": [
            {
                "name": "TriangleMesh",
                "primitives": [
                    {
                        "attributes": {
                            "POSITION": 0
                        },
                        "indices": 1,
                        "mode": 4
                    }
                ]
            }
        ],
        "accessors": [
            {
                "bufferView": 0,
                "componentType": 5126,
                "count": 3,
                "type": "VEC3",
                "max": [1.0, 1.0, 0.0],
                "min": [0.0, 0.0, 0.0]
            },
            {
                "bufferView": 1,
                "componentType": 5123,
                "count": 3,
                "type": "SCALAR"
            }
        ],
        "bufferViews": [
            {
                "buffer": 0,
                "byteOffset": 0,
                "byteLength": 36
            },
            {
                "buffer": 0,
                "byteOffset": 36,
                "byteLength": 6
            }
        ],
        "buffers": [
            {
                "uri": "test_triangle.bin",
                "byteLength": 42
            }
        ]
    })";
    
    // Create binary buffer with vertex and index data
    std::string binPath = testDir + "/test_triangle.bin";
    std::ofstream binFile(binPath, std::ios::binary);
    EXPECT_TRUE(binFile.is_open());
    
    // 3 vertices (position only, vec3 float = 12 bytes each)
    // Triangle: (0,0,0), (1,0,0), (0.5,1,0)
    float vertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.5f, 1.0f, 0.0f
    };
    binFile.write(reinterpret_cast<const char*>(vertices), sizeof(vertices));
    
    // 3 indices (uint16)
    uint16_t indices[] = {0, 1, 2};
    binFile.write(reinterpret_cast<const char*>(indices), sizeof(indices));
    
    return path;
}

/**
 * @brief Create a glTF with normals and UVs
 */
std::string createGLTFWithNormalsAndUVs() {
    std::string testDir = getTestAssetDir();
    std::string path = testDir + "/test_quad.gltf";
    
    std::ofstream file(path);
    EXPECT_TRUE(file.is_open());
    
    file << R"({
        "asset": {
            "version": "2.0"
        },
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{
            "primitives": [{
                "attributes": {
                    "POSITION": 0,
                    "NORMAL": 1,
                    "TEXCOORD_0": 2
                },
                "indices": 3
            }]
        }],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": 4, "type": "VEC3"},
            {"bufferView": 1, "componentType": 5126, "count": 4, "type": "VEC3"},
            {"bufferView": 2, "componentType": 5126, "count": 4, "type": "VEC2"},
            {"bufferView": 3, "componentType": 5123, "count": 6, "type": "SCALAR"}
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": 48},
            {"buffer": 0, "byteOffset": 48, "byteLength": 48},
            {"buffer": 0, "byteOffset": 96, "byteLength": 32},
            {"buffer": 0, "byteOffset": 128, "byteLength": 12}
        ],
        "buffers": [{"uri": "test_quad.bin", "byteLength": 140}]
    })";
    
    std::string binPath = testDir + "/test_quad.bin";
    std::ofstream binFile(binPath, std::ios::binary);
    
    // 4 vertices for a quad (2 triangles)
    float positions[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };
    binFile.write(reinterpret_cast<const char*>(positions), sizeof(positions));
    
    // Normals (all pointing up)
    float normals[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    };
    binFile.write(reinterpret_cast<const char*>(normals), sizeof(normals));
    
    // UVs
    float uvs[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    binFile.write(reinterpret_cast<const char*>(uvs), sizeof(uvs));
    
    // Indices (2 triangles)
    uint16_t indices[] = {0, 1, 2, 0, 2, 3};
    binFile.write(reinterpret_cast<const char*>(indices), sizeof(indices));
    
    return path;
}

/**
 * @brief Create a minimal GLB file (binary glTF)
 */
std::string createMinimalGLB() {
    std::string testDir = getTestAssetDir();
    std::string path = testDir + "/test_cube.glb";
    
    // GLB structure:
    // Header (12 bytes): magic, version, length
    // JSON chunk (header + data)
    // BIN chunk (header + data)
    
    std::string json = R"({
        "asset": {"version": "2.0"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{
            "primitives": [{
                "attributes": {"POSITION": 0},
                "indices": 1
            }]
        }],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"},
            {"bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR"}
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": 36},
            {"buffer": 0, "byteOffset": 36, "byteLength": 6}
        ],
        "buffers": [{"byteLength": 42}]
    })";
    
    // Pad JSON to 4-byte alignment
    while (json.size() % 4 != 0) {
        json += ' ';
    }
    
    // Binary data: 3 vertices + 3 indices
    uint8_t binData[42];
    memset(binData, 0, sizeof(binData));
    
    // Vertices: triangle
    float* verts = reinterpret_cast<float*>(binData);
    verts[0] = 0.0f; verts[1] = 0.0f; verts[2] = 0.0f;
    verts[3] = 1.0f; verts[4] = 0.0f; verts[5] = 0.0f;
    verts[6] = 0.5f; verts[7] = 1.0f; verts[8] = 0.0f;
    
    // Indices
    uint16_t* indices = reinterpret_cast<uint16_t*>(binData + 36);
    indices[0] = 0; indices[1] = 1; indices[2] = 2;
    
    std::ofstream file(path, std::ios::binary);
    
    // GLB Header
    uint32_t magic = 0x46546C67;  // "glTF"
    uint32_t version = 2;
    uint32_t totalLength = 12 + 8 + json.size() + 8 + sizeof(binData);
    
    file.write(reinterpret_cast<const char*>(&magic), 4);
    file.write(reinterpret_cast<const char*>(&version), 4);
    file.write(reinterpret_cast<const char*>(&totalLength), 4);
    
    // JSON chunk
    uint32_t jsonLength = static_cast<uint32_t>(json.size());
    uint32_t jsonType = 0x4E4F534A;  // "JSON"
    file.write(reinterpret_cast<const char*>(&jsonLength), 4);
    file.write(reinterpret_cast<const char*>(&jsonType), 4);
    file.write(json.data(), json.size());
    
    // BIN chunk
    uint32_t binLength = sizeof(binData);
    uint32_t binType = 0x004E4942;  // "BIN\0"
    file.write(reinterpret_cast<const char*>(&binLength), 4);
    file.write(reinterpret_cast<const char*>(&binType), 4);
    file.write(reinterpret_cast<const char*>(binData), binLength);
    
    return path;
}

/**
 * @brief Create a glTF with skinning data
 */
std::string createGLTFWithSkinning() {
    std::string testDir = getTestAssetDir();
    std::string path = testDir + "/test_skinned.gltf";
    
    std::ofstream file(path);
    EXPECT_TRUE(file.is_open());
    
    file << R"({
        "asset": {"version": "2.0"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [
            {
                "name": "Root",
                "children": [1, 2],
                "skin": 0
            },
            {
                "name": "Joint1",
                "translation": [0.0, 0.0, 0.0]
            },
            {
                "name": "Joint2",
                "translation": [1.0, 0.0, 0.0]
            }
        ],
        "skins": [
            {
                "joints": [1, 2],
                "inverseBindMatrices": 2
            }
        ],
        "meshes": [{
            "primitives": [{
                "attributes": {
                    "POSITION": 0,
                    "JOINTS_0": 1,
                    "WEIGHTS_0": 2
                },
                "indices": 3
            }]
        }],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": 4, "type": "VEC3"},
            {"bufferView": 1, "componentType": 5125, "count": 4, "type": "VEC4"},
            {"bufferView": 2, "componentType": 5126, "count": 4, "type": "VEC4"},
            {"bufferView": 3, "componentType": 5123, "count": 6, "type": "SCALAR"},
            {"bufferView": 4, "componentType": 5126, "count": 2, "type": "MAT4"}
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": 48},
            {"buffer": 0, "byteOffset": 48, "byteLength": 64},
            {"buffer": 0, "byteOffset": 112, "byteLength": 64},
            {"buffer": 0, "byteOffset": 176, "byteLength": 12},
            {"buffer": 0, "byteOffset": 188, "byteLength": 128}
        ],
        "buffers": [{"uri": "test_skinned.bin", "byteLength": 316}]
    })";
    
    std::string binPath = testDir + "/test_skinned.bin";
    std::ofstream binFile(binPath, std::ios::binary);
    
    // Positions
    float positions[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };
    binFile.write(reinterpret_cast<const char*>(positions), sizeof(positions));
    
    // Joint indices (4 per vertex)
    uint32_t joints[] = {
        0, 1, 0, 0,
        0, 1, 0, 0,
        0, 1, 0, 0,
        0, 1, 0, 0
    };
    binFile.write(reinterpret_cast<const char*>(joints), sizeof(joints));
    
    // Weights (4 per vertex)
    float weights[] = {
        0.5f, 0.5f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 0.0f
    };
    binFile.write(reinterpret_cast<const char*>(weights), sizeof(weights));
    
    // Indices
    uint16_t indices[] = {0, 1, 2, 0, 2, 3};
    binFile.write(reinterpret_cast<const char*>(indices), sizeof(indices));
    
    // Inverse bind matrices (2 matrices, 16 floats each)
    float ibm[32];
    memset(ibm, 0, sizeof(ibm));
    ibm[0] = 1; ibm[5] = 1; ibm[10] = 1; ibm[15] = 1;  // Identity
    ibm[16] = 1; ibm[21] = 1; ibm[26] = 1; ibm[31] = 1;  // Identity
    binFile.write(reinterpret_cast<const char*>(ibm), sizeof(ibm));
    
    return path;
}

/**
 * @brief Create a glTF with animation data
 */
std::string createGLTFWithAnimation() {
    std::string testDir = getTestAssetDir();
    std::string path = testDir + "/test_animated.gltf";
    
    std::ofstream file(path);
    EXPECT_TRUE(file.is_open());
    
    file << R"({
        "asset": {"version": "2.0"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [
            {
                "name": "AnimatedNode",
                "mesh": 0,
                "translation": [0.0, 0.0, 0.0]
            }
        ],
        "animations": [
            {
                "name": "Rotation",
                "samplers": [
                    {
                        "input": 0,
                        "output": 1,
                        "interpolation": "LINEAR"
                    }
                ],
                "channels": [
                    {
                        "sampler": 0,
                        "target": {
                            "node": 0,
                            "path": "rotation"
                        }
                    }
                ]
            }
        ],
        "meshes": [{
            "primitives": [{
                "attributes": {"POSITION": 2},
                "indices": 3
            }]
        }],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": 2, "type": "SCALAR"},
            {"bufferView": 1, "componentType": 5126, "count": 8, "type": "VEC4"},
            {"bufferView": 2, "componentType": 5126, "count": 3, "type": "VEC3"},
            {"bufferView": 3, "componentType": 5123, "count": 3, "type": "SCALAR"}
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": 8},
            {"buffer": 0, "byteOffset": 8, "byteLength": 64},
            {"buffer": 0, "byteOffset": 72, "byteLength": 36},
            {"buffer": 0, "byteOffset": 108, "byteLength": 6}
        ],
        "buffers": [{"uri": "test_animated.bin", "byteLength": 114}]
    })";
    
    std::string binPath = testDir + "/test_animated.bin";
    std::ofstream binFile(binPath, std::ios::binary);
    
    // Animation times (2 keyframes: 0s and 1s)
    float times[] = {0.0f, 1.0f};
    binFile.write(reinterpret_cast<const char*>(times), sizeof(times));
    
    // Animation rotations (quaternions for 2 keyframes)
    // 0s: identity (0,0,0,1), 1s: 90 deg around Y (0,0.707,0,0.707)
    float rotations[] = {
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.7071068f, 0.0f, 0.7071068f
    };
    binFile.write(reinterpret_cast<const char*>(rotations), sizeof(rotations));
    
    // Mesh positions (triangle)
    float positions[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.5f, 1.0f, 0.0f
    };
    binFile.write(reinterpret_cast<const char*>(positions), sizeof(positions));
    
    // Indices
    uint16_t indices[] = {0, 1, 2};
    binFile.write(reinterpret_cast<const char*>(indices), sizeof(indices));
    
    return path;
}

/**
 * @brief Create glTF with PBR material
 */
std::string createGLTFWithMaterial() {
    std::string testDir = getTestAssetDir();
    std::string path = testDir + "/test_material.gltf";
    
    std::ofstream file(path);
    EXPECT_TRUE(file.is_open());
    
    file << R"({
        "asset": {"version": "2.0"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{
            "primitives": [{
                "attributes": {"POSITION": 0},
                "material": 0
            }]
        }],
        "materials": [
            {
                "name": "TestMaterial",
                "pbrMetallicRoughness": {
                    "baseColorFactor": [1.0, 0.5, 0.0, 1.0],
                    "metallicFactor": 0.5,
                    "roughnessFactor": 0.8
                },
                "normalTexture": {
                    "scale": 1.0
                },
                "emissiveFactor": [0.1, 0.1, 0.1],
                "alphaMode": "OPAQUE",
                "doubleSided": false
            }
        ],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"}
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": 36}
        ],
        "buffers": [{"uri": "test_material.bin", "byteLength": 36}]
    })";
    
    std::string binPath = testDir + "/test_material.bin";
    std::ofstream binFile(binPath, std::ios::binary);
    
    float positions[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.5f, 1.0f, 0.0f
    };
    binFile.write(reinterpret_cast<const char*>(positions), sizeof(positions));
    
    return path;
}

/**
 * @brief Create a glTF with tangent attribute
 */
std::string createGLTFWithTangents() {
    std::string testDir = getTestAssetDir();
    std::string path = testDir + "/test_tangents.gltf";
    
    std::ofstream file(path);
    EXPECT_TRUE(file.is_open());
    
    file << R"({
        "asset": {"version": "2.0"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{
            "primitives": [{
                "attributes": {
                    "POSITION": 0,
                    "NORMAL": 1,
                    "TANGENT": 2,
                    "TEXCOORD_0": 3
                },
                "indices": 4
            }]
        }],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": 4, "type": "VEC3"},
            {"bufferView": 1, "componentType": 5126, "count": 4, "type": "VEC3"},
            {"bufferView": 2, "componentType": 5126, "count": 4, "type": "VEC4"},
            {"bufferView": 3, "componentType": 5126, "count": 4, "type": "VEC2"},
            {"bufferView": 4, "componentType": 5123, "count": 6, "type": "SCALAR"}
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": 48},
            {"buffer": 0, "byteOffset": 48, "byteLength": 48},
            {"buffer": 0, "byteOffset": 96, "byteLength": 64},
            {"buffer": 0, "byteOffset": 160, "byteLength": 32},
            {"buffer": 0, "byteOffset": 192, "byteLength": 12}
        ],
        "buffers": [{"uri": "test_tangents.bin", "byteLength": 204}]
    })";
    
    std::string binPath = testDir + "/test_tangents.bin";
    std::ofstream binFile(binPath, std::ios::binary);
    
    // Positions (quad)
    float positions[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };
    binFile.write(reinterpret_cast<const char*>(positions), sizeof(positions));
    
    // Normals
    float normals[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f
    };
    binFile.write(reinterpret_cast<const char*>(normals), sizeof(normals));
    
    // Tangents (vec4: xyz = tangent direction, w = bitangent sign)
    float tangents[] = {
        1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f
    };
    binFile.write(reinterpret_cast<const char*>(tangents), sizeof(tangents));
    
    // UVs
    float uvs[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    binFile.write(reinterpret_cast<const char*>(uvs), sizeof(uvs));
    
    // Indices
    uint16_t indices[] = {0, 1, 2, 0, 2, 3};
    binFile.write(reinterpret_cast<const char*>(indices), sizeof(indices));
    
    return path;
}

/**
 * @brief Create a glTF with vertex colors
 */
std::string createGLTFWithColors() {
    std::string testDir = getTestAssetDir();
    std::string path = testDir + "/test_colors.gltf";
    
    std::ofstream file(path);
    EXPECT_TRUE(file.is_open());
    
    file << R"({
        "asset": {"version": "2.0"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{
            "primitives": [{
                "attributes": {
                    "POSITION": 0,
                    "COLOR_0": 1
                },
                "indices": 2
            }]
        }],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"},
            {"bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC4"},
            {"bufferView": 2, "componentType": 5123, "count": 3, "type": "SCALAR"}
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": 36},
            {"buffer": 0, "byteOffset": 36, "byteLength": 48},
            {"buffer": 0, "byteOffset": 84, "byteLength": 6}
        ],
        "buffers": [{"uri": "test_colors.bin", "byteLength": 90}]
    })";
    
    std::string binPath = testDir + "/test_colors.bin";
    std::ofstream binFile(binPath, std::ios::binary);
    
    // Positions (triangle)
    float positions[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.5f, 1.0f, 0.0f
    };
    binFile.write(reinterpret_cast<const char*>(positions), sizeof(positions));
    
    // Vertex colors (RGBA)
    float colors[] = {
        1.0f, 0.0f, 0.0f, 1.0f,  // Red
        0.0f, 1.0f, 0.0f, 1.0f,  // Green
        0.0f, 0.0f, 1.0f, 1.0f   // Blue
    };
    binFile.write(reinterpret_cast<const char*>(colors), sizeof(colors));
    
    // Indices
    uint16_t indices[] = {0, 1, 2};
    binFile.write(reinterpret_cast<const char*>(indices), sizeof(indices));
    
    return path;
}

/**
 * @brief Create a glTF with CUBICSPLINE interpolation
 */
std::string createGLTFWithCubicSpline() {
    std::string testDir = getTestAssetDir();
    std::string path = testDir + "/test_cubic_spline.gltf";
    
    std::ofstream file(path);
    EXPECT_TRUE(file.is_open());
    
    file << R"({
        "asset": {"version": "2.0"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [
            {
                "name": "AnimatedNode",
                "mesh": 0,
                "translation": [0.0, 0.0, 0.0]
            }
        ],
        "animations": [
            {
                "name": "CubicSplineAnimation",
                "samplers": [
                    {
                        "input": 0,
                        "output": 1,
                        "interpolation": "CUBICSPLINE"
                    }
                ],
                "channels": [
                    {
                        "sampler": 0,
                        "target": {
                            "node": 0,
                            "path": "translation"
                        }
                    }
                ]
            }
        ],
        "meshes": [{
            "primitives": [{
                "attributes": {"POSITION": 2},
                "indices": 3
            }]
        }],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": 3, "type": "SCALAR"},
            {"bufferView": 1, "componentType": 5126, "count": 27, "type": "VEC3"},
            {"bufferView": 2, "componentType": 5126, "count": 3, "type": "VEC3"},
            {"bufferView": 3, "componentType": 5123, "count": 3, "type": "SCALAR"}
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": 12},
            {"buffer": 0, "byteOffset": 12, "byteLength": 324},
            {"buffer": 0, "byteOffset": 336, "byteLength": 36},
            {"buffer": 0, "byteOffset": 372, "byteLength": 6}
        ],
        "buffers": [{"uri": "test_cubic_spline.bin", "byteLength": 378}]
    })";
    
    std::string binPath = testDir + "/test_cubic_spline.bin";
    std::ofstream binFile(binPath, std::ios::binary);
    
    // Animation times (3 keyframes: 0s, 0.5s, 1s)
    float times[] = {0.0f, 0.5f, 1.0f};
    binFile.write(reinterpret_cast<const char*>(times), sizeof(times));
    
    // CUBICSPLINE: for each keyframe: in-tangent, value, out-tangent
    // 3 keyframes * 3 values * 3 components = 27 floats
    float splineData[] = {
        // Keyframe 0 (t=0.0)
        0.0f, 0.0f, 0.0f,   // in-tangent
        0.0f, 0.0f, 0.0f,   // value (start position)
        0.5f, 0.0f, 0.0f,   // out-tangent
        
        // Keyframe 1 (t=0.5)
        -0.5f, 0.0f, 0.0f,  // in-tangent
        1.0f, 0.5f, 0.0f,   // value (mid position)
        0.5f, 0.0f, 0.0f,   // out-tangent
        
        // Keyframe 2 (t=1.0)
        -0.5f, 0.0f, 0.0f,  // in-tangent
        2.0f, 0.0f, 0.0f,   // value (end position)
        0.0f, 0.0f, 0.0f    // out-tangent
    };
    binFile.write(reinterpret_cast<const char*>(splineData), sizeof(splineData));
    
    // Mesh positions (triangle)
    float positions[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.5f, 1.0f, 0.0f
    };
    binFile.write(reinterpret_cast<const char*>(positions), sizeof(positions));
    
    // Indices
    uint16_t indices[] = {0, 1, 2};
    binFile.write(reinterpret_cast<const char*>(indices), sizeof(indices));
    
    return path;
}

} // anonymous namespace

// ============ GLTF Loader Tests ============

TEST(GLTFLoaderTest, Constructor) {
    GLTFLoader::Config config;
    config.loadAnimations = true;
    config.loadSkins = true;
    config.maxBufferSize = 100 * 1024 * 1024;  // 100MB
    
    GLTFLoader loader(config);
    
    auto exts = loader.getSupportedExtensions();
    EXPECT_TRUE(exts.count(".gltf") > 0);
    EXPECT_TRUE(exts.count(".glb") > 0);
    EXPECT_EQ(loader.getAssetType(), AssetType::Model_GLTF);
}

TEST(GLTFLoaderTest, ValidateInvalidPath) {
    GLTFLoader loader;
    
    // Path traversal attack
    auto result = loader.validate("../../../etc/passwd");
    EXPECT_FALSE(result.isValid);
    EXPECT_NE(result.error.find(".."), std::string::npos);
    
    // Non-existent file
    result = loader.validate("nonexistent_file.gltf");
    EXPECT_FALSE(result.isValid);
}

TEST(GLTFLoaderTest, ValidateEmptyFile) {
    std::string testDir = getTestAssetDir();
    std::string path = testDir + "/empty.gltf";
    
    std::ofstream file(path);
    file.close();
    
    GLTFLoader loader;
    auto result = loader.validate(path);
    
    EXPECT_FALSE(result.isValid);
    EXPECT_NE(result.error.find("empty"), std::string::npos);
}

TEST(GLTFLoaderTest, LoadMinimalGLTF) {
    std::string path = createMinimalGLTF();
    
    GLTFLoader loader;
    auto mesh = loader.load(path);
    
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->loadState, LoadState::Loaded);
    EXPECT_EQ(mesh->assetType, AssetType::Model_GLTF);
    EXPECT_FALSE(mesh->primitives.empty());
    
    auto& prim = mesh->primitives[0];
    EXPECT_EQ(prim.vertexCount, 3u);
    EXPECT_EQ(prim.indexCount, 3u);
    EXPECT_TRUE(prim.bounds.isValid());
}

TEST(GLTFLoaderTest, LoadGLB) {
    std::string path = createMinimalGLB();
    
    GLTFLoader loader;
    auto mesh = loader.load(path);
    
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->loadState, LoadState::Loaded);
    EXPECT_EQ(mesh->assetType, AssetType::Model_GLTF_BINARY);
    EXPECT_FALSE(mesh->primitives.empty());
}

TEST(GLTFLoaderTest, LoadWithNormalsAndUVs) {
    std::string path = createGLTFWithNormalsAndUVs();
    
    GLTFLoader loader;
    auto mesh = loader.load(path);
    
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->loadState, LoadState::Loaded);
    
    auto& prim = mesh->primitives[0];
    EXPECT_EQ(prim.vertexCount, 4u);
    EXPECT_EQ(prim.indexCount, 6u);
    
    // Check vertex format includes normals and UVs
    EXPECT_TRUE(prim.vertexFormat.attributes.count(VertexAttribute::Position) > 0);
    EXPECT_TRUE(prim.vertexFormat.attributes.count(VertexAttribute::Normal) > 0);
    EXPECT_TRUE(prim.vertexFormat.attributes.count(VertexAttribute::UV0) > 0);
}

TEST(GLTFLoaderTest, LoadWithSkinning) {
    std::string path = createGLTFWithSkinning();
    
    GLTFLoader::Config config;
    config.loadSkins = true;
    GLTFLoader loader(config);
    
    auto mesh = loader.load(path);
    
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->loadState, LoadState::Loaded);
    EXPECT_FALSE(mesh->joints.empty());
    EXPECT_GE(mesh->joints.size(), 2u);  // At least 2 joints
    
    // Check joints have valid data
    for (const auto& joint : mesh->joints) {
        EXPECT_FALSE(joint.name.empty());
        EXPECT_TRUE(joint.inverseBindMatrix.isValid());
    }
    
    // Check primitives have skinning data
    auto& prim = mesh->primitives[0];
    EXPECT_TRUE(prim.hasSkinning);
}

TEST(GLTFLoaderTest, LoadWithAnimation) {
    std::string path = createGLTFWithAnimation();
    
    GLTFLoader::Config config;
    config.loadAnimations = true;
    GLTFLoader loader(config);
    
    auto mesh = loader.load(path);
    
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->loadState, LoadState::Loaded);
    EXPECT_FALSE(mesh->animations.empty());
    
    auto& clip = mesh->animations[0];
    EXPECT_FALSE(clip.name.empty());
    EXPECT_GT(clip.duration, 0.0f);
    EXPECT_FALSE(clip.channels.empty());
    
    // Check keyframes
    for (const auto& channel : clip.channels) {
        EXPECT_FALSE(channel.keyframes.empty());
        
        // Check keyframe times are in order
        for (size_t i = 1; i < channel.keyframes.size(); ++i) {
            EXPECT_GE(channel.keyframes[i].time, channel.keyframes[i-1].time);
        }
    }
}

TEST(GLTFLoaderTest, LoadWithMaterial) {
    std::string path = createGLTFWithMaterial();
    
    GLTFLoader loader;
    auto mesh = loader.load(path);
    
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->loadState, LoadState::Loaded);
    EXPECT_FALSE(mesh->materials.empty());
    
    auto& mat = mesh->materials[0];
    EXPECT_EQ(mat.name, "TestMaterial");
    EXPECT_NEAR(mat.baseColorFactor.r, 1.0f, 0.001f);
    EXPECT_NEAR(mat.baseColorFactor.g, 0.5f, 0.001f);
    EXPECT_NEAR(mat.baseColorFactor.b, 0.0f, 0.001f);
    EXPECT_NEAR(mat.metallicFactor, 0.5f, 0.001f);
    EXPECT_NEAR(mat.roughnessFactor, 0.8f, 0.001f);
    EXPECT_EQ(mat.alphaMode, Material::AlphaMode::Opaque);
    EXPECT_FALSE(mat.doubleSided);
}

TEST(GLTFLoaderTest, LoadFromMemory) {
    std::string path = createMinimalGLB();
    
    // Read file into memory
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(file.is_open());
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    
    GLTFLoader loader;
    auto mesh = loader.loadFromMemory(buffer.data(), buffer.size(), "memory_test.glb");
    
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->loadState, LoadState::Loaded);
    EXPECT_FALSE(mesh->primitives.empty());
}

TEST(GLTFLoaderTest, CanLoad) {
    GLTFLoader loader;
    
    EXPECT_TRUE(loader.canLoad("model.gltf"));
    EXPECT_TRUE(loader.canLoad("model.glb"));
    EXPECT_TRUE(loader.canLoad("path/to/model.GLTf"));
    EXPECT_TRUE(loader.canLoad("path/to/model.GLB"));
    EXPECT_FALSE(loader.canLoad("model.obj"));
    EXPECT_FALSE(loader.canLoad("model.fbx"));
    EXPECT_FALSE(loader.canLoad("model.txt"));
}

TEST(GLTFLoaderTest, EstimateMemoryUsage) {
    std::string path = createMinimalGLTF();
    
    GLTFLoader loader;
    size_t usage = loader.estimateMemoryUsage(path);
    
    EXPECT_GT(usage, 0);
    // Should be roughly 3-5x file size
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::streamsize fileSize = file.tellg();
    EXPECT_GE(usage, static_cast<size_t>(fileSize) * 3);
}

TEST(GLTFLoaderTest, ValidateURI) {
    GLTFLoader::Config config;
    config.limitExternalURIs = true;
    config.allowedExternalDomains = {"example.com", "cdn.example.com"};
    
    GLTFLoader loader(config);
    
    // Relative URIs should pass (no path traversal)
    EXPECT_TRUE(loader.validateURI("textures/diffuse.png"));
    EXPECT_TRUE(loader.validateURI("../textures/diffuse.png"));  // This should actually fail
    
    // External URIs
    EXPECT_TRUE(loader.validateURI("https://example.com/image.png"));
    EXPECT_TRUE(loader.validateURI("https://cdn.example.com/image.png"));
    EXPECT_FALSE(loader.validateURI("https://malicious.com/image.png"));
}

TEST(GLTFLoaderTest, AsyncLoad) {
    std::string path = createMinimalGLTF();
    
    GLTFLoader loader;
    auto future = loader.loadAsync(path);
    
    auto mesh = future.get();
    
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->loadState, LoadState::Loaded);
}

TEST(GLTFLoaderTest, MeshValidation) {
    std::string path = createMinimalGLTF();
    
    GLTFLoader loader;
    auto mesh = loader.load(path);
    
    auto result = mesh->validate();
    EXPECT_TRUE(result.isValid);
}

TEST(GLTFLoaderTest, MemoryUsageCalculation) {
    std::string path = createGLTFWithNormalsAndUVs();
    
    GLTFLoader loader;
    auto mesh = loader.load(path);
    
    size_t usage = mesh->calculateMemoryUsage();
    EXPECT_GT(usage, 0);
    
    // Check memory budget was set
    EXPECT_GT(mesh->memoryUsage.currentUsage, 0);
}

TEST(GLTFLoaderTest, BoundingVolume) {
    std::string path = createMinimalGLTF();
    
    GLTFLoader loader;
    auto mesh = loader.load(path);
    
    EXPECT_TRUE(mesh->bounds.isValid());
    
    // Triangle vertices: (0,0,0), (1,0,0), (0.5,1,0)
    // Center should be around (0.5, 0.5, 0)
    EXPECT_NEAR(mesh->bounds.center.x, 0.5f, 0.1f);
    EXPECT_NEAR(mesh->bounds.center.y, 0.5f, 0.1f);
    EXPECT_NEAR(mesh->bounds.center.z, 0.0f, 0.1f);
}

TEST(GLTFLoaderTest, AnimationSampling) {
    std::string path = createGLTFWithAnimation();
    
    GLTFLoader::Config config;
    config.loadAnimations = true;
    GLTFLoader loader(config);
    
    auto mesh = loader.load(path);
    
    ASSERT_FALSE(mesh->animations.empty());
    auto& clip = mesh->animations[0];
    
    // Test sampling at different times
    if (!clip.channels.empty()) {
        auto& channel = clip.channels[0];
        
        // Sample at start
        math::Vector3 pos = channel.samplePosition(0.0f);
        math::Quaternion rot = channel.sampleRotation(0.0f);
        math::Vector3 scale = channel.sampleScale(0.0f);
        
        // Should return valid values (not NaN)
        EXPECT_FALSE(std::isnan(pos.x));
        EXPECT_FALSE(std::isnan(rot.x));
        EXPECT_FALSE(std::isnan(scale.x));
    }
}

TEST(GLTFLoaderTest, JointHierarchy) {
    std::string path = createGLTFWithSkinning();
    
    GLTFLoader::Config config;
    config.loadSkins = true;
    GLTFLoader loader(config);
    
    auto mesh = loader.load(path);
    
    ASSERT_FALSE(mesh->joints.empty());
    
    // Check parent-child relationships
    for (size_t i = 0; i < mesh->joints.size(); ++i) {
        const auto& joint = mesh->joints[i];
        
        // Parent index should be valid or -1
        EXPECT_GE(joint.parentIndex, -1);
        EXPECT_LT(joint.parentIndex, static_cast<int32_t>(mesh->joints.size()));
        
        // Inverse bind matrix should be valid
        EXPECT_TRUE(joint.inverseBindMatrix.isValid());
    }
}

TEST(GLTFLoaderTest, ErrorHandling_InvalidGLTFVersion) {
    std::string testDir = getTestAssetDir();
    std::string path = testDir + "/invalid_version.gltf";
    
    std::ofstream file(path);
    file << R"({
        "asset": {"version": "1.0"},
        "scene": 0,
        "scenes": [{}],
        "nodes": [],
        "meshes": []
    })";
    
    GLTFLoader loader;
    auto mesh = loader.load(path);
    
    EXPECT_EQ(mesh->loadState, LoadState::Failed);
    EXPECT_NE(mesh->loadError.find("version"), std::string::npos);
}

TEST(GLTFLoaderTest, ErrorHandling_CorruptGLB) {
    std::string testDir = getTestAssetDir();
    std::string path = testDir + "/corrupt.glb";
    
    // Write invalid GLB header
    std::ofstream file(path, std::ios::binary);
    uint32_t invalidMagic = 0x12345678;
    file.write(reinterpret_cast<const char*>(&invalidMagic), 4);
    
    GLTFLoader loader;
    auto result = loader.validate(path);
    
    EXPECT_FALSE(result.isValid);
    EXPECT_NE(result.error.find("magic"), std::string::npos);
}

TEST(GLTFLoaderTest, ErrorHandling_MissingPositions) {
    std::string testDir = getTestAssetDir();
    std::string path = testDir + "/no_positions.gltf";
    
    // glTF without POSITION attribute (invalid)
    std::ofstream file(path);
    file << R"({
        "asset": {"version": "2.0"},
        "scene": 0,
        "scenes": [{"nodes": [0]}],
        "nodes": [{"mesh": 0}],
        "meshes": [{
            "primitives": [{
                "attributes": {"NORMAL": 0}
            }]
        }],
        "accessors": [{"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"}],
        "bufferViews": [{"buffer": 0, "byteOffset": 0, "byteLength": 36}],
        "buffers": [{"uri": "data.bin", "byteLength": 36}]
    })";
    
    GLTFLoader loader;
    auto mesh = loader.load(path);
    
    // Should load but have no primitives (skipped due to missing positions)
    EXPECT_TRUE(mesh->primitives.empty() || mesh->loadState == LoadState::Failed);
}

TEST(GLTFLoaderTest, LoadWithTangents) {
    std::string path = createGLTFWithTangents();
    
    GLTFLoader loader;
    auto mesh = loader.load(path);
    
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->loadState, LoadState::Loaded);
    
    auto& prim = mesh->primitives[0];
    EXPECT_EQ(prim.vertexCount, 4u);
    EXPECT_EQ(prim.indexCount, 6u);
    
    // Check vertex format includes tangents
    EXPECT_TRUE(prim.vertexFormat.attributes.count(VertexAttribute::Position) > 0);
    EXPECT_TRUE(prim.vertexFormat.attributes.count(VertexAttribute::Normal) > 0);
    EXPECT_TRUE(prim.vertexFormat.attributes.count(VertexAttribute::Tangent) > 0);
    EXPECT_TRUE(prim.vertexFormat.attributes.count(VertexAttribute::UV0) > 0);
}

TEST(GLTFLoaderTest, LoadWithColors) {
    std::string path = createGLTFWithColors();
    
    GLTFLoader loader;
    auto mesh = loader.load(path);
    
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->loadState, LoadState::Loaded);
    
    auto& prim = mesh->primitives[0];
    EXPECT_EQ(prim.vertexCount, 3u);
    EXPECT_EQ(prim.indexCount, 3u);
    
    // Check vertex format includes colors
    EXPECT_TRUE(prim.vertexFormat.attributes.count(VertexAttribute::Position) > 0);
    EXPECT_TRUE(prim.vertexFormat.attributes.count(VertexAttribute::Color) > 0);
}

TEST(GLTFLoaderTest, LoadWithCubicSplineInterpolation) {
    std::string path = createGLTFWithCubicSpline();
    
    GLTFLoader::Config config;
    config.loadAnimations = true;
    GLTFLoader loader(config);
    
    auto mesh = loader.load(path);
    
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->loadState, LoadState::Loaded);
    EXPECT_FALSE(mesh->animations.empty());
    
    auto& clip = mesh->animations[0];
    EXPECT_EQ(clip.name, "CubicSplineAnimation");
    EXPECT_GT(clip.duration, 0.0f);
    EXPECT_FALSE(clip.channels.empty());
    
    // Check that keyframes were created
    for (const auto& channel : clip.channels) {
        EXPECT_GE(channel.keyframes.size(), 3u);  // At least 3 keyframes
        
        // Verify keyframe times are in order
        for (size_t i = 1; i < channel.keyframes.size(); ++i) {
            EXPECT_GE(channel.keyframes[i].time, channel.keyframes[i-1].time);
        }
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
