#include <gtest/gtest.h>
#include "phoenix/scene/animation_types.hpp"
#include "phoenix/scene/skeleton.hpp"
#include "phoenix/scene/animator.hpp"
#include "phoenix/scene/morph_animation.hpp"
#include "phoenix/math/vector3.hpp"
#include "phoenix/math/quaternion.hpp"
#include <memory>
#include <cmath>

using namespace phoenix;
using namespace phoenix::scene;
using namespace phoenix::math;

// ============================================================================
// 动画类型测试
// ============================================================================

TEST(AnimationTypesTest, KeyframeConstruction) {
    Vector3 pos(1.0f, 2.0f, 3.0f);
    Quaternion rot = Quaternion::fromAxisAngle(Vector3::unitY(), 1.57f);
    Vector3 scale(2.0f, 2.0f, 2.0f);
    
    Keyframe kf(0.5f, pos, rot, scale);
    
    EXPECT_FLOAT_EQ(kf.time, 0.5f);
    EXPECT_FLOAT_EQ(kf.position.x, 1.0f);
    EXPECT_FLOAT_EQ(kf.position.y, 2.0f);
    EXPECT_FLOAT_EQ(kf.position.z, 3.0f);
    EXPECT_FLOAT_EQ(kf.scale.x, 2.0f);
}

TEST(AnimationTypesTest, BoneConstruction) {
    Bone bone;
    bone.id = 5;
    bone.parentId = 2;
    bone.name = "Spine";
    
    EXPECT_EQ(bone.id, 5u);
    EXPECT_EQ(bone.parentId, 2u);
    EXPECT_EQ(bone.name, "Spine");
    EXPECT_EQ(bone.parentId == UINT32_MAX, false);
}

TEST(AnimationTypesTest, BoneWeightInitialization) {
    BoneWeight weight;
    
    EXPECT_EQ(weight.boneIndex[0], 0);
    EXPECT_EQ(weight.boneIndex[1], 0);
    EXPECT_EQ(weight.boneIndex[2], 0);
    EXPECT_EQ(weight.boneIndex[3], 0);
    EXPECT_FLOAT_EQ(weight.weight[0], 0.0f);
}

TEST(AnimationTypesTest, MorphTargetConstruction) {
    MorphTarget morph("Smile");
    
    EXPECT_EQ(morph.name, "Smile");
    EXPECT_FLOAT_EQ(morph.weight, 0.0f);
    EXPECT_TRUE(morph.positionDeltas.empty());
}

TEST(AnimationTypesTest, AnimationClipCreation) {
    AnimationClip clip("Walk");
    clip.duration = 2.0f;
    clip.fps = 30.0f;
    clip.loopMode = LoopMode::Loop;
    
    EXPECT_EQ(clip.name, "Walk");
    EXPECT_FLOAT_EQ(clip.duration, 2.0f);
    EXPECT_FLOAT_EQ(clip.fps, 30.0f);
    EXPECT_EQ(clip.loopMode, LoopMode::Loop);
}

TEST(AnimationTypesTest, AnimationStateDefaults) {
    AnimationState state;
    
    EXPECT_EQ(state.clipIndex, UINT32_MAX);
    EXPECT_FLOAT_EQ(state.time, 0.0f);
    EXPECT_FLOAT_EQ(state.weight, 1.0f);
    EXPECT_FLOAT_EQ(state.speed, 1.0f);
    EXPECT_FALSE(state.playing);
}

// ============================================================================
// 骨骼系统测试
// ============================================================================

TEST(SkeletonTest, AddBone) {
    Skeleton skeleton;
    
    uint32_t rootId = skeleton.addBone("Root");
    uint32_t childId = skeleton.addBone("Child", rootId);
    
    EXPECT_EQ(rootId, 0u);
    EXPECT_EQ(childId, 1u);
    EXPECT_EQ(skeleton.boneCount(), 2u);
    
    const Bone* root = skeleton.getBone(rootId);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->name, "Root");
    EXPECT_EQ(root->parentId, UINT32_MAX);
    
    const Bone* child = skeleton.getBone(childId);
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(child->name, "Child");
    EXPECT_EQ(child->parentId, rootId);
}

TEST(SkeletonTest, FindBoneByName) {
    Skeleton skeleton;
    skeleton.addBone("Hip");
    skeleton.addBone("Spine");
    skeleton.addBone("Head");
    
    EXPECT_EQ(skeleton.findBoneByName("Hip"), 0);
    EXPECT_EQ(skeleton.findBoneByName("Spine"), 1);
    EXPECT_EQ(skeleton.findBoneByName("Head"), 2);
    EXPECT_EQ(skeleton.findBoneByName("NonExistent"), -1);
}

