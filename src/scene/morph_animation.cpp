#include "phoenix/scene/morph_animation.hpp"
#include <algorithm>
#include <cmath>

namespace phoenix {
namespace scene {

// ============================================================================
// MorphAnimationController 实现
// ============================================================================

uint32_t MorphAnimationController::addMorphTarget(MorphTarget target) {
    if (vertexCount_ == 0 && !target.positionDeltas.empty()) {
        vertexCount_ = target.positionDeltas.size();
    }
    
    morphTargets_.push_back(std::move(target));
    weights_.resize(morphTargets_.size(), 0.0f);
    
    return static_cast<uint32_t>(morphTargets_.size() - 1);
}

const MorphTarget* MorphAnimationController::getMorphTarget(uint32_t index) const noexcept {
    if (index < morphTargets_.size()) {
        return &morphTargets_[index];
    }
    return nullptr;
}

MorphTarget* MorphAnimationController::getMorphTarget(uint32_t index) noexcept {
    if (index < morphTargets_.size()) {
        return &morphTargets_[index];
    }
    return nullptr;
}

int32_t MorphAnimationController::findMorphTargetByName(const std::string& name) const {
    for (size_t i = 0; i < morphTargets_.size(); ++i) {
        if (morphTargets_[i].name == name) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

void MorphAnimationController::setWeight(uint32_t index, float weight) {
    if (index < weights_.size()) {
        weights_[index] = std::clamp(weight, 0.0f, 1.0f);
    }
}

float MorphAnimationController::weight(uint32_t index) const {
    if (index < weights_.size()) {
        return weights_[index];
    }
    return 0.0f;
}

void MorphAnimationController::setWeights(const std::vector<float>& weights) {
    weights_ = weights;
    for (float& w : weights_) {
        w = std::clamp(w, 0.0f, 1.0f);
    }
}

void MorphAnimationController::resetWeights() {
    std::fill(weights_.begin(), weights_.end(), 0.0f);
}

void MorphAnimationController::apply(std::vector<math::Vector3>& positions,
                                      std::vector<math::Vector3>& normals,
                                      std::vector<math::Vector3>* tangents) const {
    if (morphTargets_.empty() || vertexCount_ == 0) {
        return;
    }
    
    size_t vertexCount = std::min(positions.size(), vertexCount_);
    
    // 对每个顶点应用形变
    for (size_t vi = 0; vi < vertexCount; ++vi) {
        math::Vector3 posDelta(0.0f);
        math::Vector3 normDelta(0.0f);
        math::Vector3 tanDelta(0.0f);
        
        // 累加所有形变目标的贡献
        for (size_t mi = 0; mi < morphTargets_.size(); ++mi) {
            float w = weights_[mi];
            if (w <= 0.0f) continue;
            
            const auto& morph = morphTargets_[mi];
            
            if (vi < morph.positionDeltas.size()) {
                posDelta = posDelta + morph.positionDeltas[vi] * w;
            }
            if (vi < morph.normalDeltas.size()) {
                normDelta = normDelta + morph.normalDeltas[vi] * w;
            }
            if (tangents && vi < morph.tangentDeltas.size()) {
                tanDelta = tanDelta + morph.tangentDeltas[vi] * w;
            }
        }
        
        // 应用增量
        positions[vi] = positions[vi] + posDelta;
        normals[vi] = normals[vi] + normDelta;
        normals[vi] = normals[vi].normalized();  // 重新归一化
        
        if (tangents && vi < tangents->size()) {
            (*tangents)[vi] = (*tangents)[vi] + tanDelta;
            (*tangents)[vi] = (*tangents)[vi].normalized();
        }
    }
}

void MorphAnimationController::applyToVertex(math::Vector3& position, math::Vector3& normal, uint32_t vertexIndex) const {
    if (vertexIndex >= vertexCount_) {
        return;
    }
    
    math::Vector3 posDelta(0.0f);
    math::Vector3 normDelta(0.0f);
    
    for (size_t mi = 0; mi < morphTargets_.size(); ++mi) {
        float w = weights_[mi];
        if (w <= 0.0f) continue;
        
        const auto& morph = morphTargets_[mi];
        
        if (vertexIndex < morph.positionDeltas.size()) {
            posDelta = posDelta + morph.positionDeltas[vertexIndex] * w;
        }
        if (vertexIndex < morph.normalDeltas.size()) {
            normDelta = normDelta + morph.normalDeltas[vertexIndex] * w;
        }
    }
    
    position = position + posDelta;
    normal = (normal + normDelta).normalized();
}

void MorphAnimationController::computeBlendedDeltas(std::vector<math::Vector3>& outPositions,
                                                     std::vector<math::Vector3>& outNormals) const {
    outPositions.resize(vertexCount_, math::Vector3(0.0f));
    outNormals.resize(vertexCount_, math::Vector3(0.0f));
    
    for (size_t mi = 0; mi < morphTargets_.size(); ++mi) {
        float w = weights_[mi];
        if (w <= 0.0f) continue;
        
        const auto& morph = morphTargets_[mi];
        
        for (size_t vi = 0; vi < std::min(vertexCount_, morph.positionDeltas.size()); ++vi) {
            outPositions[vi] = outPositions[vi] + morph.positionDeltas[vi] * w;
        }
        for (size_t vi = 0; vi < std::min(vertexCount_, morph.normalDeltas.size()); ++vi) {
            outNormals[vi] = outNormals[vi] + morph.normalDeltas[vi] * w;
        }
    }
}

void MorphAnimationController::addExpression(const std::string& name,
                                              const std::vector<uint32_t>& morphIndices,
                                              const std::vector<float>& weights) {
    if (morphIndices.size() != weights.size()) {
        return;
    }
    
    Expression expr;
    expr.name = name;
    expr.morphIndices = morphIndices;
    expr.weights = weights;
    expr.currentWeight = 0.0f;
    expr.targetWeight = 0.0f;
    
    expressions_.push_back(std::move(expr));
}

void MorphAnimationController::playExpression(const std::string& name, float fadeDuration) {
    for (auto& expr : expressions_) {
        if (expr.name == name) {
            expr.targetWeight = 1.0f;
            if (fadeDuration > 0.0f) {
                expr.fadeSpeed = 1.0f / fadeDuration;
            } else {
                expr.currentWeight = 1.0f;
            }
            break;
        }
    }
}

void MorphAnimationController::stopExpression(const std::string& name) {
    for (auto& expr : expressions_) {
        if (expr.name == name) {
            expr.targetWeight = 0.0f;
            expr.fadeSpeed = 1.0f / 0.1f;  // 默认 0.1 秒淡出
            break;
        }
    }
}

void MorphAnimationController::stopAllExpressions() {
    for (auto& expr : expressions_) {
        expr.targetWeight = 0.0f;
        expr.fadeSpeed = 1.0f / 0.1f;
    }
}

void MorphAnimationController::updateExpressions(float deltaTime) {
    for (auto& expr : expressions_) {
        if (expr.currentWeight < expr.targetWeight) {
            expr.currentWeight = std::min(expr.targetWeight, 
                                           expr.currentWeight + expr.fadeSpeed * deltaTime);
        } else if (expr.currentWeight > expr.targetWeight) {
            expr.currentWeight = std::max(expr.targetWeight,
                                           expr.currentWeight - expr.fadeSpeed * deltaTime);
        }
        
        // 应用表情权重到形变目标
        for (size_t i = 0; i < expr.morphIndices.size(); ++i) {
            uint32_t morphIndex = expr.morphIndices[i];
            if (morphIndex < weights_.size()) {
                weights_[morphIndex] = std::max(weights_[morphIndex], expr.weights[i] * expr.currentWeight);
            }
        }
    }
}

size_t MorphAnimationController::memoryUsage() const noexcept {
    size_t total = 0;
    
    for (const auto& morph : morphTargets_) {
        total += morph.positionDeltas.size() * sizeof(math::Vector3);
        total += morph.normalDeltas.size() * sizeof(math::Vector3);
        total += morph.tangentDeltas.size() * sizeof(math::Vector3);
        total += morph.name.size();
    }
    
    total += weights_.size() * sizeof(float);
    
    return total;
}

void MorphAnimationController::setVertexCount(size_t count) {
    vertexCount_ = count;
}

} // namespace scene
} // namespace phoenix
