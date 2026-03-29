#!/bin/bash
# Phoenix Engine 目录整理脚本
# 清理临时文件、合并重复文档、结构化整理

set -e

ENGINE_ROOT="/home/admin/.openclaw/workspace/phoenix-engine"
cd "$ENGINE_ROOT"

echo "=== Phoenix Engine 目录整理 ==="
echo ""

# 1. 清理构建产物
echo "📦 清理构建产物..."
rm -rf build/ build_test/ build_wasm/ 2>/dev/null || true
echo "   ✅ 清理完成"

# 2. 清理临时测试文件
echo "🧹 清理临时测试文件..."
rm -rf final-tests/ 2>/dev/null || true
echo "   ✅ 清理完成"

# 3. 清理重复的 demo 文件
echo "🎨 整理 demo 文件..."
mkdir -p demo-website/archive
mv -f demo-*.html demo-website/archive/ 2>/dev/null || true
mv -f deploy-demo.sh demo-website/ 2>/dev/null || true
mv -f nginx-demo.conf demo-website/ 2>/dev/null || true
mv -f vercel.json demo-website/ 2>/dev/null || true
echo "   ✅ 整理完成"

# 4. 整理根目录文档
echo "📄 整理根目录文档..."

# 创建 docs/archive 存放历史文档
mkdir -p docs/archive/phase-reports
mkdir -p docs/archive/iterations

# 移动阶段报告
mv -f PHASE*.md docs/archive/phase-reports/ 2>/dev/null || true

# 移动迭代报告
mv -f ITERATION*.md docs/archive/iterations/ 2>/dev/null || true

# 移动其他历史文档
mv -f IMPLEMENTATION_SUMMARY.md docs/archive/ 2>/dev/null || true
mv -f PROJECT-LAUNCH.md docs/archive/ 2>/dev/null || true
mv -f TEAM-ORGANIZATION.md docs/archive/ 2>/dev/null || true
mv -f CODE_REVIEW_REPORT.md docs/archive/ 2>/dev/null || true

# 移动修复相关文档到 reviews 目录
mv -f fix-execution-plan.md reviews/ 2>/dev/null || true
mv -f task-*.md reviews/ 2>/dev/null || true
mv -f phase1-summary.md reviews/ 2>/dev/null || true

echo "   ✅ 整理完成"