TEST(SkeletonTest, BoneHierarchy) {
    Skeleton skeleton;
    
    uint32_t root = skeleton.addBone("Root");
    uint32_t spine = skeleton.addBone("Spine", root);
    uint32_t head = skeleton.addBone("Head", spine);
    
    const auto& rootChildren = skeleton.getChildren(root);
    EXPECT_EQ(rootChildren.size(), 1u);
    EXPECT_EQ(rootChildren[0], spine);
    
    const auto& spineChildren = skeleton.getChildren(spine);
    EXPECT_EQ(spineChildren.size(), 1u);
    EXPECT_EQ(spineChildren[0], head);
    
    const auto& rootBones = skeleton.rootBones();
    EXPECT_EQ(rootBones.size(), 1u);
    EXPECT_EQ(rootBones[0], root);
}

TEST(SkeletonTest, UpdateBonePose) {
    Skeleton skeleton;
    skeleton.addBone("Test");
    
    Vector3 pos(1.0f, 2.0f, 3.0f);
    Quaternion rot = Quaternion::fromAxisAngle(Vector3::unitY(), 1.57f);
    Vector3 scale(2.0f, 2.0f, 2.0f);
    
    skeleton.updateBonePose(0, pos, rot, scale);
    
    const Bone* bone = skeleton.getBone(0);
    ASSERT_NE(bone, nullptr);
    EXPECT_FLOAT_EQ(bone->localPosition.x, 1.0f);
    EXPECT_FLOAT_EQ(bone->localPosition.y, 2.0f);
    EXPECT_FLOAT_EQ(bone->localPosition.z, 3.0f);
}

TEST(SkeletonTest, CalculateFinalMatrices) {
    Skeleton skeleton;
    
    uint32_t root = skeleton.addBone("Root");
    uint32_t child = skeleton.addBone("Child", root);
    
    skeleton.updateBonePose(root, Vector3(0, 0, 0), Quaternion::identity(), Vector3(1));
    skeleton.updateBonePose(child, Vector3(1, 0, 0), Quaternion::identity(), Vector3(1));
    
    skeleton.calculateFinalMatrices();
    
    const Matrix4& rootMatrix = skeleton.getFinalMatrix(root);
    const Matrix4& childMatrix = skeleton.getFinalMatrix(child);
    
    // 根骨骼应该是单位矩阵（无变换）
    EXPECT_FLOAT_EQ(rootMatrix.data[3], 0.0f);
    EXPECT_FLOAT_EQ(rootMatrix.data[7], 0.0f);
    EXPECT_FLOAT_EQ(rootMatrix.data[11], 0.0f);
    
    // 子骨骼应该有平移
    EXPECT_FLOAT_EQ(childMatrix.data[3], 1.0f);
}

TEST(SkeletonTest, ResetToBindPose) {
    Skeleton skeleton;
    skeleton.addBone("Test");
    
    // 修改姿态
    skeleton.updateBonePose(0, Vector3(5, 5, 5), Quaternion::identity(), Vector3(2));
    skeleton.calculateFinalMatrices();
    
    // 重置
    skeleton.resetToBindPose();
    
    const Bone* bone = skeleton.getBone(0);
    // 应该恢复到绑定姿态
    EXPECT_FLOAT_EQ(bone->localScale.x, 1.0f);
}

TEST(SkeletonTest, MemoryUsage) {
    Skeleton skeleton;
    for (int i = 0; i < 10; ++i) {
        skeleton.addBone("Bone" + std::to_string(i));
    }
    
    size_t usage = skeleton.memoryUsage();
    EXPECT_GT(usage, 0);
}

// ============================================================================
// 动画师测试
// ============================================================================

TEST(AnimatorTest, CreateAndDestroy) {
    Animator animator;
    // 简单的创建和销毁测试
}

TEST(AnimatorTest, SetSkeleton) {
    Animator animator;
    auto skeleton = std::make_shared<Skeleton>();
    skeleton->addBone("Root");
    
    animator.setSkeleton(skeleton);
    
    EXPECT_NE(animator.skeleton(), nullptr);
    EXPECT_EQ(animator.skeleton()->boneCount(), 1u);
}

TEST(AnimatorTest, AddClip) {
    Animator animator;
    
    auto clip = std::make_shared<AnimationClip>("TestClip");
    clip->duration = 1.0f;
    
    uint32_t index = animator.addClip(clip);
    
    EXPECT_EQ(index, 0u);
    EXPECT_EQ(animator.clipCount(), 1u);
}

