# setup.ps1
Write-Host "🚀 Setting up CI/CD Environment for Windows..." -ForegroundColor Cyan

# 1. Check and Install Docker
if (-not (Get-Command "docker" -ErrorAction SilentlyContinue)) {
    Write-Host "🐳 Docker not found. Installing Docker Desktop via Winget..." -ForegroundColor Yellow
    winget install Docker.DockerDesktop --accept-package-agreements --accept-source-agreements
    Write-Host "⚠️ Docker Desktop installed! Please start Docker Desktop from your start menu." -ForegroundColor Red
} else {
    Write-Host "✅ Docker is already installed." -ForegroundColor Green
}

# 2. Check and Install Python
if (-not (Get-Command "python" -ErrorAction SilentlyContinue)) {
    Write-Host "🐍 Python not found. Installing Python 3 via Winget..." -ForegroundColor Yellow
    winget install Python.Python.3.12 --accept-package-agreements --accept-source-agreements
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
} else {
    Write-Host "✅ Python is already installed." -ForegroundColor Green
}

# 3. Python Virtual Environment Setup
Write-Host "🐍 Setting up Python Virtual Environment..." -ForegroundColor Cyan
if (-not (Test-Path ".venv")) {
    python -m venv .venv
}

# Activate venv
.venv\Scripts\Activate.ps1

# Install requirements
Write-Host "📦 Installing Python dependencies..." -ForegroundColor Cyan
python -m pip install --upgrade pip
pip install -r requirements.txt

Write-Host "`n🎉 Setup Complete! 🎉" -ForegroundColor Green
Write-Host "👉 To activate the environment, run: .venv\Scripts\Activate.ps1" -ForegroundColor Yellow