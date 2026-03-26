# 性能优化第四课：内存优化

## 🎯 学习目标
- 纹理压缩
- 内存池
- 资源流式加载

## 📝 代码示例

```cpp
// 使用压缩纹理
config.format = phoenix::TextureFormat::BC7;

// 内存池
MemoryPool pool(1024 * 1024);  // 1MB
void* ptr = pool.allocate(256);
```

## 📚 下一步
**下一课**: [第五课：移动端优化](lesson-05-mobile.md)
