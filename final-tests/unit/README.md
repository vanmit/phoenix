# Phoenix Engine Unit Tests

**目标覆盖率**: >90%

---

## 测试文件

- `test_math_unit.cpp` - 数学库单元测试
- `test_security_unit.cpp` - 安全核心单元测试
- `test_render_unit.cpp` - 渲染系统单元测试
- `test_resource_unit.cpp` - 资源系统单元测试

---

## 构建与运行

```bash
mkdir build && cd build
cmake .. -DPHOENIX_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)

# 运行单元测试
ctest -R unit --verbose

# 生成覆盖率报告
./coverage.sh
```

---

## 覆盖率目标

| 模块 | 目标 | 当前 | 状态 |
|------|------|------|------|
| Math | 95% | - | 🔄 |
| Security | 95% | - | 🔄 |
| Render | 90% | - | 🔄 |
| Resource | 90% | - | 🔄 |
| Scene | 90% | - | 🔄 |
| Animation | 90% | - | 🔄 |
| **总计** | **>90%** | **-** | **🔄** |

---

## 测试用例清单

### Math 模块

- [ ] Vector2/Vector3/Vector4 运算
- [ ] Matrix4x4 变换
- [ ] Quaternion 旋转
- [ ] 碰撞检测
- [ ] 射线投射
- [ ] 边界盒计算

### Security 模块

- [ ] 内存安全操作
- [ ] 加密/解密
- [ ] 哈希函数
- [ ] 安全字符串处理
- [ ] 输入验证
- [ ] 权限检查

### Render 模块

- [ ] 设备初始化
- [ ] 管线创建
- [ ] 资源绑定
- [ ] 绘制调用
- [ ] 着色器编译
- [ ] 纹理管理

### Resource 模块

- [ ] 资源加载
- [ ] 资源缓存
- [ ] 资源卸载
- [ ] 异步加载
- [ ] 依赖管理
- [ ] 内存管理

---

*Phoenix Engine Unit Test Suite*
