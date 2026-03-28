# 🦅 Phoenix Engine 演示中心

Phoenix Engine 交互式演示中心，展示引擎的核心渲染能力和图形技术。

## 📁 项目结构

```
demo-website/
├── index.html              # 演示中心首页
├── demo-template.html      # 演示页面模板
├── demos.json              # 演示配置文件
├── css/
│   ├── showcase.css        # 主样式表
│   └── responsive.css      # 响应式样式
├── js/
│   ├── showcase.js         # 演示中心逻辑
│   └── player.js           # WASM 播放器
├── demos/
│   ├── pbr-materials/      # PBR 材质演示
│   ├── lighting/           # 光影效果演示
│   ├── animation/          # 动画角色演示
│   ├── particles/          # 粒子系统演示
│   └── performance/        # 性能测试演示
└── README.md               # 本文件
```

## 🎮 演示列表

### 1. PBR 材质展示 (`/demos/pbr-materials/`)
基于物理的渲染材质系统，展示：
- 金属、塑料、木材、石材等多种材质
- 金属度工作流 (Metallic-Roughness)
- 基于图像的光照 (IBL)
- 法线贴图和表面细节

**技术亮点：**
- 金属度/粗糙度实时调整
- 4 种预设材质类型
- 60 FPS 稳定运行

### 2. 光影效果 (`/demos/lighting/`)
实时阴影系统，展示：
- CSM 级联阴影映射
- VSM 方差阴影映射
- 软阴影和硬阴影切换
- 多光源支持

**技术亮点：**
- 4 级 CSM 级联
- 可调节阴影质量 (1024-4096)
- 支持 1-8 个动态光源

### 3. 动画角色 (`/demos/animation/`)
高性能骨骼动画系统，展示：
- GPU 蒙皮技术
- 动画混合树
- 表情混合形状
- 待机/行走/奔跑/跳跃动画

**技术亮点：**
- 64 骨骼支持
- 24 个动画片段
- 平滑动画混合过渡

### 4. 粒子系统 (`/demos/particles/`)
GPU 加速粒子系统，展示：
- 火焰、烟雾、魔法、火花效果
- 10 万级粒子模拟
- 子发射器和力场系统

**技术亮点：**
- GPU 粒子模拟
- 可调节粒子数量 (100-5000)
- 实时效果切换

### 5. 性能测试 (`/demos/performance/`)
万级物体渲染压力测试，展示：
- 实例化渲染
- 视锥剔除
- 遮挡剔除
- LOD 系统

**技术亮点：**
- 支持 10,000 物体
- 实时性能监控
- 基准测试工具

## 🚀 快速开始

### 本地开发

```bash
# 克隆项目
cd /home/admin/.openclaw/workspace/phoenix-engine/demo-website

# 使用 Python 启动本地服务器
python3 -m http.server 8080

# 或使用 Node.js
npx serve .
```

然后在浏览器中访问 `http://localhost:8080`

### 生产部署

1. 将所有文件上传到服务器
2. 配置 Web 服务器 (Nginx/Apache)
3. 启用 Gzip 压缩
4. 配置缓存策略

## 📊 技术规格

### 浏览器支持
- Chrome 80+
- Firefox 75+
- Safari 13+
- Edge 80+

### WebGL 要求
- WebGL 2.0 (推荐)
- WebGL 1.0 (兼容模式)

### 性能目标
- 桌面端：60 FPS
- 移动端：30+ FPS
- 加载时间：< 3 秒

## 🎨 自定义

### 添加新演示

1. 在 `demos/` 目录创建新文件夹
2. 创建 `index.html` 演示页面
3. 在 `demos.json` 中添加配置：

```json
{
  "id": "your-demo",
  "title": "演示名称",
  "description": "演示描述",
  "category": "category-id",
  "thumbnail": "demos/your-demo/thumb.jpg",
  "demoUrl": "demos/your-demo/index.html",
  "featured": true,
  "popularity": 85,
  "tags": ["标签 1", "标签 2"],
  "techSpecs": {
    "key": "value"
  },
  "features": ["特性 1", "特性 2"]
}
```

### 样式定制

编辑 `css/showcase.css` 中的 CSS 变量：

```css
:root {
  --primary-color: #ff6b35;    /* 主色调 */
  --secondary-color: #f7c59f;  /* 辅助色 */
  --accent-color: #2ec4b6;     /* 强调色 */
  --dark-bg: #1a1a2e;          /* 深色背景 */
  --card-bg: #16213e;          /* 卡片背景 */
}
```

## 📈 访问统计

演示中心包含可选的访问统计功能：

- 本地存储记录访问次数
- 热门演示自动排序
- 实时 FPS 监控

## 🔧 配置选项

### demos.json 配置

| 字段 | 类型 | 说明 |
|------|------|------|
| id | string | 演示唯一标识 |
| title | string | 演示标题 |
| description | string | 演示描述 |
| category | string | 分类 ID |
| thumbnail | string | 缩略图路径 |
| demoUrl | string | 演示页面 URL |
| featured | boolean | 是否推荐 |
| popularity | number | 热度值 |
| tags | array | 标签列表 |
| techSpecs | object | 技术规格 |
| features | array | 特性列表 |

## 📝 更新日志

### v1.0.0 (2026-03-26)
- ✨ 初始版本发布
- 🎨 5 个核心演示
- 📱 完整响应式设计
- 🔍 搜索和筛选功能
- 📊 性能监控系统

## 🤝 贡献

欢迎贡献代码和建议！

## 📄 许可证

© 2026 Phoenix Engine Team. All rights reserved.

## 📞 联系方式

- 官网：https://phoenix-engine.com
- GitHub: https://github.com/phoenix-engine
- 邮箱：contact@phoenix-engine.com

---

**🦅 Phoenix Engine - 点燃你的图形世界**
