import json
import urllib.request
import urllib.error
import sys
from .config import DEFAULT_PROVIDERS

def _build_payload(provider, model, prompt):
    if provider == "ollama":
        return {"model": model, "prompt": prompt, "stream": False}
    elif provider == "openai":
        return {
            "model": model,
            "messages": [{"role": "user", "content": prompt}],
            "temperature": 0.1
        }
    elif provider == "gemini":
        return {
            "contents": [{"parts": [{"text": prompt}]}],
            "generationConfig": {"temperature": 0.1}
        }
    elif provider == "anthropic":
        return {
            "model": model,
            "max_tokens": 4096,
            "messages": [{"role": "user", "content": prompt}],
            "temperature": 0.1
        }
    return {}

def _parse_response(provider, res_body):
    if provider == "ollama":
        return res_body.get("response", "")
    elif provider == "openai":
        return res_body["choices"][0]["message"]["content"]
    elif provider == "gemini":
        return res_body["candidates"][0]["content"]["parts"][0]["text"]
    elif provider == "anthropic":
        return res_body["content"][0]["text"]
    return ""

def query_ai(provider, model, prompt, api_key, custom_url=None):
    config = DEFAULT_PROVIDERS.get(provider)
    if not config:
        raise ValueError(f"Unknown provider: {provider}")

    url = custom_url or config["url"]
    headers = {"Content-Type": "application/json"}

    if provider != "ollama" and not api_key:
        raise ValueError(f"{provider.upper()}_API_KEY is required for this provider.")

    if provider == "openai":
        headers["Authorization"] = f"Bearer {api_key}"
    elif provider == "gemini":
        url = url.format(model=model) + f"?key={api_key}"
    elif provider == "anthropic":
        headers["x-api-key"] = api_key
        headers["anthropic-version"] = "2023-06-01"

    payload = _build_payload(provider, model, prompt)
    req = urllib.request.Request(
        url,
        data=json.dumps(payload).encode("utf-8"),
        headers=headers,
        method="POST"
    )

    try:
        with urllib.request.urlopen(req) as response:
            res_body = json.loads(response.read().decode("utf-8"))
            return _parse_response(provider, res_body)
    except urllib.error.HTTPError as e:
        err_msg = e.read().decode("utf-8")
        print(f"❌ API HTTP Error ({e.code}): {err_msg}")
        sys.exit(1)
    except Exception as e:
        print(f"❌ Request failed: {e}")
        sys.exit(1)