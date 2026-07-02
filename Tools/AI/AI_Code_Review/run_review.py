#!/usr/bin/env python3
# embedded_system/Tools/AI/run_review.py

import os
import sys
import json
import argparse
import subprocess
import urllib.request
from pathlib import Path

# Defaults
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
        "model": "gemini-2.5-flash",
    },
    "anthropic": {
        "url": "https://api.anthropic.com/v1/messages",
        "model": "claude-3-5-haiku-20241022",
    }
}

def get_git_diff():
    """Retrieves staged and unstaged git diff compared to HEAD from the repository root."""
    try:
        # Find the Git repository root directory
        repo_root = subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip()
        # Retrieve the diff from the repo root
        diff = subprocess.check_output(["git", "diff", "HEAD"], cwd=repo_root, text=True)
        return diff.strip()
    except subprocess.CalledProcessError as e:
        print(f"❌ Failed to get git diff: {e}")
        sys.exit(1)

def load_template():
    """Loads C++ review instructions template."""
    script_dir = Path(__file__).parent.absolute()
    # Go up one level to reach Tools/AI/ and then enter PROMPTS/
    template_path = script_dir.parent / "PROMPTS" / "AI_Repo_Review" / "review_template.md"
    
    if not template_path.exists():
        print(f"⚠️  Template not found at {template_path}. Using fallback system instructions.")
        return "Review the C++ diff for embedded constraints, bugs, and memory leaks. Direct style."
    
    with open(template_path, "r", encoding="utf-8") as f:
        return f.read()

def query_ai(provider, model, prompt, api_key, custom_url=None):
    """Zero-dependency HTTP requester supporting any major AI API provider."""
    config = DEFAULT_PROVIDERS.get(provider)
    url = custom_url or config["url"]
    headers = {"Content-Type": "application/json"}
    payload = {}

    if provider == "ollama":
        payload = {
            "model": model,
            "prompt": prompt,
            "stream": False
        }
    elif provider == "openai":
        if not api_key:
            raise ValueError("OPENAI_API_KEY environment variable is required.")
        headers["Authorization"] = f"Bearer {api_key}"
        payload = {
            "model": model,
            "messages": [{"role": "user", "content": prompt}],
            "temperature": 0.1
        }
    elif provider == "gemini":
        if not api_key:
            raise ValueError("GEMINI_API_KEY environment variable/parameter is required.")
        # Embed API Key in URL query parameter
        url = url.format(model=model) + f"?key={api_key}"
        payload = {
            "contents": [{"parts": [{"text": prompt}]}],
            "generationConfig": {"temperature": 0.1}
        }
    elif provider == "anthropic":
        if not api_key:
            raise ValueError("ANTHROPIC_API_KEY environment variable is required.")
        headers["x-api-key"] = api_key
        headers["anthropic-version"] = "2023-06-01"
        payload = {
            "model": model,
            "max_tokens": 4096,
            "messages": [{"role": "user", "content": prompt}],
            "temperature": 0.1
        }

    # Execute HTTP POST request
    req = urllib.request.Request(
        url,
        data=json.dumps(payload).encode("utf-8"),
        headers=headers,
        method="POST"
    )

    try:
        with urllib.request.urlopen(req) as response:
            res_body = json.loads(response.read().decode("utf-8"))
            
            # Parse responses based on target provider format
            if provider == "ollama":
                return res_body.get("response", "")
            elif provider == "openai":
                return res_body["choices"][0]["message"]["content"]
            elif provider == "gemini":
                return res_body["candidates"][0]["content"]["parts"][0]["text"]
            elif provider == "anthropic":
                return res_body["content"][0]["text"]
    except urllib.error.HTTPError as e:
        err_msg = e.read().decode("utf-8")
        print(f"❌ API HTTP Error ({e.code}): {err_msg}")
        sys.exit(1)
    except Exception as e:
        print(f"❌ Request failed: {e}")
        sys.exit(1)

def Parse_Args():
    parser = argparse.ArgumentParser(description="Multi-Provider AI C++ Code Reviewer")
    parser.add_argument(
        "--provider",
        choices=["ollama", "openai", "gemini", "anthropic"],
        default=os.environ.get("AI_PROVIDER", "ollama"),
        help="Target AI service provider (default: ollama)"
    )
    parser.add_argument(
        "--model",
        default=os.environ.get("AI_MODEL"),
        help="Specify LLM model (defaults to provider's mini model)"
    )
    parser.add_argument(
        "--key",
        default=os.environ.get("AI_API_KEY"),
        help="API Key (or set AI_API_KEY/GEMINI_API_KEY/OPENAI_API_KEY env variables)"
    )
    parser.add_argument(
        "--url",
        default=os.environ.get("AI_API_URL"),
        help="Override endpoint URL (e.g. for LM Studio, OpenRouter, custom proxy)"
    )
    return parser.parse_args()

def main():
    args = Parse_Args()

    # Determine Model
    provider = args.provider
    model = args.model or DEFAULT_PROVIDERS[provider]["model"]

    # Retrieve API Key
    api_key = args.key
    if not api_key:
        if provider == "openai":
            api_key = os.environ.get("OPENAI_API_KEY")
        elif provider == "gemini":
            api_key = os.environ.get("GEMINI_API_KEY")
        elif provider == "anthropic":
            api_key = os.environ.get("ANTHROPIC_API_KEY")

    # Fetch git changes & instructions
    diff = get_git_diff()
    if not diff:
        print("✅ No local C++ changes to review.")
        sys.exit(0)

    template = load_template()
    
    # Construct complete prompt
    prompt = f"{template}\n\n--- TARGET GIT DIFF FOR REVIEW ---\n```diff\n{diff}\n```"

    print(f"🚀 Running Code Review via {provider.upper()} using model: '{model}'...")
    review = query_ai(provider, model, prompt, api_key, args.url)
    
    print("\n" + "="*45)
    print("📝 AI CODE REVIEW REPORT")
    print("="*45)
    print(review)

if __name__ == "__main__":
    main()
