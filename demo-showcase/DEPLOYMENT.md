# Phoenix Engine Demo - 部署指南

## 📋 部署前检查

### 1. 构建环境

```bash
# 安装 Emscripten (如未安装)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# 验证安装
emcc --version
```

### 2. 构建 WASM

```bash
cd demo-showcase
bash build.sh
```

构建完成后，`dist/` 目录将包含所有部署文件。

## 🚀 部署到服务器

### 方法一：自动部署脚本

```bash
cd /home/admin/.openclaw/workspace/phoenix-engine
bash deploy-demo.sh
```

脚本会自动：
1. 构建 WASM 模块
2. 通过 rsync 同步到服务器
3. 部署演示中心页面

### 方法二：手动部署

```bash
# 1. 构建
cd demo-showcase && bash build.sh

# 2. 上传到服务器
scp -r dist/* root@47.245.126.212:/var/www/html/demo/

# 3. 上传演示中心
scp demo-center.html root@47.245.126.212:/var/www/html/
```

## 🔧 服务器配置

### Nginx 配置

```bash
# 复制配置文件
sudo cp nginx-demo.conf /etc/nginx/sites-available/phoenix-demo

# 创建软链接
sudo ln -s /etc/nginx/sites-available/phoenix-demo /etc/nginx/sites-enabled/

# 测试配置
sudo nginx -t

# 重载 Nginx
sudo systemctl reload nginx
```

### 目录权限

```bash
sudo chown -R www-data:www-data /var/www/html/demo
sudo chmod -R 755 /var/www/html/demo
```

## 📱 访问地址

- **WASM 演示**: http://47.245.126.212:3000/demo/
- **演示中心**: http://47.245.126.212:3000/demo-center.html

## ✅ 验证清单

部署后请检查：

- [ ] 页面正常加载
- [ ] WASM 模块成功加载
- [ ] 3D 场景正常渲染
- [ ] 触摸交互正常工作
- [ ] 性能监控显示数据
- [ ] 控制面板功能正常
- [ ] 移动端适配正确
- [ ] HTTPS 证书有效 (如使用)

## 🐛 故障排除

### WASM 加载失败

```bash
# 检查 MIME 类型
curl -I http://47.245.126.212:3000/demo/demo-app.wasm

# 应返回：Content-Type: application/wasm
```

### 跨域问题

确保 Nginx 配置包含：
```nginx
add_header Access-Control-Allow-Origin "*";
```

### 性能问题

1. 启用 Brotli 压缩
2. 检查 WASM 文件大小
3. 使用 Chrome DevTools 分析性能

## 📊 性能优化建议

### 服务器端

1. **启用 HTTP/2**
```nginx
listen 3000 ssl http2;
```

2. **启用 Brotli 压缩**
```bash
sudo apt install libnginx-mod-brotli
```

3. **配置缓存**
```nginx
proxy_cache_path /var/cache/nginx levels=1:2 keys_zone=phoenix:100m;
```

### 客户端

1. **Service Worker** - 已包含在 `sw.js`
2. **资源预加载**
```html
<link rel="preload" href="demo-app.wasm" as="fetch" crossorigin>
```
3. **懒加载** - 按需加载资源

## 🔄 更新流程

```bash
# 1. 修改代码
# 编辑 demo-app.cpp 或其他文件

# 2. 重新构建
cd demo-showcase && bash build.sh

# 3. 部署更新
cd .. && bash deploy-demo.sh

# 4. 清除浏览器缓存
# 或更新 Service Worker 版本号
```

## 📈 监控建议

### 性能监控

在 `index.html` 中添加：
```javascript
// 发送到分析服务器
fetch('/api/analytics', {
    method: 'POST',
    body: JSON.stringify({
        fps: phoenixLoader.fps,
        drawCalls: phoenixLoader.drawCalls,
        memory: phoenixLoader.memoryUsage
    })
});
```

### 错误监控

```javascript
window.onerror = function(msg, url, line) {
    fetch('/api/error', {
        method: 'POST',
        body: JSON.stringify({ msg, url, line })
    });
};
```

## 📞 技术支持

如有问题，请检查：
1. 浏览器控制台错误
2. Nginx 错误日志：`/var/log/nginx/phoenix-demo-error.log`
3. 服务器系统日志：`journalctl -u nginx`

---

**Phoenix Engine Team** 🦅
