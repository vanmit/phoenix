# Phoenix Engine - IBL Test Scene

## 测试场景说明

此场景用于验证 PBR IBL (Image-Based Lighting) 管线的正确性。

## 场景内容

### 1. 测试物体

- **金属球体**: 高金属度 (0.9), 低粗糙度 (0.1) - 测试镜面反射
- **粗糙球体**: 低金属度 (0.0), 高粗糙度 (0.8) - 测试漫反射
- **混合材质球体**: 渐变金属度/粗糙度 - 测试完整 PBR 范围

### 2. 环境贴图

使用 HDR 环境贴图进行测试:
- `assets/environments/royal_esplanade_1k.hdr` (默认)
- `assets/environments/potsdamer_platz_1k.hdr`
- `assets/environments/kloofendal_4k.hdr`

### 3. 测试要点

#### IBL 生成验证
- [ ] 环境立方体贴图正确生成 (6 个面)
- [ ] Irradiance map 正确 (512x512x6, 低频)
- [ ] Prefiltered map 正确 (256x256x6, 多 MIP 级别)
- [ ] BRDF LUT 正确 (256x256, 2 通道)

#### 渲染验证
- [ ] 金属表面正确反射环境
- [ ] 粗糙表面正确散射环境光
- [ ] 菲涅尔效应正确 (掠射角反射增强)
- [ ] 能量守恒 (漫反射 + 镜面反射 ≤ 1)

#### 性能验证
- [ ] 60 FPS @ 1080p
- [ ] IBL 纹理内存合理
- [ ] 无着色器编译卡顿

## 运行测试

```bash
# 运行 IBL 测试场景
./phoenix-engine --scene test_ibl

# 指定环境贴图
./phoenix-engine --scene test_ibl --environment assets/environments/royal_esplanade_1k.hdr

# 性能分析模式
./phoenix-engine --scene test_ibl --profile
```

## 预期结果

### 视觉检查

1. **金属球体**
   - 清晰的环境反射
   - 高光随视角变化
   - 菲涅尔效应明显

2. **粗糙球体**
   - 模糊的环境反射
   - 均匀的漫反射光照
   - 无锐利高光

3. **混合材质**
   - 平滑过渡
   - 金属度/粗糙度正确影响外观

### 性能指标

- 帧率：≥ 60 FPS
- 绘制调用：< 100
- IBL 纹理内存：< 50 MB

## 调试命令

```glsl
// 在着色器中调试 IBL
debugShowIrradiance = true;   // 仅显示辐照度
debugShowPrefiltered = true;  // 仅显示预过滤
debugShowBRDF = true;         // 显示 BRDF LUT
```

## 已知问题

- [ ] 无

## 更新日志

- 2026-03-28: 初始 IBL 测试场景创建
