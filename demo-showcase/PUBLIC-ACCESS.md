# 🚀 Phoenix Engine WASM Demo - 公网访问

## ✅ 服务已启动

**服务器地址:** `http://47.245.126.212:8888`

**内网地址:** `http://172.17.27.161:8888`

---

## 📱 访问方式

### 方式 1: 直接访问（推荐）

在浏览器中打开：
```
http://47.245.126.212:8888
```

### 方式 2: 如果端口被防火墙阻止

如果无法访问，可能需要开放防火墙端口：

```bash
# 阿里云 ECS - 在控制台安全组添加入站规则
# 端口范围：8888/8888
# 授权对象：0.0.0.0/0
# 协议类型：TCP
```

---

## 🔍 验证步骤

### 1. 检查 WASM 文件是否可访问

在浏览器或命令行测试：
```bash
curl -I http://47.245.126.212:8888/phoenix-engine.wasm
```

应返回：
```
HTTP/1.0 200 OK
Content-type: application/wasm
```

### 2. 访问主页面

打开浏览器访问：`http://47.245.126.212:8888`

应看到 Phoenix Engine 加载界面

### 3. 浏览器调试（F12）

- **Console**: 查看 `[PhoenixWasmLoader]` 日志
- **Network**: 检查 WASM 文件加载状态
- **调试命令**: 在 Console 中输入 `window.phoenixLoader`

---

## 🛠️ 如果无法访问

### 问题 1: 连接超时

**可能原因:** 防火墙/安全组阻止

**解决方法:**
1. 登录阿里云控制台
2. 找到 ECS 实例
3. 进入"安全组" → "入站规则"
4. 添加规则：
   - 端口：`8888`
   - 协议：`TCP`
   - 授权对象：`0.0.0.0/0`

### 问题 2: 页面加载但 WASM 失败

**检查:**
```bash
curl http://47.245.126.212:8888/phoenix-engine.wasm | head -c 20 | xxd
```

应输出 WASM 魔数：`00 61 73 6d` (即 `\0asm`)

### 问题 3: MIME 类型错误

如果 Console 显示 MIME 类型错误，检查：
```bash
curl -I http://47.245.126.212:8888/phoenix-engine.wasm | grep Content-Type
```

应为：`Content-type: application/wasm`

---

## 📊 服务状态

```bash
# 检查服务器是否运行
ps aux | grep phoenix-server

# 查看日志
tail -f /tmp/phoenix-server.log

# 重启服务
pkill -f phoenix-server.py
cd /home/admin/.openclaw/workspace/phoenix-engine/demo-showcase
nohup python3 phoenix-server.py 8888 > /tmp/phoenix-server.log 2>&1 &
```

---

## 🔗 快速链接

- **Demo 页面:** http://47.245.126.212:8888
- **WASM 文件:** http://47.245.126.212:8888/phoenix-engine.wasm
- **JS 文件:** http://47.245.126.212:8888/demo-wasm-fixed.js
- **调试指南:** http://47.245.126.212:8888/DEBUG-GUIDE.md

---

## 📝 修复说明

本次修复解决了以下问题：

1. ✅ **MIME 类型配置** - 服务器正确返回 `application/wasm`
2. ✅ **CORS 支持** - 允许跨域访问
3. ✅ **加载流程优化** - 流式加载 + fallback
4. ✅ **错误处理改进** - 用户友好的错误提示
5. ✅ **内存管理** - 防止 use-after-free

详细技术报告见：`FIX-SUMMARY.md`

---

**更新时间:** 2026-03-30 12:35 CST  
**服务状态:** 🟢 运行中
