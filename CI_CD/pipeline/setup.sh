#!/usr/bin/env bash
set -e

echo "🚀 Setting up CI/CD Environment..."

# 1. OS Detection & Docker Installation
OS="$(uname -s)"
if [ "$OS" = "Linux" ]; then
    echo "🐧 Linux / WSL detected."
    if ! command -v docker &> /dev/null; then
        echo "🐳 Docker not found. Installing Docker..."
        # Official Docker automated install script
        curl -fsSL https://get.docker.com -o get-docker.sh
        sudo sh get-docker.sh
        sudo usermod -aG docker $USER
        rm get-docker.sh
        echo "⚠️  CRITICAL: Docker group added. You MUST log out and log back in, or run 'newgrp docker' to use Docker!"
    else
        echo "✅ Docker is already installed."
    fi
elif [ "$OS" = "Darwin" ]; then
    echo "🍎 macOS detected."
    if ! command -v docker &> /dev/null; then
        echo "🐳 Docker not found. Installing Docker via Homebrew..."
        if ! command -v brew &> /dev/null; then
            echo "❌ Homebrew not found. Please install Homebrew first: https://brew.sh/"
            exit 1
        fi
        brew install --cask docker
        echo "⚠️  Please open Docker Desktop from your Applications folder to finish setup!"
    else
        echo "✅ Docker is already installed."
    fi
else
    echo "⚠️ Unsupported OS for automated Docker install. Please install Docker manually."
fi

# 2. Python Virtual Environment Setup
echo "🐍 Setting up Python Virtual Environment..."
if[ ! -d ".venv" ]; then
    python3 -m venv .venv
fi

# Activate venv
source .venv/bin/activate

# Install requirements
echo "📦 Installing Python dependencies..."
python3 -m pip install --upgrade pip
pip install -r requirements.txt

echo ""
echo "🎉 Setup Complete! 🎉"
echo "👉 To activate the environment, run: source .venv/bin/activate"