# 5. 整理 reviews 目录
echo "📋 整理 reviews 目录..."
mkdir -p reviews/2026-03-29-phase1
mv -f reviews/*-2026-03-29.md reviews/2026-03-29-phase1/ 2>/dev/null || true
mv -f reviews/task-*.md reviews/2026-03-29-phase1/ 2>/dev/null || true
mv -f reviews/phase1-summary.md reviews/2026-03-29-phase1/ 2>/dev/null || true
echo "   ✅ 整理完成"

# 6. 清理 security-audit 重复内容
echo "🔒 整理安全审计文档..."
if [ -d "security-final" ]; then
    mkdir -p docs/archive/security-audit-historical
    mv -f security-final/* docs/archive/security-audit-historical/ 2>/dev/null || true
    rm -rf security-final/ 2>/dev/null || true
fi
echo "   ✅ 整理完成"

# 7. 创建统一的 NEXT_TASKS.md
echo "📝 创建下一阶段任务清单..."
cat > NEXT_TASKS.md << 'EOF'
# Phoenix Engine 下一阶段任务清单

**更新日期**: 2026-03-29  
**当前阶段**: Phase 1 完成 ✅  
**下一阶段**: Phase 2 - 内存管理优化 (Week 3-4)

---

## 📊 Phase 1 完成情况

| 任务 | 状态 | 提交 |
|------|------|------|
| 1.1: 应用安全修复补丁 | ✅ 完成 | 2ad634f |
| 1.2: Handle 代数检查 | ✅ 完成 | c64530a |
| 1.3: FFI 边界检查 | ✅ 完成 | 842ec5a |
| 1.4: ASAN/UBSAN 集成 | ✅ 完成 | f534c03 |

**审查状态**: 全部通过 ✅

---

## 🎯 Phase 2: 内存管理优化 (Week 3-4)

### 任务 2.1: 实现 MemoryPool 类
- **优先级**: P0
- **预计工时**: 8 小时
- **文件**: `include/phoenix/core/MemoryPool.hpp`, `src/core/MemoryPool.cpp`
- **验收标准**:
  - [ ] 支持多种块大小 (64B, 256B, 1KB, 4KB)
  - [ ] 分配/释放时间复杂度 O(1)
  - [ ] 线程安全
  - [ ] 内存碎片率 <5%

### 任务 2.2: 实现 FrameAllocator 类
- **优先级**: P0
- **预计工时**: 6 小时
- **文件**: `include/phoenix/core/FrameAllocator.hpp`, `src/core/FrameAllocator.cpp`
- **验收标准**:
  - [ ] beginFrame()/endFrame() 接口
  - [ ] 分配速度比 malloc 快 10x+
  - [ ] 帧结束自动重置
  - [ ] 无内存泄漏

### 任务 2.3: 集成到渲染系统
- **优先级**: P0
- **预计工时**: 8 小时
- **文件**: `src/render/RenderDevice.cpp`, `include/phoenix/render/Types.hpp`
- **验收标准**:
  - [ ] 渲染管线使用帧分配器
  - [ ] 资源管理使用内存池
  - [ ] 性能提升可量化
  - [ ] 无回归问题

### 任务 2.4: 性能基准测试
- **优先级**: P1
- **预计工时**: 4 小时
- **文件**: `benchmarks/memory/benchmark_memory.cpp`
- **验收标准**:
  - [ ] 分配速度对比测试 (vs std::malloc)
  - [ ] 内存碎片率测试
  - [ ] 多线程压力测试
  - [ ] 基准测试文档

---

## 📅 时间安排

| 周次 | 任务 | 负责人 |
|------|------|--------|
| Week 3 | 任务 2.1 + 2.2 | 核心架构组 |
| Week 4 | 任务 2.3 + 2.4 + 阶段验收 | 核心架构组 + 性能组 |

---

## 📈 预期指标

| 指标 | 当前 | 目标 | 提升 |
|------|------|------|------|
| 内存分配效率 | 1x | 5x | +400% |
| 内存碎片率 | ~15% | <5% | -67% |
| 渲染性能 | 1x | 1.15x | +15% |

---

## 📝 审查安排

每个任务完成后自动安排审查：
- 任务 2.1 → Architecture Agent
- 任务 2.2 → Architecture Agent + Security Audit Agent
- 任务 2.3 → Architecture Agent
- 任务 2.4 → Architecture Agent + Performance Agent

**阶段验收**: Architecture Agent + Security Audit Agent

---

*最后更新：2026-03-29*
EOF

echo "   ✅ 任务清单创建完成"

# 8. 更新 FIX_PROGRESS.md
echo "📊 更新进度文档..."
cat > FIX_PROGRESS.md << 'EOF'
# Phoenix Engine 修复执行进度

**计划制定**: 2026-03-29  
**当前阶段**: Phase 1 ✅ 完成  
**最后更新**: 2026-03-29 16:15  

---

## 📊 总体进度

| 阶段 | 状态 | 进度 | 开始时间 | 完成时间 |
|------|------|------|----------|----------|
| Phase 1: 安全修复 | ✅ 完成 | 100% | 2026-03-29 | 2026-03-29 |
| Phase 2: 内存优化 | ⏳ 待开始 | 0% | - | - |
| Phase 3: SIMD 优化 | ⏳ 待开始 | 0% | - | - |
| Phase 4: WASM 加固 | ⏳ 待开始 | 0% | - | - |
| Phase 5: 渲染优化 | ⏳ 待开始 | 0% | - | - |
| Phase 6: 安全合规 | ⏳ 待开始 | 0% | - | - |
| Phase 7: 工程化 | ⏳ 待开始 | 0% | - | - |

---

## ✅ Phase 1: 安全修复 (Week 1-2)

### 任务 1.1: 应用安全修复补丁
- **状态**: ✅ 完成，审查通过
- **提交**: `2ad634f`
- **审查**: ✅ 通过 (`reviews/2026-03-29-phase1/task-1.1-review.md`)

### 任务 1.2: Handle 代数检查
- **状态**: ✅ 完成，审查通过
- **提交**: `c64530a`
- **审查**: ✅ 通过 (`reviews/2026-03-29-phase1/task-1.2-review.md`)

### 任务 1.3: FFI 边界检查
- **状态**: ✅ 完成，审查通过
- **提交**: `842ec5a`
- **审查**: ✅ 通过 (`reviews/2026-03-29-phase1/task-1.3-review.md`)

### 任务 1.4: ASAN/UBSAN 集成
- **状态**: ✅ 完成，审查通过
- **提交**: `f534c03`
- **审查**: ✅ 通过 (`reviews/2026-03-29-phase1/task-1.4-review.md`)

### 阶段验收
- **状态**: ✅ 通过
- **审查报告**: `reviews/2026-03-29-phase1/phase1-summary.md`

---

## 📝 提交历史

```
commit f534c03 - feat: Add ASAN/UBSAN sanitizer support (task 1.4)
commit 842ec5a - security: Add texture size boundary checks (task 1.3)
commit c64530a - feat: Add generation counter to Handle template (task 1.2)
commit 2ad634f - security: Apply P0 security fixes (task 1.1)
```

---

## 🎯 下一步

**Phase 2: 内存管理优化** (详见 `NEXT_TASKS.md`)

---

*最后更新：2026-03-29 16:15*
EOF

echo "   ✅ 进度文档更新完成"

# 9. 创建 .gitignore 补充规则
echo "🔒 更新 .gitignore..."
cat >> .gitignore << 'EOF'

# Build directories
build/
build_test/
build_wasm/
cmake-build-*/

# Temporary files
*.tmp
*.temp
*.log

# IDE
.vscode/
.idea/
*.swp
*.swo
*~

# OS
.DS_Store
Thumbs.db

# Test artifacts
*.gcda
*.gcno
EOF

echo "   ✅ .gitignore 更新完成"

# 10. 最终清理
echo "🧹 最终清理..."
find . -name "*.bak" -delete 2>/dev/null || true
find . -name "*.orig" -delete 2>/dev/null || true
echo "   ✅ 清理完成"

echo ""
echo "=== 整理完成 ==="
echo ""
echo "📦 清理内容:"
echo "   - 构建产物 (build/, build_test/, build_wasm/)"
echo "   - 临时测试文件 (final-tests/)"
echo "   - 重复 demo 文件 (已归档到 demo-website/archive/)"
echo ""
echo "📄 文档整理:"
echo "   - 历史阶段报告 → docs/archive/phase-reports/"
echo "   - 历史迭代报告 → docs/archive/iterations/"
echo "   - 审查报告 → reviews/2026-03-29-phase1/"
echo "   - 安全审计历史 → docs/archive/security-audit-historical/"
echo ""
echo "📝 新建文件:"
echo "   - NEXT_TASKS.md (下一阶段任务清单)"
echo "   - FIX_PROGRESS.md (修复进度跟踪)"
echo ""
echo "✅ 源码目录已结构化整理完成！"
