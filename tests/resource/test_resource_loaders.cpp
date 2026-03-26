#include <gtest/gtest.h>
#include <phoenix/resource/asset_loader.hpp>
#include <phoenix/resource/resource_manager.hpp>
#include <phoenix/resource/mesh.hpp>
#include <phoenix/resource/texture.hpp>
#include <phoenix/resource/point_cloud.hpp>
#include <phoenix/resource/terrain.hpp>
#include <fstream>
#include <filesystem>

using namespace phoenix::resource;

namespace {

// Create temporary test files
std::string createTempDir() {
    return std::filesystem::temp_directory_path().string() + "/phoenix_test";
}

void ensureTempDir() {
    std::filesystem::create_directories(createTempDir());
}

std::string createTestOBJ() {
    ensureTempDir();
    std::string path = createTempDir() + "/test_cube.obj";
    
    std::ofstream file(path);
    file << "# Test cube\n";
    file << "v 0.0 0.0 0.0\n";
    file << "v 1.0 0.0 0.0\n";
    file << "v 1.0 1.0 0.0\n";
    file << "v 0.0 1.0 0.0\n";
    file << "v 0.0 0.0 1.0\n";
    file << "v 1.0 0.0 1.0\n";
    file << "v 1.0 1.0 1.0\n";
    file << "v 0.0 1.0 1.0\n";
    file << "vn 0.0 0.0 -1.0\n";
    file << "vn 0.0 0.0 1.0\n";
    file << "vn 0.0 -1.0 0.0\n";
    file << "vn 0.0 1.0 0.0\n";
    file << "vn -1.0 0.0 0.0\n";
    file << "vn 1.0 0.0 0.0\n";
    file << "f 1//1 2//1 3//1\n";
    file << "f 1//1 3//1 4//1\n";
    file << "f 5//2 8//2 7//2\n";
    file << "f 5//2 7//2 6//2\n";
    file << "f 1//3 5//3 6//3\n";
    file << "f 1//3 6//3 2//3\n";
    file << "f 3//4 7//4 8//4\n";
    file << "f 3//4 8//4 4//4\n";
    file << "f 1//5 4//5 8//5\n";
    file << "f 1//5 8//5 5//5\n";
    file << "f 2//6 6//6 7//6\n";
    file << "f 2//6 7//6 3//6\n";
    
    return path;
}

std::string createTestSTL() {
    ensureTempDir();
    std::string path = createTempDir() + "/test_triangle.stl";
    
    std::ofstream file(path);
    file << "solid test\n";
    file << "  facet normal 0.0 0.0 1.0\n";
    file << "    outer loop\n";
    file << "      vertex 0.0 0.0 0.0\n";
    file << "      vertex 1.0 0.0 0.0\n";
    file << "      vertex 0.5 1.0 0.0\n";
    file << "    endloop\n";
    file << "  endfacet\n";
    file << "endsolid test\n";
    
    return path;
}

std::string createTestHeightmap() {
    ensureTempDir();
    std::string path = createTempDir() + "/test_terrain.raw";
    
    std::ofstream file(path, std::ios::binary);
    
    // Create 64x64 heightmap
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            float h = 0.5f + 0.25f * std::sin(x * 0.1f) * std::cos(y * 0.1f);
            uint16_t value = static_cast<uint16_t>(h * 65535);
            file.write(reinterpret_cast<const char*>(&value), 2);
        }
    }
    
    return path;
}

} // anonymous namespace

// ============ GLTF Loader Tests ============

TEST(GLTFLoaderTest, ValidatePath) {
    GLTFLoader loader;
    
    auto result = loader.validate("../../../etc/passwd");
    EXPECT_FALSE(result.isValid);
    EXPECT_NE(result.error.find(".."), std::string::npos);
}

TEST(GLTFLoaderTest, SupportedExtensions) {
    GLTFLoader loader;
    auto exts = loader.getSupportedExtensions();
    
    EXPECT_TRUE(exts.count(".gltf") > 0);
    EXPECT_TRUE(exts.count(".glb") > 0);
}

TEST(GLTFLoaderTest, CanLoad) {
    GLTFLoader loader;
    
    EXPECT_TRUE(loader.canLoad("model.gltf"));
    EXPECT_TRUE(loader.canLoad("model.glb"));
    EXPECT_TRUE(loader.canLoad("path/to/model.GLTf"));
    EXPECT_FALSE(loader.canLoad("model.obj"));
}

// ============ OBJ Loader Tests ============

TEST(OBJLoaderTest, ValidateValidFile) {
    OBJLoader loader;
    std::string path = createTestOBJ();
    
    auto result = loader.validate(path);
    EXPECT_TRUE(result.isValid);
}

TEST(OBJLoaderTest, ValidateInvalidPath) {
    OBJLoader loader;
    
    auto result = loader.validate("../../../invalid.obj");
    EXPECT_FALSE(result.isValid);
}

