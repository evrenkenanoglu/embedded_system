import os
import json
import argparse
from pathlib import Path

DEFAULT_PROVIDERS = {
    "ollama": {
        "url": "http://localhost:11434/api/generate",
        "model": "qwen2.5-coder:1.5b",
    },
    "openai": {
        "url": "https://api.openai.com/v1/chat/completions",
        "model": "gpt-4o-mini",
    },
    "gemini": {
        "url": "https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent",
        "model": "gemini-3.5-flash",  # Updated from gemini-2.5-flash
    },
    "anthropic": {
        "url": "https://api.anthropic.com/v1/messages",
        "model": "claude-3-5-haiku-20241022",
    }
}

def load_auth_config():
    """Attempts to read the local authentication.json file."""
    project_root = Path(__file__).parent.parent.absolute()
    auth_file = project_root / "authentication.json"
    if auth_file.exists():
        try:
            with open(auth_file, "r", encoding="utf-8") as f:
                return json.load(f)
        except json.JSONDecodeError as e:
            print(f"⚠️ Warning: Failed to parse authentication.json ({e}).")
    return {}

def parse_args():
    parser = argparse.ArgumentParser(description="Modular AI Code Reviewer")
    parser.add_argument(
        "--provider",
        choices=["ollama", "openai", "gemini", "anthropic"],
        default=os.environ.get("AI_PROVIDER", "ollama"),
        help="Target AI service provider (default: ollama)"
    )
    parser.add_argument(
        "--model",
        default=os.environ.get("AI_MODEL"),
        help="Specify LLM model"
    )
    parser.add_argument(
        "--key",
        help="Explicit API Key (overrides authentication.json and env variables)"
    )
    parser.add_argument(
        "--url",
        default=os.environ.get("AI_API_URL"),
        help="Override endpoint URL"
    )
    parser.add_argument(
        "--templates",
        nargs="+",
        default=["cpp_review"],
        help="One or more prompt templates from prompts/ directory (without .md)"
    )
    parser.add_argument(
        "--report-template",
        default="report_template",
        help="Specify the output formatting template name from prompts/ (without .md)"
    )
    parser.add_argument(
        "--files",
        nargs="+",
        help="Specific files to review directly (bypasses git diff if provided)"
    )
    parser.add_argument(
        "--format",
        choices=["markdown", "json"],
        default="markdown",
        help="Output report format (default: markdown)"
    )
    parser.add_argument(
        "--output-file",
        help="Path to save the generated review report"
    )
    return parser.parse_args()

def get_api_key(provider, cli_key):
    """Retrieves API key with priority: CLI Arg > authentication.json > Environment Var."""
    if cli_key:
        return cli_key

    auth_config = load_auth_config()
    if isinstance(auth_config, dict) and provider in auth_config:
        return auth_config[provider]

    env_vars = {
        "openai": "OPENAI_API_KEY",
        "gemini": "GEMINI_API_KEY",
        "anthropic": "ANTHROPIC_API_KEY"
    }
    return os.environ.get(env_vars.get(provider, ""))