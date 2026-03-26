import { defineConfig } from 'vitepress'

export default defineConfig({
  title: 'Phoenix Engine',
  description: 'Cross-platform 3D Rendering Engine',
  
  head: [
    ['link', { rel: 'icon', href: '/favicon.ico' }],
    ['meta', { name: 'theme-color', content: '#6366f1' }],
  ],
  
  themeConfig: {
    logo: '/logo.svg',
    
    nav: [
      { text: 'Home', link: '/' },
      { text: 'Guide', link: '/guide/introduction' },
      { text: 'API', link: '/api/overview' },
      { text: 'Tutorials', link: '/tutorials/' },
      { text: 'Examples', link: '/examples/' },
      { text: 'Blog', link: '/blog/' },
    ],
    
    sidebar: {
      '/guide/': [
        {
          text: 'Getting Started',
          items: [
            { text: 'Introduction', link: '/guide/introduction' },
            { text: 'Installation', link: '/guide/installation' },
            { text: 'Quick Start', link: '/guide/quick-start' },
          ],
        },
        {
          text: 'Core Concepts',
          items: [
            { text: 'Architecture', link: '/guide/architecture' },
            { text: 'Rendering Pipeline', link: '/guide/rendering-pipeline' },
            { text: 'Scene Management', link: '/guide/scene-management' },
            { text: 'Resource System', link: '/guide/resource-system' },
          ],
        },
        {
          text: 'Advanced Topics',
          items: [
            { text: 'PBR Materials', link: '/guide/pbr-materials' },
            { text: 'Lighting', link: '/guide/lighting' },
            { text: 'Shadows', link: '/guide/shadows' },
            { text: 'Post-Processing', link: '/guide/post-processing' },
            { text: 'Animation', link: '/guide/animation' },
          ],
        },
        {
          text: 'Platform Guides',
          items: [
            { text: 'Windows', link: '/guide/platforms/windows' },
            { text: 'Linux', link: '/guide/platforms/linux' },
            { text: 'macOS', link: '/guide/platforms/macos' },
            { text: 'Android', link: '/guide/platforms/android' },
            { text: 'iOS', link: '/guide/platforms/ios' },
            { text: 'Web', link: '/guide/platforms/web' },
          ],
        },
        {
          text: 'Security',
          items: [
            { text: 'Security Overview', link: '/guide/security/overview' },
            { text: 'Rust Core', link: '/guide/security/rust-core' },
            { text: 'Encryption', link: '/guide/security/encryption' },
            { text: 'Audit Logging', link: '/guide/security/audit-logging' },
          ],
        },
      ],
      '/api/': [
        {
          text: 'API Reference',
          items: [
            { text: 'Overview', link: '/api/overview' },
            { text: 'Core', link: '/api/core/' },
            { text: 'Math', link: '/api/math/' },
            { text: 'Render', link: '/api/render/' },
            { text: 'Scene', link: '/api/scene/' },
            { text: 'Resource', link: '/api/resource/' },
            { text: 'Security', link: '/api/security/' },
          ],
        },
      ],
      '/tutorials/': [
        {
          text: 'Tutorials',
          items: [
            { text: 'Overview', link: '/tutorials/' },
            { text: 'Hello Triangle', link: '/tutorials/hello-triangle' },
            { text: 'Loading Models', link: '/tutorials/loading-models' },
            { text: 'PBR Materials', link: '/tutorials/pbr-materials' },
            { text: 'Lighting Setup', link: '/tutorials/lighting' },
            { text: 'Animation', link: '/tutorials/animation' },
            { text: 'Web Integration', link: '/tutorials/web-integration' },
          ],
        },
      ],
    },
    
    socialLinks: [
      { icon: 'github', link: 'https://github.com/phoenix-engine/phoenix-engine' },
      { icon: 'discord', link: 'https://discord.gg/phoenix-engine' },
      { icon: 'twitter', link: 'https://twitter.com/phoenixengine' },
    ],
    
    footer: {
      message: 'Released under the MIT License.',
      copyright: 'Copyright © 2026 Phoenix Engine Team',
    },
    
    search: {
      provider: 'local',
    },
  },
  
  markdown: {
    lineNumbers: true,
  },
})