TEST(OBJLoaderTest, LoadCube) {
    OBJLoader loader;
    std::string path = createTestOBJ();
    
    auto mesh = loader.load(path);
    
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->loadState, LoadState::Loaded);
    EXPECT_EQ(mesh->assetType, AssetType::Model_OBJ);
    EXPECT_FALSE(mesh->primitives.empty());
    
    auto prim = mesh->primitives[0];
    EXPECT_GT(prim.vertexCount, 0);
    EXPECT_GT(prim.indexCount, 0);
    EXPECT_TRUE(prim.bounds.isValid());
}

// ============ STL Loader Tests ============

TEST(STLLoaderTest, ValidateAsciiSTL) {
    STLLoader loader;
    std::string path = createTestSTL();
    
    auto result = loader.validate(path);
    EXPECT_TRUE(result.isValid);
}

TEST(STLLoaderTest, LoadTriangle) {
    STLLoader loader;
    std::string path = createTestSTL();
    
    auto mesh = loader.load(path);
    
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->loadState, LoadState::Loaded);
    EXPECT_EQ(mesh->assetType, AssetType::Model_STL);
    EXPECT_EQ(mesh->primitives.size(), 1u);
    
    auto& prim = mesh->primitives[0];
    EXPECT_EQ(prim.vertexCount, 3u);  // One triangle
    EXPECT_EQ(prim.indexCount, 3u);
}

// ============ Texture Loader Tests ============

TEST(TextureLoaderTest, ValidatePNG) {
    TextureLoader loader;
    
    // Create minimal PNG (1x1 pixel)
    ensureTempDir();
    std::string path = createTempDir() + "/test.png";
    
    // PNG signature + minimal IHDR
    std::ofstream file(path, std::ios::binary);
    uint8_t pngSig[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    file.write(reinterpret_cast<char*>(pngSig), 8);
    
    auto result = loader.validate(path);
    EXPECT_TRUE(result.isValid);
}

TEST(TextureLoaderTest, CalculateMipLevels) {
    EXPECT_EQ(Texture::calculateMipLevels(256, 256), 9u);
    EXPECT_EQ(Texture::calculateMipLevels(1024, 1024), 11u);
    EXPECT_EQ(Texture::calculateMipLevels(1, 1), 1u);
}

TEST(TextureLoaderTest, GetFormatSize) {
    EXPECT_EQ(Texture::getFormatSize(TextureFormat::RGBA8), 4);
    EXPECT_EQ(Texture::getFormatSize(TextureFormat::RGB8), 3);
    EXPECT_EQ(Texture::getFormatSize(TextureFormat::R8), 1);
    EXPECT_EQ(Texture::getFormatSize(TextureFormat::RGBA32F), 16);
}

// ============ Point Cloud Tests ============

TEST(PointCloudTest, BuildOctree) {
    PointCloud cloud;
    
    // Create simple point cloud
    for (int i = 0; i < 100; ++i) {
        PointCloudPoint pt;
        pt.position.x = i * 0.1f;
        pt.position.y = i * 0.1f;
        pt.position.z = i * 0.1f;
        cloud.points.push_back(pt);
    }
    
    cloud.totalPoints = cloud.points.size();
    cloud.buildOctree(5, 10);
    
    EXPECT_FALSE(cloud.octree.empty());
    EXPECT_GT(cloud.octree.size(), 1u);
}

TEST(PointCloudTest, CalculateLOD) {
    PointCloud cloud;
    cloud.lodConfig.push_back({10.0f, 1.0f, 1000000});
    cloud.lodConfig.push_back({50.0f, 0.5f, 500000});
    cloud.lodConfig.push_back({100.0f, 0.25f, 250000});
    
    auto lod1 = cloud.calculateLOD(5.0f, 1.0f);
    EXPECT_FLOAT_EQ(lod1.pointDensity, 1.0f);
    
    auto lod2 = cloud.calculateLOD(75.0f, 1.0f);
    EXPECT_FLOAT_EQ(lod2.pointDensity, 0.5f);
}

TEST(PointCloudTest, Validate) {
    PointCloud cloud;
    
    // Empty cloud
    auto result = cloud.validate();
    EXPECT_FALSE(result.isValid);
    
    // Add valid points
    for (int i = 0; i < 10; ++i) {
        PointCloudPoint pt;
        pt.position = math::Vector3(i, i, i);
        cloud.points.push_back(pt);
    }
    cloud.totalPoints = 10;
    
    result = cloud.validate();
    EXPECT_TRUE(result.isValid);
}

// ============ Terrain Tests ============

TEST(TerrainTest, CalculateNormal) {
    Terrain terrain;
    terrain.width = 4;
    terrain.depth = 4;
    terrain.heightData = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
    
    auto normal = terrain.calculateNormal(1, 1);
    EXPECT_NEAR(normal.y, 1.0f, 0.1f);  // Should point mostly up
}

TEST(TerrainTest, GetHeight) {
    Terrain terrain;
    terrain.width = 2;
    terrain.depth = 2;
    terrain.heightData = {0.0f, 0.5f, 0.25f, 1.0f};
    terrain.verticalScale = 2.0f;
    terrain.verticalOffset = 10.0f;
    
    EXPECT_FLOAT_EQ(terrain.getHeight(0, 0), 10.0f);
    EXPECT_FLOAT_EQ(terrain.getHeight(1, 1), 12.0f);
}

TEST(TerrainTest, GenerateNormals) {
    Terrain terrain;
    terrain.width = 4;
    terrain.depth = 4;
    terrain.heightData.resize(16, 0.5f);
    
    terrain.generateNormals();
    
    EXPECT_EQ(terrain.normalData.size(), 16u);
    
    // Flat terrain should have up normals
    for (const auto& normal : terrain.normalData) {
        EXPECT_NEAR(normal.x, 0.0f, 0.01f);
        EXPECT_NEAR(normal.y, 1.0f, 0.01f);
        EXPECT_NEAR(normal.z, 0.0f, 0.01f);
    }
}

TEST(TerrainTest, LoadHeightmap) {
    TerrainLoader loader;
    std::string path = createTestHeightmap();
    
    auto terrain = loader.load(path);
    
    ASSERT_TRUE(terrain);
    EXPECT_EQ(terrain->loadState, LoadState::Loaded);
    EXPECT_EQ(terrain->width, 64u);
    EXPECT_EQ(terrain->depth, 64u);
    EXPECT_EQ(terrain->heightData.size(), 64u * 64u);
}

// ============ Resource Manager Tests ============

TEST(ResourceManagerTest, Initialize) {
    ResourceManager::Config config;
    config.numLoaderThreads = 2;
    config.maxMemoryBudget = 512 * 1024 * 1024;  // 512MB
    
    ResourceManager manager(config);
    
    EXPECT_TRUE(manager.initialize());
    manager.shutdown();
}

TEST(ResourceManagerTest, LoadMesh) {
    ResourceManager manager;
    manager.initialize();
    
    std::string path = createTestOBJ();
    auto handle = manager.loadMesh(path);
    
    EXPECT_TRUE(handle.isValid());
    
    auto* mesh = manager.getMesh(handle);
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->assetType, AssetType::Model_OBJ);
    
    manager.shutdown();
}

