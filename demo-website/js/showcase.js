/**
 * Phoenix Engine Demo Showcase - Main JavaScript
 * Handles demo loading, filtering, search, and interactions
 */

class DemoShowcase {
  constructor() {
    this.demos = [];
    this.filteredDemos = [];
    this.currentFilter = 'all';
    this.searchQuery = '';
    this.init();
  }

  async init() {
    await this.loadDemos();
    this.setupEventListeners();
    this.renderDemos();
    this.updateStats();
    console.log('🦅 Phoenix Engine Demo Showcase initialized');
  }

  async loadDemos() {
    try {
      const response = await fetch('demos.json');
      const data = await response.json();
      this.demos = data.demos;
      this.showcaseInfo = data.showcase;
      this.categories = data.categories;
      this.stats = data.stats;
      
      // Update page title
      document.title = this.showcaseInfo.title;
      
      // Update header if exists
      const headerTitle = document.querySelector('.header h1');
      const headerDesc = document.querySelector('.header p');
      if (headerTitle) headerTitle.textContent = this.showcaseInfo.title;
      if (headerDesc) headerDesc.textContent = this.showcaseInfo.description;
      
    } catch (error) {
      console.error('Failed to load demos:', error);
      this.showError('无法加载演示数据，请刷新页面重试');
    }
  }

  setupEventListeners() {
    // Search input
    const searchInput = document.getElementById('searchInput');
    if (searchInput) {
      searchInput.addEventListener('input', (e) => {
        this.searchQuery = e.target.value.toLowerCase().trim();
        this.filterDemos();
      });
    }

    // Filter buttons
    document.querySelectorAll('.filter-btn').forEach(btn => {
      btn.addEventListener('click', (e) => {
        document.querySelectorAll('.filter-btn').forEach(b => b.classList.remove('active'));
        e.target.classList.add('active');
        this.currentFilter = e.target.dataset.filter;
        this.filterDemos();
      });
    });

    // Demo card clicks
    document.addEventListener('click', (e) => {
      const launchBtn = e.target.closest('.btn-launch');
      if (launchBtn) {
        const demoId = launchBtn.dataset.demoId;
        this.launchDemo(demoId);
      }

      const shareBtn = e.target.closest('.btn-share');
      if (shareBtn) {
        const demoId = shareBtn.dataset.demoId;
        this.shareDemo(demoId);
      }
    });
  }

  filterDemos() {
    this.filteredDemos = this.demos.filter(demo => {
      // Category filter
      const categoryMatch = this.currentFilter === 'all' || 
                           demo.category === this.currentFilter;
      
      // Search filter
      const searchMatch = this.searchQuery === '' ||
                         demo.title.toLowerCase().includes(this.searchQuery) ||
                         demo.description.toLowerCase().includes(this.searchQuery) ||
                         demo.tags.some(tag => tag.toLowerCase().includes(this.searchQuery));
      
      return categoryMatch && searchMatch;
    });

    this.renderDemos();
  }

  renderDemos() {
    const grid = document.getElementById('demoGrid');
    if (!grid) return;

    if (this.filteredDemos.length === 0) {
      grid.innerHTML = `
        <div class="no-results" style="grid-column: 1 / -1;">
          <h3>😕 未找到匹配的演示</h3>
          <p>尝试调整搜索关键词或筛选条件</p>
        </div>
      `;
      return;
    }

    // Sort by popularity (featured first, then by popularity)
    const sorted = [...this.filteredDemos].sort((a, b) => {
      if (a.featured && !b.featured) return -1;
      if (!a.featured && b.featured) return 1;
      return b.popularity - a.popularity;
    });

    grid.innerHTML = sorted.map(demo => this.createDemoCard(demo)).join('');
  }

