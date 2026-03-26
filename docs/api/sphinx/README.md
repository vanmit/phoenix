# Sphinx 文档站点

Phoenix Engine 使用 Sphinx 构建可搜索的文档站点。

## 📋 快速开始

### 安装依赖

```bash
pip install sphinx sphinx-rtd-theme sphinx-autodoc sphinx-intl
```

### 构建文档

```bash
cd docs/api/sphinx
make html
# 输出在 _build/html/
```

### 本地预览

```bash
cd _build/html
python -m http.server 8000
# 访问 http://localhost:8000
```

## 📁 目录结构

```
docs/api/sphinx/
├── conf.py              # Sphinx 配置
├── index.rst            # 主文档入口
├── api/                 # API 参考
│   ├── platform.rst
│   ├── render.rst
│   ├── scene.rst
│   ├── resource.rst
│   └── math.rst
├── guides/              # 使用指南
│   ├── quickstart.rst
│   ├── installation.rst
│   └── building.rst
└── _static/             # 静态资源
```

## 🔍 搜索功能

Sphinx 提供强大的搜索功能：
- 全文搜索
- 跨文档搜索
- 代码搜索
- API 符号搜索

## 🎨 主题

使用 ReadTheDocs 主题，支持：
- 响应式设计
- 深色模式
- 移动端优化
- 打印样式

## 📊 构建选项

```bash
# HTML 输出
make html

# PDF 输出
make latexpdf

# ePub 输出
make epub

# 清理构建
make clean

# 检查链接
make linkcheck
```

## 🔗 Breathe 集成

使用 Breathe 集成 Doxygen 输出：

```python
# conf.py
extensions = ['breathe']
breathe_projects = {'Phoenix': '../doxygen/xml'}
breathe_default_project = 'Phoenix'
```

---
*最后更新：2026-03-26*