TEST(AnimatorTest, PlayAnimation) {
    Animator animator;
    
    auto skeleton = std::make_shared<Skeleton>();
    skeleton->addBone("Root");
    animator.setSkeleton(skeleton);
    
    auto clip = std::make_shared<AnimationClip>("Walk");
    clip->duration = 2.0f;
    animator.addClip(clip);
    
    animator.play(0);
    
    EXPECT_TRUE(animator.isPlaying());
    EXPECT_EQ(animator.currentClipIndex(), 0u);
    EXPECT_FLOAT_EQ(animator.currentTime(), 0.0f);
}

TEST(AnimatorTest, PlayByName) {
    Animator animator;
    
    auto clip1 = std::make_shared<AnimationClip>("Walk");
    clip1->duration = 2.0f;
    animator.addClip(clip1);
    
    auto clip2 = std::make_shared<AnimationClip>("Run");
    clip2->duration = 1.0f;
    animator.addClip(clip2);
    
    animator.playByName("Run");
    
    EXPECT_EQ(animator.currentClipIndex(), 1u);
}

TEST(AnimatorTest, StopAnimation) {
    Animator animator;
    
    auto clip = std::make_shared<AnimationClip>("Test");
    clip->duration = 1.0f;
    animator.addClip(clip);
    
    animator.play(0);
    EXPECT_TRUE(animator.isPlaying());
    
    animator.stop();
    EXPECT_FALSE(animator.isPlaying());
}

TEST(AnimatorTest, SetSpeed) {
    Animator animator;
    
    auto clip = std::make_shared<AnimationClip>("Test");
    clip->duration = 1.0f;
    animator.addClip(clip);
    
    animator.play(0);
    animator.setSpeed(2.0f);
    
    EXPECT_FLOAT_EQ(animator.speed(), 2.0f);
}

TEST(AnimatorTest, UpdateAnimation) {
    Animator animator;
    
    auto skeleton = std::make_shared<Skeleton>();
    skeleton->addBone("Root");
    animator.setSkeleton(skeleton);
    
    auto clip = std::make_shared<AnimationClip>("Test");
    clip->duration = 1.0f;
    animator.addClip(clip);
    
    animator.play(0);
    
    // 更新 0.5 秒
    animator.update(0.5f);
    
    EXPECT_FLOAT_EQ(animator.currentTime(), 0.5f);
}

TEST(AnimatorTest, AnimationLooping) {
    Animator animator;
    
    auto clip = std::make_shared<AnimationClip>("Loop");
    clip->duration = 1.0f;
    animator.addClip(clip);
    
    animator.play(0, 0.0f, LoopMode::Loop);
    
    // 更新 1.5 秒，应该循环
    animator.update(1.5f);
    
    EXPECT_FLOAT_EQ(animator.currentTime(), 0.5f);  // 应该循环到 0.5
}

TEST(AnimatorTest, MorphWeight) {
    Animator animator;
    
    animator.setMorphWeight(0, 0.5f);
    animator.setMorphWeight(1, 0.8f);
    
    EXPECT_FLOAT_EQ(animator.morphWeight(0), 0.5f);
    EXPECT_FLOAT_EQ(animator.morphWeight(1), 0.8f);
    EXPECT_FLOAT_EQ(animator.morphWeight(2), 0.0f);  // 不存在的索引
}

// ============================================================================
// 形变动画测试
// ============================================================================

TEST(MorphAnimationTest, AddMorphTarget) {
    MorphAnimationController controller;
    controller.setVertexCount(3);
    
    MorphTarget target("Smile");
    target.positionDeltas = {
        Vector3(0.1f, 0.0f, 0.0f),
        Vector3(0.0f, 0.1f, 0.0f),
        Vector3(-0.1f, 0.0f, 0.0f)
    };
    
    uint32_t index = controller.addMorphTarget(std::move(target));
    
    EXPECT_EQ(index, 0u);
    EXPECT_EQ(controller.morphTargetCount(), 1u);
}

TEST(MorphAnimationTest, FindMorphTargetByName) {
    MorphAnimationController controller;
    
    MorphTarget t1("Smile");
    MorphTarget t2("Frown");
    
    controller.addMorphTarget(std::move(t1));
    controller.addMorphTarget(std::move(t2));
    
    EXPECT_EQ(controller.findMorphTargetByName("Smile"), 0);
    EXPECT_EQ(controller.findMorphTargetByName("Frown"), 1);
    EXPECT_EQ(controller.findMorphTargetByName("Angry"), -1);
}

