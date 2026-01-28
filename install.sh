#!/usr/bin/env bash
set -e

INSTALL_DIR="${HOME}/.local/bin"
BINARY_NAME="gsr"

# Build
make

# Install binary
echo "Installing to ${INSTALL_DIR}..."
mkdir -p "${INSTALL_DIR}"
install -m 755 "${BINARY_NAME}" "${INSTALL_DIR}/${BINARY_NAME}"

# Add to PATH if needed
add_to_path() {
    local rc_file="$1"
    local line='export PATH="$HOME/.local/bin:$PATH"'

    if [ -f "$rc_file" ] && grep -q '\.local/bin' "$rc_file"; then
        return 1  # Already present
    fi

    echo "" >> "$rc_file"
    echo "# Added by ${BINARY_NAME} installer" >> "$rc_file"
    echo "$line" >> "$rc_file"
    return 0
}

if ! echo "$PATH" | grep -q "${INSTALL_DIR}"; then
    rc_file=""
    shell_name="$(basename "$SHELL")"

    case "$shell_name" in
        zsh)  rc_file="${HOME}/.zshrc" ;;
        bash) rc_file="${HOME}/.bashrc" ;;
        *)    rc_file="${HOME}/.profile" ;;
    esac

    if add_to_path "$rc_file"; then
        echo "Added ${INSTALL_DIR} to PATH in ${rc_file}"
        echo ""
        echo "Run this to use immediately:"
        echo "  source ${rc_file}"
        echo ""
        echo "Or restart your terminal."
    else
        echo "${INSTALL_DIR} already in PATH config."
    fi
else
    echo "${INSTALL_DIR} already in PATH."
fi

echo ""
echo "${BINARY_NAME} installed"
