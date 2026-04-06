# Phoenix Engine WASM 调试指南

本文档提供完整的浏览器调试流程，用于验证 WASM 加载问题的修复。

---

## 🚀 快速启动

### 1. 启动专用服务器

```bash
cd /home/admin/.openclaw/workspace/phoenix-engine/demo-showcase

# 使用修复后的服务器（正确配置 WASM MIME 类型）
python3 phoenix-server.py 8080
```

### 2. 访问页面

打开浏览器访问：`http://localhost:8080/index.html`

### 3. 使用修复后的 JS

编辑 `index.html`，将：
```html
<script src="demo-wasm.js"></script>
```

改为：
```html
<script src="demo-wasm-fixed.js"></script>
```

---

## 🔍 浏览器调试步骤

### Chrome DevTools 调试流程

#### 步骤 1：检查 WASM 文件加载

1. 打开 DevTools (F12)
2. 切换到 **Network** 面板
3. 刷新页面
4. 查找 `phoenix-engine.wasm`
5. 检查响应头：
   - ✅ 正确：`Content-Type: application/wasm`
   - ❌ 错误：`Content-Type: text/html` 或其他

#### 步骤 2：检查控制台日志

1. 切换到 **Console** 面板
2. 查找 `[PhoenixWasmLoader]` 前缀的日志
3. 正常流程应显示：
   ```
   [PhoenixWasmLoader] Initializing...
   [PhoenixWasmLoader] ✓ WASM support verified
   [PhoenixWasmLoader] WASM Content-Type: application/wasm
   [PhoenixWasmLoader] ✓ Loaded with streaming
   [PhoenixWasmLoader] ✓ WASM module loaded
   [PhoenixWasmLoader] ✓ GL initialized
   [PhoenixWasmLoader] ✓ Render loop started
   [PhoenixWasmLoader] ✓ Initialization complete in XXX.XXms
   ```

#### 步骤 3：检查 WASM 模块

1. 切换到 **Sources** 面板
2. 在左侧找到 `phoenix-engine.wasm`
3. 点击展开，应能看到反汇编的 WAT 格式
4. 检查导出的函数：
   - `demo_init`
   - `demo_update`
   - `demo_render`
   - `demo_resize`
   - 等...

#### 步骤 4：内存监控

1. 切换到 **Performance** 面板
2. 勾选 **Memory**
3. 开始录制
4. 操作页面（旋转、缩放等）
5. 停止录制，检查内存是否有泄漏

#### 步骤 5：性能分析

在 Console 中运行：
```javascript
// 检查 WASM 实例
console.log('WASM Instance:', window.phoenixLoader.instance);
console.log('Exported functions:', Object.keys(window.phoenixLoader.instance.exports));

// 检查内存
console.log('WASM Memory:', window.phoenixLoader.instance.exports.memory.buffer.byteLength / 1024 / 1024, 'MB');
console.log('JS Heap:', performance.memory ? performance.memory.usedJSHeapSize / 1024 / 1024 : 'N/A', 'MB');

// 性能测试
const start = performance.now();
for (let i = 0; i < 1000; i++) {
    window.phoenixLoader.safeCall('demo_update', 0.016);
}
console.log('1000 updates took:', performance.now() - start, 'ms');
```

---

## 🐛 常见问题诊断

### 问题 1：WASM 文件 404

**症状：**
```
Failed to load resource: the server responded with a status of 404
```

**检查：**
```bash
# 确认文件存在
ls -la phoenix-engine.wasm phoenix-engine.js

# 检查文件完整性
file phoenix-engine.wasm
```

**解决：**
```bash
# 重新构建（如果有 emsdk）
./build-demo.sh

# 或从备份恢复
cp ../build/dist/phoenix-wasm.wasm ./phoenix-engine.wasm
```

### 问题 2：MIME 类型错误

**症状：**
```
WebAssembly.instantiateStreaming failed because your server does not 
serve wasm with application/wasm MIME type
```

**检查：**
```bash
curl -I http://localhost:8080/phoenix-engine.wasm | grep Content-Type
```

**解决：**
使用 `phoenix-server.py` 而不是 `python -m http.server`

### 问题 3：CORS 错误

**症状：**
```
Access to fetch at 'file:///...' from origin 'null' has been blocked by CORS policy
```

**解决：**
- ❌ 不要直接打开 `file://` URL
- ✅ 使用 HTTP 服务器：`python3 phoenix-server.py 8080`

### 问题 4：WebGL 初始化失败

**症状：**
```
WebGL context creation failed
```