TEST(MorphAnimationTest, SetWeight) {
    MorphAnimationController controller;
    
    MorphTarget target("Test");
    controller.addMorphTarget(std::move(target));
    
    controller.setWeight(0, 0.5f);
    EXPECT_FLOAT_EQ(controller.weight(0), 0.5f);
    
    // 测试钳制
    controller.setWeight(0, 1.5f);
    EXPECT_FLOAT_EQ(controller.weight(0), 1.0f);
    
    controller.setWeight(0, -0.5f);
    EXPECT_FLOAT_EQ(controller.weight(0), 0.0f);
}

TEST(MorphAnimationTest, ApplyMorph) {
    MorphAnimationController controller;
    controller.setVertexCount(2);
    
    MorphTarget target("Test");
    target.positionDeltas = {
        Vector3(1.0f, 0.0f, 0.0f),
        Vector3(0.0f, 1.0f, 0.0f)
    };
    controller.addMorphTarget(std::move(target));
    
    std::vector<Vector3> positions = {
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3(0.0f, 0.0f, 0.0f)
    };
    std::vector<Vector3> normals = {
        Vector3(0.0f, 1.0f, 0.0f),
        Vector3(0.0f, 1.0f, 0.0f)
    };
    
    controller.setWeight(0, 1.0f);
    controller.apply(positions, normals);
    
    EXPECT_FLOAT_EQ(positions[0].x, 1.0f);
    EXPECT_FLOAT_EQ(positions[1].y, 1.0f);
}

TEST(MorphAnimationTest, ResetWeights) {
    MorphAnimationController controller;
    
    controller.addMorphTarget(MorphTarget("Test1"));
    controller.addMorphTarget(MorphTarget("Test2"));
    
    controller.setWeight(0, 0.5f);
    controller.setWeight(1, 0.8f);
    
    controller.resetWeights();
    
    EXPECT_FLOAT_EQ(controller.weight(0), 0.0f);
    EXPECT_FLOAT_EQ(controller.weight(1), 0.0f);
}

TEST(MorphAnimationTest, Expression) {
    MorphAnimationController controller;
    
    MorphTarget t1("MouthSmile");
    MorphTarget t2("EyeSquint");
    controller.addMorphTarget(std::move(t1));
    controller.addMorphTarget(std::move(t2));
    
    controller.addExpression("Happy", {0, 1}, {1.0f, 0.5f});
    controller.playExpression("Happy");
    
    // 表情应该增加权重
    EXPECT_GE(controller.weight(0), 0.0f);
}

TEST(MorphAnimationTest, MemoryUsage) {
    MorphAnimationController controller;
    controller.setVertexCount(100);
    
    for (int i = 0; i < 10; ++i) {
        MorphTarget target("Target" + std::to_string(i));
        target.positionDeltas.resize(100, Vector3(0.1f));
        controller.addMorphTarget(std::move(target));
    }
    
    size_t usage = controller.memoryUsage();
    EXPECT_GT(usage, 0);
}

// ============================================================================
// 性能测试
// ============================================================================

TEST(AnimationPerformanceTest, SkeletonWith100Bones) {
    Skeleton skeleton;
    
    // 创建 100 根骨骼的层级
    uint32_t parent = skeleton.addBone("Root");
    for (int i = 1; i < 100; ++i) {
        uint32_t child = skeleton.addBone("Bone" + std::to_string(i), parent);
        parent = child;
    }
    
    // 更新所有骨骼
    for (size_t i = 0; i < skeleton.boneCount(); ++i) {
        skeleton.updateBonePose(static_cast<uint32_t>(i), 
                                Vector3(0.01f * i, 0, 0),
                                Quaternion::identity(),
                                Vector3(1));
    }
    
    // 计算最终矩阵
    skeleton.calculateFinalMatrices();
    
    EXPECT_EQ(skeleton.boneCount(), 100u);
}

TEST(AnimationPerformanceTest, MultipleAnimationStates) {
    Animator animator;
    
    auto skeleton = std::make_shared<Skeleton>();
    for (int i = 0; i < 50; ++i) {
        skeleton->addBone("Bone" + std::to_string(i));
    }
    animator.setSkeleton(skeleton);
    
    // 添加多个动画剪辑
    for (int i = 0; i < 10; ++i) {
        auto clip = std::make_shared<AnimationClip>("Anim" + std::to_string(i));
        clip->duration = 1.0f;
        animator.addClip(clip);
    }
    
    // 添加多个层
    for (int i = 0; i < 5; ++i) {
        animator.addLayer(i, 0.5f);
    }
    
    // 更新
    animator.update(0.016f);
    
    EXPECT_EQ(animator.clipCount(), 10u);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
