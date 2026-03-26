#!/bin/bash
# Phoenix Engine Version Management Script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

show_help() {
    cat << EOF
Phoenix Engine Version Management

Usage: $(basename "$0") <command> [options]

Commands:
    current         Show current version
    set <version>   Set new version (SemVer: MAJOR.MINOR.PATCH)
    bump-major      Bump major version (breaking changes)
    bump-minor      Bump minor version (new features)
    bump-patch      Bump patch version (bug fixes)
    tag             Create git tag for current version
    changelog       Generate changelog from git commits
    release         Full release workflow

Options:
    -h, --help      Show this help message

Examples:
    $(basename "$0") current
    $(basename "$0") set 1.0.0
    $(basename "$0") bump-minor
    $(basename "$0") tag
    $(basename "$0") release

EOF
}

get_current_version() {
    grep -oP 'VERSION \K[0-9]+\.[0-9]+\.[0-9]+' "$PROJECT_ROOT/CMakeLists.txt" | head -1
}

set_version() {
    local new_version="$1"
    
    if [[ ! "$new_version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        log_error "Invalid version format. Use SemVer: MAJOR.MINOR.PATCH"
        exit 1
    fi
    
    log_info "Setting version to $new_version"
    
    # Update CMakeLists.txt
    sed -i "s/VERSION [0-9]\+\.[0-9]\+\.[0-9]\+/VERSION $new_version/" "$PROJECT_ROOT/CMakeLists.txt"
    
    # Update package.json (WASM)
    if [ -f "$PROJECT_ROOT/wasm/package.json" ]; then
        sed -i "s/\"version\": \"[^\"]*\"/\"version\": \"$new_version\"/" "$PROJECT_ROOT/wasm/package.json"
    fi
    
    # Update README.md
    sed -i "s/\*\*版本\*\*: [0-9]\+\.[0-9]\+\.[0-9]\+/\*\*版本\*\*: $new_version/" "$PROJECT_ROOT/README.md"
    
    # Update CHANGELOG.md header
    if [ -f "$PROJECT_ROOT/CHANGELOG.md" ]; then
        sed -i "s/## \[Unreleased\]/## [Unreleased]\n\n## [$new_version] - $(date +%Y-%m-%d)/" "$PROJECT_ROOT/CHANGELOG.md"
    fi
    
    log_success "Version updated to $new_version"
    log_info "Don't forget to commit and tag the release!"
}

bump_version() {
    local bump_type="$1"
    local current_version=$(get_current_version)
    
    IFS='.' read -r major minor patch <<< "$current_version"
    
    case "$bump_type" in
        major)
            major=$((major + 1))
            minor=0
            patch=0
            ;;
        minor)
            minor=$((minor + 1))
            patch=0
            ;;
        patch)
            patch=$((patch + 1))
            ;;
        *)
            log_error "Invalid bump type. Use: major, minor, or patch"
            exit 1
            ;;
    esac
    
    local new_version="$major.$minor.$patch"
    set_version "$new_version"
}

create_tag() {
    local version=$(get_current_version)
    local tag_name="v$version"
    
    log_info "Creating git tag: $tag_name"
    
    cd "$PROJECT_ROOT"
    
    # Check if tag already exists
    if git rev-parse "$tag_name" >/dev/null 2>&1; then
        log_error "Tag $tag_name already exists!"
        exit 1
    fi
    
    # Create annotated tag
    git tag -a "$tag_name" -m "Phoenix Engine $tag_name"
    
    log_success "Tag $tag_name created"
    log_info "Push with: git push origin $tag_name"
}

generate_changelog() {
    local version=$(get_current_version)
    local tag_name="v$version"
    
    log_info "Generating changelog for $tag_name"
    
    cd "$PROJECT_ROOT"
    
    # Get commits since last tag
    local last_tag=$(git describe --tags --abbrev=0 HEAD^ 2>/dev/null || echo "")
    
    if [ -n "$last_tag" ]; then
        git log --pretty=format:"* %s (%h)" "$last_tag"..HEAD >> "$PROJECT_ROOT/CHANGELOG.md.tmp"
        
        if [ -s "$PROJECT_ROOT/CHANGELOG.md.tmp" ]; then
            echo "" >> "$PROJECT_ROOT/CHANGELOG.md.tmp"
            cat "$PROJECT_ROOT/CHANGELOG.md" >> "$PROJECT_ROOT/CHANGELOG.md.tmp"
            mv "$PROJECT_ROOT/CHANGELOG.md.tmp" "$PROJECT_ROOT/CHANGELOG.md"
            log_success "Changelog updated"
        else
            rm -f "$PROJECT_ROOT/CHANGELOG.md.tmp"
            log_info "No new commits since last tag"
        fi
    else
        log_info "No previous tags found"
    fi
}

full_release() {
    log_info "Starting full release workflow..."
    
    local version=$(get_current_version)
    
    echo ""
    log_info "Current version: $version"
    echo ""
    
    # Confirm release
    read -p "Create release for v$version? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        log_info "Release cancelled"
        exit 0
    fi
    
    # Generate changelog
    generate_changelog
    
    # Create tag
    create_tag
    
    # Build packages
    log_info "Building packages..."
    "$SCRIPT_DIR/build-all.sh"
    
    log_success "Release workflow complete!"
    log_info "Next steps:"
    log_info "  1. Review generated packages in release/packages/"
    log_info "  2. Test on all platforms"
    log_info "  3. Push tag: git push origin v$version"
    log_info "  4. Create GitHub Release"
}

# Main command handler
case "${1:-}" in
    current)
        get_current_version
        ;;
    set)
        set_version "$2"
        ;;
    bump-major)
        bump_version "major"
        ;;
    bump-minor)
        bump_version "minor"
        ;;
    bump-patch)
        bump_version "patch"
        ;;
    tag)
        create_tag
        ;;
    changelog)
        generate_changelog
        ;;
    release)
        full_release
        ;;
    -h|--help)
        show_help
        ;;
    *)
        show_help
        exit 1
        ;;
esac