**检查：**
1. 访问 `https://get.webgl.org/webgl2/` 测试 WebGL 2 支持
2. 在 Console 运行：
   ```javascript
   const canvas = document.createElement('canvas');
   const gl = canvas.getContext('webgl2');
   console.log('WebGL2 supported:', !!gl);
   ```

**解决：**
- 更新显卡驱动
- 使用 Chrome/Firefox 浏览器
- 在浏览器设置中启用 WebGL

### 问题 5：内存不足

**症状：**
```
RangeError: WebAssembly.Memory: maximum size too large
```

**解决：**
在 `createImportObject()` 中调整内存限制：
```javascript
const memory = new WebAssembly.Memory({ 
    initial: 128,  // 减少初始值
    maximum: 256   // 减少最大值
});
```

---

## 📊 性能基准测试

在 Console 中运行以下测试：

```javascript
// 基准测试套件
async function runBenchmarks() {
    const loader = window.phoenixLoader;
    const results = {};
    
    // 1. WASM 调用开销
    const calls = 10000;
    const start = performance.now();
    for (let i = 0; i < calls; i++) {
        loader.safeCall('demo_update', 0.016);
    }
    results.wasmCallOverhead = (performance.now() - start) / calls;
    
    // 2. 内存访问
    const memStart = performance.now();
    const view = loader.getMemoryView('Uint8Array');
    for (let i = 0; i < 1000000; i++) {
        view[i % view.length] = i & 0xFF;
    }
    results.memoryAccess = performance.now() - memStart;
    
    // 3. FPS 统计
    await new Promise(r => setTimeout(r, 2000));
    results.currentFPS = loader.fps;
    results.frameTime = loader.frameTime;
    
    console.table(results);
    return results;
}

runBenchmarks();
```

**预期结果：**
- WASM 调用开销：< 0.01ms/次
- 内存访问：~50-100ms (1M 次写入)
- FPS: > 50 (取决于场景复杂度)

---

## 🛠️ 调试技巧

### 1. 启用详细日志

```javascript
window.phoenixLoader.debugMode = true;
location.reload();
```

### 2. 检查导入对象

```javascript
const imports = window.phoenixLoader.createImportObject();
console.log('Import object structure:');
console.log('env.memory:', imports.env.memory);
console.log('env.table:', imports.env.table);
console.log('env functions:', Object.keys(imports.env).filter(k => typeof imports.env[k] === 'function'));
```

### 3. 监控内存增长

```javascript
setInterval(() => {
    const mem = window.phoenixLoader.instance?.exports?.memory;
    if (mem) {
        console.log('WASM Memory:', mem.buffer.byteLength / 1024 / 1024, 'MB');
    }
}, 1000);
```

### 4. 捕获 WASM 异常

```javascript
// 包装所有 WASM 调用
const originalSafeCall = window.phoenixLoader.safeCall.bind(window.phoenixLoader);
window.phoenixLoader.safeCall = function(name, ...args) {
    try {
        const result = originalSafeCall(name, ...args);
        console.log(`✓ ${name}() returned:`, result);
        return result;
    } catch (e) {
        console.error(`✗ ${name}() failed:`, e);
        throw e;
    }
};
```

---

## ✅ 验证清单

完成修复后，应满足以下条件：

- [ ] WASM 文件正确加载（HTTP 200）
- [ ] Content-Type 为 `application/wasm`
- [ ] 无 CORS 错误
- [ ] Console 显示完整初始化日志
- [ ] Canvas 正常渲染
- [ ] FPS > 30
- [ ] 触摸/键盘控制响应正常
- [ ] 内存稳定（无明显泄漏）
- [ ] 错误处理正常工作
- [ ] 刷新页面后正常恢复

---

## 📝 报告问题

如果问题仍然存在，请收集以下信息：

1. **浏览器信息：**
   ```javascript
   console.log(navigator.userAgent);
   ```

2. **控制台完整日志**（包括错误堆栈）

3. **Network 面板截图**（显示 WASM 请求）

4. **WASM 文件信息：**
   ```bash
   file phoenix-engine.wasm
   ls -lh phoenix-engine.wasm
   ```

5. **服务器日志**（来自 `phoenix-server.py`）

---

## 🔗 相关资源

- [WebAssembly 官方文档](https://webassembly.org/)
- [Emscripten 文档](https://emscripten.org/)
- [Chrome DevTools WASM 调试](https://developer.chrome.com/docs/devtools/webassembly/)
- [WebAssembly 最佳实践](https://developer.mozilla.org/en-US/docs/WebAssembly/C_to_wasm)
