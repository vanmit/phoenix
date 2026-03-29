# 任务 1.3 准备 - FFI 边界检查完善

**准备时间**: 2026-03-29 16:05  
**状态**: ⏳ 等待任务 1.2 审查通过  
**审查 Agent**: Security Audit Agent + WASM Review Agent  

---

## 📋 任务内容

### 修复文件
- `wasm/src/wasm_bindings.cpp` - 所有 FFI 函数
- `wasm/src/wasm_webgpu.cpp` - WebGPU 相关 FFI

### 修复内容
1. **纹理创建函数边界检查**
   - 验证 width/height 上限 (16384)
   - 检查总尺寸溢出
2. **资源加载函数 URI 验证**
   - 协议白名单
   - 路径遍历检查
   - 长度限制
3. **视口设置边界检查**
   - 验证不超出渲染目标范围
   - 深度范围验证

### 代码变更示例
```cpp
// 添加纹理尺寸上限
constexpr uint32_t MAX_TEXTURE_SIZE = 16384;
if (width > MAX_TEXTURE_SIZE || height > MAX_TEXTURE_SIZE) {
    return TextureHandle{0};
}

// 添加溢出检查
if (width > 0 && height > MAX_TEXTURE_SIZE / width / 4) {
    return TextureHandle{0};  // 防止溢出
}
```

---

## 🎯 验收标准

### 代码质量
- [ ] 所有 FFI 函数包含边界检查
- [ ] 错误处理一致
- [ ] 无编译器警告

### 功能验证
- [ ] 超大纹理拒绝创建
- [ ] URI 验证有效
- [ ] 模糊测试 10,000+ 次无崩溃

### 测试覆盖
- [ ] 边界值测试
- [ ] 模糊测试
- [ ] 错误处理测试

---

## 🔍 审查要点

### Security Audit Agent
1. 边界检查完整性
2. 错误处理一致性
3. 模糊测试覆盖

### WASM Review Agent
1. WASM 内存边界验证
2. 性能开销评估
3. 浏览器兼容性

---

## ⏱️ 执行计划

### 等待条件
- ⏳ 任务 1.2 审查通过

### 触发条件
- ✅ 任务 1.2 审查报告生成
- ✅ 审查结论为"通过"

### 执行步骤
1. 修改 wasm_bindings.cpp
2. 修改 wasm_webgpu.cpp
3. 编译验证
4. 安排双 Agent 审查
5. 审查通过后进入任务 1.4

---

*准备完成，等待任务 1.2 审查通过*