TEST(ResourceManagerTest, MemoryBudget) {
    ResourceManager::Config config;
    config.maxMemoryBudget = 10 * 1024 * 1024;  // 10MB
    
    ResourceManager manager(config);
    
    EXPECT_EQ(manager.getMemoryBudget(), 10 * 1024 * 1024);
    EXPECT_EQ(manager.getMemoryUsage(), 0u);
}

TEST(ResourceManagerTest, PathValidation) {
    ResourceManager manager;
    
    EXPECT_TRUE(manager.isPathAllowed("assets/model.obj"));
    EXPECT_FALSE(manager.isPathAllowed("../../../etc/passwd"));
    
    manager.addAllowedPath("/safe/path");
    EXPECT_TRUE(manager.isPathAllowed("/safe/path/model.obj"));
    EXPECT_FALSE(manager.isPathAllowed("/unsafe/path/model.obj"));
}

// ============ Mesh Tests ============

TEST(MeshTest, CalculateMemoryUsage) {
    Mesh mesh;
    
    MeshPrimitive prim;
    prim.vertexData.resize(1000);
    prim.indices.resize(500);
    prim.vertexCount = 100;
    prim.indexCount = 500;
    
    mesh.primitives.push_back(std::move(prim));
    
    size_t usage = mesh.calculateMemoryUsage();
    EXPECT_GT(usage, 0);
}

TEST(MeshTest, Validate) {
    Mesh mesh;
    
    // Empty mesh
    auto result = mesh.validate();
    EXPECT_FALSE(result.isValid);
    
    // Add valid primitive
    MeshPrimitive prim;
    prim.vertexData.resize(32);  // 1 vertex, 8 floats
    prim.vertexFormat.stride = 32;
    prim.vertexCount = 1;
    prim.materialIndex = 0;
    
    mesh.primitives.push_back(std::move(prim));
    
    result = mesh.validate();
    EXPECT_TRUE(result.isValid);
}

TEST(MeshTest, BuildLODLevels) {
    Mesh mesh;
    
    MeshPrimitive prim;
    prim.vertexCount = 10000;
    mesh.primitives.push_back(std::move(prim));
    
    mesh.buildLODLevels(100.0f, 4);
    
    EXPECT_EQ(mesh.lodLevels.size(), 4u);
    EXPECT_FLOAT_EQ(mesh.lodLevels[0].distance, 0.0f);
    EXPECT_FLOAT_EQ(mesh.lodLevels[3].distance, 100.0f);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
