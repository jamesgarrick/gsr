#!/usr/bin/env bash
set -e

REPO="jamesgarrick/gsr"
BINARY_NAME="gsr"
INSTALL_DIR="${HOME}/.local/bin"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

info()  { echo -e "${NC}$1"; }
warn()  { echo -e "${YELLOW}warning:${NC} $1"; }
error() { echo -e "${RED}error:${NC} $1" >&2; exit 1; }

INSTALL_METHOD="binary"
while [[ $# -gt 0 ]]; do
    case "$1" in
        --install-method)
            INSTALL_METHOD="$2"
            shift 2
            ;;
        --install-method=*)
            INSTALL_METHOD="${1#*=}"
            shift
            ;;
        -h|--help)
            echo "usage: curl -fsSL <url>/install.sh | bash [-s -- OPTIONS]"
            echo ""
            echo "options:"
            echo "  --install-method <method>  Installation method: binary (default) or git"
            echo "  -h, --help                 Show this help"
            exit 0
            ;;
        *)
            error "unknown option: $1"
            ;;
    esac
done

case "$INSTALL_METHOD" in
    binary|git) ;;
    *) error "invalid install method: $INSTALL_METHOD (use 'binary' or 'git')" ;;
esac

check_deps() {
    local missing=()
    for cmd in "$@"; do
        command -v "$cmd" &>/dev/null || missing+=("$cmd")
    done
    if [[ ${#missing[@]} -gt 0 ]]; then
        error "missing required commands: ${missing[*]}"
    fi
}

detect_platform() {
    local os arch

    case "$(uname -s)" in
        Linux*)  os="linux" ;;
        Darwin*) os="darwin" ;;
        *)       error "unsupported OS: $(uname -s)" ;;
    esac

    case "$(uname -m)" in
        x86_64|amd64)  arch="amd64" ;;
        arm64|aarch64) arch="arm64" ;;
        *)             error "unsupported architecture: $(uname -m)" ;;
    esac

    echo "${os}-${arch}"
}

get_latest_version() {
    local response
    response=$(curl -fsSL "https://api.github.com/repos/${REPO}/releases/latest" 2>/dev/null) || {
        error "failed to fetch latest release from GitHub"
    }

    # get tag for versioning
    echo "$response" | grep -o '"tag_name": *"[^"]*"' | head -1 | cut -d'"' -f4
}

add_to_path() {
    local rc_file="$1"
    local line='export PATH="$HOME/.local/bin:$PATH"'

    if [[ -f "$rc_file" ]] && grep -q '\.local/bin' "$rc_file"; then
        return 1
    fi

    {
        echo ""
        echo "# added by ${BINARY_NAME} installer"
        echo "$line"
    } >> "$rc_file"
    return 0
}

setup_path() {
    if echo "$PATH" | grep -q "${INSTALL_DIR}"; then
        info "${INSTALL_DIR} already in PATH"
        return
    fi

    local rc_file
    case "$(basename "$SHELL")" in
        zsh)  rc_file="${HOME}/.zshrc" ;;
        bash) rc_file="${HOME}/.bashrc" ;;
        *)    rc_file="${HOME}/.profile" ;;
    esac

    if add_to_path "$rc_file"; then
        info "added ${INSTALL_DIR} to PATH in ${rc_file}"
        echo ""
        warn "run 'source ${rc_file}' or restart your terminal to use ${BINARY_NAME}"
    else
        info "${INSTALL_DIR} already configured in ${rc_file}"
    fi
}

install_binary() {
    check_deps curl

    local platform version download_url temp_file

    platform=$(detect_platform)

    version=$(get_latest_version)
    [[ -z "$version" ]] && error "could not determine latest version"

    download_url="https://github.com/${REPO}/releases/download/${version}/${BINARY_NAME}"

    info "downloading ${BINARY_NAME} ${version}"
    temp_file=$(mktemp)
    trap 'rm -f "$temp_file"' EXIT

    if ! curl -fsSL "$download_url" -o "$temp_file"; then
        error "failed to download binary from ${download_url}"
    fi

    # must be exec
    if file "$temp_file" | grep -qiE 'text|html'; then
        error "download failed"
    fi

    mkdir -p "${INSTALL_DIR}"
    install -m 755 "$temp_file" "${INSTALL_DIR}/${BINARY_NAME}"

    info "installed ${BINARY_NAME} to ${INSTALL_DIR}/${BINARY_NAME}"
}

install_git() {
    check_deps git make g++

    local clone_dir="./gsr"

    if [[ -d "$clone_dir/.git" ]]; then
        info "updating existing clone..."
        git -C "$clone_dir" fetch --tags
        git -C "$clone_dir" checkout main 2>/dev/null || git -C "$clone_dir" checkout master
        git -C "$clone_dir" pull
    else
        info "cloning repository..."
        git clone "https://github.com/${REPO}.git" "$clone_dir"
    fi

    info "building..."
    make -C "$clone_dir" clean 2>/dev/null || true
    make -C "$clone_dir"

    echo ""
    info "source cloned to $(cd "$clone_dir" && pwd)"
}

main() {
    case "$INSTALL_METHOD" in
        binary)
            install_binary
            setup_path
            echo ""
            info "${BINARY_NAME} installed successfully"
            ;;
        git)
            install_git
            ;;
    esac
}

main
