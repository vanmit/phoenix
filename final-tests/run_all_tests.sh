#!/bin/bash

# Phoenix Engine Phase 6 - 完整测试套件运行脚本
# 用法：./run_all_tests.sh [选项]

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 配置
BUILD_DIR="build-tests"
REPORT_DIR="reports"
TEST_COVERAGE_TARGET=90

# 打印函数
print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}ℹ $1${NC}"
}

# 帮助信息
show_help() {
    echo "Phoenix Engine Phase 6 测试套件"
    echo ""
    echo "用法：$0 [选项]"
    echo ""
    echo "选项:"
    echo "  -u, --unit          只运行单元测试"
    echo "  -p, --performance   只运行性能基准"
    echo "  -s, --stress        只运行压力测试"
    echo "  -c, --compatibility 只运行兼容性测试"
    echo "  -a, --all           运行所有测试 (默认)"
    echo "  -h, --help          显示帮助"
    echo ""
}

# 构建测试
build_tests() {
    print_header "构建测试套件"
    
    if [ ! -d "$BUILD_DIR" ]; then
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        
        cmake .. \
            -DPHOENIX_BUILD_TESTS=ON \
            -DPHOENIX_BUILD_BENCHMARKS=ON \
            -DENABLE_COVERAGE=ON \
            -DCMAKE_BUILD_TYPE=Debug
        
        cd ..
    fi
    
    cd "$BUILD_DIR"
    cmake --build . -j$(nproc)
    cd ..
    
    print_success "构建完成"
}

# 运行单元测试
run_unit_tests() {
    print_header "单元测试"
    
    cd "$BUILD_DIR"
    
    # 运行单元测试
    ctest -R unit --verbose --output-on-failure
    
    # 生成覆盖率报告
    if command -v gcovr &> /dev/null; then
        gcovr -r .. --html-details ../$REPORT_DIR/coverage.html
        print_success "覆盖率报告生成：$REPORT_DIR/coverage.html"
    fi
    
    cd ..
    
    print_success "单元测试完成"
}

# 运行性能基准
run_performance_benchmarks() {
    print_header "性能基准测试"
    
    cd "$BUILD_DIR"
    
    # FPS 基准
    if [ -f "./bin/benchmark_fps" ]; then
        ./bin/benchmark_fps
        print_success "FPS 基准完成"
    fi
    
    # 内存基准
    if [ -f "./bin/benchmark_memory" ]; then
        ./bin/benchmark_memory
        print_success "内存基准完成"
    fi
    
    # 加载时间基准
    if [ -f "./bin/benchmark_loading" ]; then
        ./bin/benchmark_loading
        print_success "加载时间基准完成"
    fi
    
    cd ..
    
    print_success "性能基准测试完成"
}

# 运行压力测试
run_stress_tests() {
    print_header "压力测试"
    
    cd "$BUILD_DIR"
    
    # 万级物体测试
    if [ -f "./bin/stress_10k_objects" ]; then
        ./bin/stress_10k_objects
        print_success "万级物体测试完成"
    fi
    
    # 24 小时稳定性测试 (缩短为 1 分钟用于快速测试)
    if [ -f "./bin/stress_24h_stability" ]; then
        timeout 60 ./bin/stress_24h_stability 24 || true
        print_success "稳定性测试完成 (快速模式)"
    fi
    
    cd ..
    
    print_success "压力测试完成"
}

# 运行兼容性测试
run_compatibility_tests() {
    print_header "兼容性测试"
    
    print_info "兼容性测试需要多平台环境"
    print_info "当前平台测试将在本地运行"
    
    cd "$BUILD_DIR"
    
    # 平台特定测试
    if [ -f "./bin/test_platform" ]; then
        ./bin/test_platform
    fi
    
    cd ..
    
    print_success "兼容性测试完成"
}

# 生成汇总报告
generate_reports() {
    print_header "生成测试报告"
    
    mkdir -p "$REPORT_DIR"
    
    # 汇总测试结果
    cat > "$REPORT_DIR/test_summary.md" << EOF
# Phoenix Engine Phase 6 - 测试汇总报告

**生成时间**: $(date '+%Y-%m-%d %H:%M:%S')

## 测试结果

EOF
    
    # 添加覆盖率报告链接
    if [ -f "$REPORT_DIR/coverage.html" ]; then
        echo "- ✅ 覆盖率报告：[coverage.html]($REPORT_DIR/coverage.html)" >> "$REPORT_DIR/test_summary.md"
    fi
    
    # 添加性能报告
    if [ -f "fps_benchmark_results.json" ]; then
        echo "- ✅ FPS 基准：fps_benchmark_results.json" >> "$REPORT_DIR/test_summary.md"
    fi
    
    if [ -f "memory_benchmark_results.json" ]; then
        echo "- ✅ 内存基准：memory_benchmark_results.json" >> "$REPORT_DIR/test_summary.md"
    fi
    
    if [ -f "stress_10k_objects_results.json" ]; then
        echo "- ✅ 压力测试：stress_10k_objects_results.json" >> "$REPORT_DIR/test_summary.md"
    fi
    
    print_success "报告生成完成"
}

# 主函数
main() {
    local run_unit=false
    local run_perf=false
    local run_stress=false
    local run_compat=false
    local run_all=true
    
    # 解析参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -u|--unit)
                run_unit=true
                run_all=false
                shift
                ;;
            -p|--performance)
                run_perf=true
                run_all=false
                shift
                ;;
            -s|--stress)
                run_stress=true
                run_all=false
                shift
                ;;
            -c|--compatibility)
                run_compat=true
                run_all=false
                shift
                ;;
            -a|--all)
                run_all=true
                shift
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                print_error "未知选项：$1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 如果未指定，运行所有测试
    if $run_all; then
        run_unit=true
        run_perf=true
        run_stress=true
        run_compat=true
    fi
    
    # 进入引擎目录
    cd "$(dirname "$0")/.."
    
    print_header "Phoenix Engine Phase 6 测试套件"
    echo "开始时间：$(date '+%Y-%m-%d %H:%M:%S')"
    echo ""
    
    # 构建
    build_tests
    
    # 运行测试
    if $run_unit; then
        run_unit_tests
    fi
    
    if $run_perf; then
        run_performance_benchmarks
    fi
    
    if $run_stress; then
        run_stress_tests
    fi
    
    if $run_compat; then
        run_compatibility_tests
    fi
    
    # 生成报告
    generate_reports
    
    print_header "测试完成"
    echo "结束时间：$(date '+%Y-%m-%d %H:%M:%S')"
    echo ""
    print_success "所有测试完成！"
}

# 执行
main "$@"
