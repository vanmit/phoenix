# Phoenix Engine Release Checklist

Use this checklist for every release to ensure consistency and quality.

## Pre-Release

### Code Quality
- [ ] All tests pass (`ctest --output-on-failure`)
- [ ] No compiler warnings (`-Werror` in CI)
- [ ] Code formatted (`clang-format`, `rustfmt`)
- [ ] Static analysis clean (`clang-tidy`, `clippy`)
- [ ] Security audit completed

### Documentation
- [ ] CHANGELOG.md updated
- [ ] API documentation generated
- [ ] README.md version updated
- [ ] Migration guide (for breaking changes)
- [ ] Release notes drafted

### Version Management
- [ ] Version number updated in CMakeLists.txt
- [ ] Version number updated in package.json (WASM)
- [ ] Git tag created (`git tag -a v1.0.0 -m "..."`)
- [ ] Version branch created (for major releases)

## Build & Package

### Platform Builds
- [ ] Windows (MSI/EXE/ZIP)
- [ ] Linux (DEB/RPM/AppImage)
- [ ] macOS (DMG)
- [ ] Android (APK/AAB)
- [ ] iOS (IPA)
- [ ] Web (NPM)
- [ ] Source archive

### Code Signing
- [ ] Windows Authenticode signature
- [ ] macOS Apple Developer ID signature
- [ ] iOS provisioning profile
- [ ] Android keystore signature

### Quality Assurance
- [ ] Checksums generated (SHA256)
- [ ] Package sizes within limits
- [ ] Install/uninstall tested on all platforms
- [ ] Demo application runs on all platforms

## Release

### GitHub
- [ ] Release created on GitHub
- [ ] Release notes published
- [ ] All artifacts uploaded
- [ ] Checksums published
- [ ] Pre-release flag removed (for stable releases)

### Package Managers
- [ ] NPM package published
- [ ] Homebrew formula updated
- [ ] AUR package updated (community)
- [ ] Winget manifest submitted

### Documentation
- [ ] Documentation site deployed
- [ ] API docs updated
- [ ] Changelog published on website
- [ ] Blog post (for major releases)

### Communication
- [ ] Release announcement on Discord
- [ ] Tweet/X post
- [ ] LinkedIn post
- [ ] Email to stakeholders (for major releases)

## Post-Release

### Monitoring
- [ ] GitHub Issues monitored for bugs
- [ ] Crash reports reviewed
- [ ] Performance metrics checked
- [ ] Download statistics tracked

### Follow-up
- [ ] Retrospective meeting scheduled
- [ ] Known issues documented
- [ ] Patch release planned (if needed)
- [ ] Next release milestone created

---

## Emergency Rollback

If critical issues are discovered:

1. **Assess severity**: Is rollback necessary?
2. **Create patch**: If possible, release v1.0.1 instead
3. **Rollback decision**: Team lead approves rollback
4. **Execute rollback**:
   - Mark release as pre-release
   - Remove binaries (keep source)
   - Announce on all channels
   - Deploy previous stable version
5. **Post-mortem**: Document lessons learned

---

## Release Cadence

| Release Type | Frequency | Example |
|--------------|-----------|---------|
| Major | Quarterly | v1.0.0, v2.0.0 |
| Minor | Monthly | v1.1.0, v1.2.0 |
| Patch | As needed | v1.0.1, v1.0.2 |
| Security | Immediate | v1.0.1-security |

---

*Last updated: 2026-03-26*
