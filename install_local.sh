#!/bin/bash
# install_local.sh - Install cvt_asm and IDE extensions locally
# Installs to ~/.local and copies extensions to Visual Studio and CLion if installed

set -e

INSTALL_PREFIX="$HOME/.local"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Installing cvt_asm to $INSTALL_PREFIX..."
make -C "$SCRIPT_DIR" install_local

echo ""
echo "Installing IDE extensions..."

# Visual Studio extensions
install_vscode_extensions() {
    echo ""
    echo "Checking for Visual Studio Code..."

    local vscode_ext_dir
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        vscode_ext_dir="$HOME/Library/Application Support/Code/extensions"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # Linux
        vscode_ext_dir="$HOME/.vscode/extensions"
    elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
        # Windows (Git Bash or Cygwin)
        vscode_ext_dir="$HOME/AppData/Roaming/Code/User/extensions"
    fi

    if [ -z "$vscode_ext_dir" ]; then
        echo "✗ Unsupported OS for VS Code extension installation"
        return 1
    fi

    if [ ! -d "$vscode_ext_dir" ]; then
        echo "  VS Code not found or extensions directory doesn't exist"
        return 1
    fi

    echo "✓ VS Code extensions directory found: $vscode_ext_dir"

    # Check if VS Code extensions exist in the project
    local vscode_ext_src="$SCRIPT_DIR/extensions/vscode"
    if [ ! -d "$vscode_ext_src" ]; then
        echo "  No VS Code extensions found in $vscode_ext_src"
        return 1
    fi

    # Copy extensions
    echo "  Installing VS Code extensions..."
    for ext_dir in "$vscode_ext_src"/*; do
        if [ -d "$ext_dir" ]; then
            local ext_name=$(basename "$ext_dir")
            echo "    Installing $ext_name..."
            mkdir -p "$vscode_ext_dir/$ext_name"
            cp -r "$ext_dir"/* "$vscode_ext_dir/$ext_name/" 2>/dev/null || true
        fi
    done

    echo "✓ VS Code extensions installed"
    return 0
}

# CLion extensions
install_clion_extensions() {
    echo ""
    echo "Checking for CLion..."

    local clion_ext_dir
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        clion_ext_dir="$HOME/Library/Application Support/JetBrains/CLion*/plugins"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # Linux
        clion_ext_dir="$HOME/.config/JetBrains/CLion*/plugins"
    elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
        # Windows
        clion_ext_dir="$HOME/AppData/Roaming/JetBrains/CLion*/plugins"
    fi

    if [ -z "$clion_ext_dir" ]; then
        echo "✗ Unsupported OS for CLion extension installation"
        return 1
    fi

    # Find actual CLion config directory (handles multiple versions)
    local actual_dir=$(ls -d $clion_ext_dir 2>/dev/null | head -1)
    if [ -z "$actual_dir" ]; then
        echo "  CLion not found"
        return 1
    fi

    echo "✓ CLion plugins directory found: $actual_dir"

    # Check if CLion extensions exist in the project
    local clion_ext_src="$SCRIPT_DIR/extensions/clion"
    if [ ! -d "$clion_ext_src" ]; then
        echo "  No CLion extensions found in $clion_ext_src"
        return 1
    fi

    # Copy extensions
    echo "  Installing CLion extensions..."
    for ext_dir in "$clion_ext_src"/*; do
        if [ -d "$ext_dir" ]; then
            local ext_name=$(basename "$ext_dir")
            echo "    Installing $ext_name..."
            mkdir -p "$actual_dir/$ext_name"
            cp -r "$ext_dir"/* "$actual_dir/$ext_name/" 2>/dev/null || true
        fi
    done

    echo "✓ CLion extensions installed"
    return 0
}

# JetBrains IDE extensions (generic for all JetBrains IDEs)
install_jetbrains_extensions() {
    echo ""
    echo "Checking for JetBrains IDEs..."

    local jetbrains_config_dir
    if [[ "$OSTYPE" == "darwin"* ]]; then
        jetbrains_config_dir="$HOME/Library/Application Support/JetBrains"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        jetbrains_config_dir="$HOME/.config/JetBrains"
    elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
        jetbrains_config_dir="$HOME/AppData/Roaming/JetBrains"
    fi

    if [ ! -d "$jetbrains_config_dir" ]; then
        echo "  No JetBrains IDEs found"
        return 1
    fi

    echo "✓ JetBrains config directory found: $jetbrains_config_dir"

    local jetbrains_ext_src="$SCRIPT_DIR/extensions/jetbrains"
    if [ ! -d "$jetbrains_ext_src" ]; then
        echo "  No JetBrains extensions found in $jetbrains_ext_src"
        return 1
    fi

    # Find all JetBrains IDE plugin directories
    local found_any=false
    for ide_dir in "$jetbrains_config_dir"/*; do
        if [ -d "$ide_dir/plugins" ]; then
            local ide_name=$(basename "$ide_dir")
            echo "  Found IDE: $ide_name"

            # Copy extensions to this IDE
            for ext_dir in "$jetbrains_ext_src"/*; do
                if [ -d "$ext_dir" ]; then
                    local ext_name=$(basename "$ext_dir")
                    echo "    Installing $ext_name to $ide_name..."
                    mkdir -p "$ide_dir/plugins/$ext_name"
                    cp -r "$ext_dir"/* "$ide_dir/plugins/$ext_name/" 2>/dev/null || true
                    found_any=true
                fi
            done
        fi
    done

    if $found_any; then
        echo "✓ JetBrains extensions installed"
        return 0
    else
        echo "  No JetBrains plugin directories found"
        return 1
    fi
}

# Main installation
echo "================================"
echo "cvt_asm Local Installation"
echo "================================"

# Try installing extensions (don't fail if any fail)
install_vscode_extensions || true
install_clion_extensions || true
install_jetbrains_extensions || true

echo ""
echo "================================"
echo "Installation Complete!"
echo "================================"
echo ""
echo "Summary:"
echo "  Binaries installed to: $INSTALL_PREFIX/bin"
echo "  Libraries installed to: $INSTALL_PREFIX/lib"
echo "  Headers installed to: $INSTALL_PREFIX/include"
echo ""
echo "IDE Extensions:"
echo "  Visual Studio Code: ~/.vscode/extensions (or per-OS equivalent)"
echo "  CLion: ~/.config/JetBrains/CLion*/plugins (or per-OS equivalent)"
echo "  Other JetBrains IDEs: ~/.config/JetBrains/*/plugins (or per-OS equivalent)"
echo ""
echo "To use the installed binaries, add to your PATH:"
echo "  export PATH=\"$INSTALL_PREFIX/bin:\$PATH\""