  createDemoCard(demo) {
    const featuredClass = demo.featured ? 'featured' : '';
    const thumbnail = demo.thumbnail || `data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 400 200"><rect fill="%23ff6b35" width="400" height="200"/><text x="50%" y="50%" dominant-baseline="middle" text-anchor="middle" font-size="48" fill="white">🦅</text></svg>`;
    
    const statsHtml = demo.techSpecs ? `
      <div class="demo-stats">
        ${Object.entries(demo.techSpecs).slice(0, 4).map(([key, value]) => `
          <div class="stat-item">
            <div class="stat-value">${this.formatStatValue(key, value)}</div>
            <div class="stat-label">${this.getStatLabel(key)}</div>
          </div>
        `).join('')}
      </div>
    ` : '';

    const featuresHtml = demo.features && demo.features.length > 0 ? `
      <div class="demo-features">
        <h4>技术特性</h4>
        <ul>
          ${demo.features.slice(0, 5).map(f => `<li>${f}</li>`).join('')}
        </ul>
      </div>
    ` : '';

    return `
      <article class="demo-card ${featuredClass}" data-demo-id="${demo.id}">
        <div class="demo-thumbnail">
          <img src="${thumbnail}" alt="${demo.title}" onerror="this.parentElement.innerHTML='🦅'">
        </div>
        ${demo.popularity ? `<div class="popularity-badge">${demo.popularity}</div>` : ''}
        <div class="demo-content">
          <h3 class="demo-title">${demo.title}</h3>
          <p class="demo-description">${demo.description}</p>
          <div class="demo-tags">
            ${demo.tags.map(tag => `<span class="demo-tag">${tag}</span>`).join('')}
          </div>
          ${statsHtml}
          ${featuresHtml}
          <div class="demo-actions">
            <a href="${demo.demoUrl}" class="btn btn-primary btn-launch" data-demo-id="${demo.id}">
              🚀 启动演示
            </a>
            <button class="btn btn-secondary btn-share" data-demo-id="${demo.id}">
              📤 分享
            </button>
          </div>
        </div>
      </article>
    `;
  }

  formatStatValue(key, value) {
    if (typeof value === 'number') {
      if (value >= 1000) {
        return (value / 1000).toFixed(1) + 'k';
      }
      return value.toString();
    }
    return value;
  }

  getStatLabel(key) {
    const labels = {
      'drawCalls': '绘制调用',
      'vertices': '顶点数',
      'textures': '纹理数',
      'shaderVariants': '着色器变体',
      'shadowMaps': '阴影贴图',
      'lightSources': '光源数',
      'cascades': '级联数',
      'resolution': '分辨率',
      'bones': '骨骼数',
      'animations': '动画数',
      'blendTrees': '混合树',
      'morphTargets': '形态目标',
      'maxParticles': '最大粒子',
      'emitters': '发射器',
      'simulations': '模拟方式',
      'renderMode': '渲染模式',
      'objects': '物体数',
      'instances': '实例数',
      'culling': '剔除方式'
    };
    return labels[key] || key;
  }

  launchDemo(demoId) {
    const demo = this.demos.find(d => d.id === demoId);
    if (demo) {
      // Track view
      this.trackView(demoId);
      console.log(`🚀 Launching demo: ${demo.title}`);
    }
  }

  async shareDemo(demoId) {
    const demo = this.demos.find(d => d.id === demoId);
    if (!demo) return;

    const shareData = {
      title: demo.title,
      text: demo.description,
      url: window.location.origin + window.location.pathname + demo.demoUrl
    };

    if (navigator.share) {
      try {
        await navigator.share(shareData);
      } catch (err) {
        console.log('Share cancelled');
      }
    } else {
      // Fallback: copy to clipboard
      try {
        await navigator.clipboard.writeText(shareData.url);
        alert('链接已复制到剪贴板！');
      } catch (err) {
        console.error('Failed to copy:', err);
      }
    }
  }

  trackView(demoId) {
    // Increment view count locally
    const demo = this.demos.find(d => d.id === demoId);
    if (demo) {
      demo.popularity = (demo.popularity || 0) + 1;
      
      // Save to localStorage
      const views = JSON.parse(localStorage.getItem('demoViews') || '{}');
      views[demoId] = (views[demoId] || 0) + 1;
      localStorage.setItem('demoViews', JSON.stringify(views));
      
      this.updateStats();
    }
  }

  updateStats() {
    const statsEl = document.getElementById('showcaseStats');
    if (statsEl && this.stats) {
      const views = this.getTotalViews();
      statsEl.innerHTML = `
        <span>📊 演示总数：${this.stats.totalDemos}</span>
        <span>👁 总访问：${views}</span>
        <span>📅 更新：${this.stats.lastReset}</span>
      `;
    }
  }

  getTotalViews() {
    const views = JSON.parse(localStorage.getItem('demoViews') || '{}');
    return Object.values(views).reduce((sum, v) => sum + v, 0);
  }

  showError(message) {
    const grid = document.getElementById('demoGrid');
    if (grid) {
      grid.innerHTML = `
        <div class="no-results" style="grid-column: 1 / -1;">
          <h3>⚠️ 错误</h3>
          <p>${message}</p>
        </div>
      `;
    }
  }
}

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
  window.showcase = new DemoShowcase();
});

// Export for module usage
if (typeof module !== 'undefined' && module.exports) {
  module.exports = DemoShowcase;
}
