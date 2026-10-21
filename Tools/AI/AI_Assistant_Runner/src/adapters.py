import os
from abc import ABC, abstractmethod
from typing import List


class BaseAdapter(ABC):
    name: str = "base"
    default_binary: str = ""

    def __init__(self, config: dict, provider_override: str | None = None):
        self.ai_cfg = config.get("ai_assistant", config)
        self.tool_config = self.ai_cfg.get("tools", {}).get(self.name, {})

        providers = self.ai_cfg.get("providers", {})

        # Priority: explicit override -> tool default -> first provider in list
        chosen_provider = (
            provider_override
            or self.tool_config.get("provider")
            or (next(iter(providers.keys())) if providers else None)
        )

        if chosen_provider and chosen_provider in providers:
            self.provider = providers[chosen_provider]
            self.provider_name = chosen_provider
        elif "provider" in self.ai_cfg:  # Backward compatibility
            self.provider = self.ai_cfg["provider"]
            self.provider_name = "default"
        else:
            self.provider = {}
            self.provider_name = "unknown"

    def resolve_api_key(self, env: dict) -> str:
        if self.provider.get("api_key"):
            return str(self.provider["api_key"]).strip()

        raw_val = str(self.provider.get("api_key_env_var", "")).strip()
        if raw_val:
            if raw_val.startswith("sk-") or ("-" in raw_val and raw_val not in env):
                return raw_val
            if raw_val in env:
                return env[raw_val]

        return (
            env.get("OPEN_ROUTER_API_KEY")
            or env.get("ANTHROPIC_API_KEY")
            or env.get("OPENAI_API_KEY")
            or ""
        )

    def clean_environment(self, env: dict) -> None:
        for var in self.tool_config.get("clean_env", []):
            env.pop(var, None)

    def get_binary(self) -> str:
        return self.tool_config.get("binary") or self.default_binary

    def get_args(self) -> List[str]:
        raw_args = self.tool_config.get("args", [])
        model = self.provider.get("model", "")
        return [arg.replace("{ai_assistant.provider.model}", model) for arg in raw_args]

    @abstractmethod
    def configure_env(self, env: dict) -> dict:
        pass


class CopilotAdapter(BaseAdapter):
    name = "copilot"
    default_binary = "copilot"

    def configure_env(self, env: dict) -> dict:
        self.clean_environment(env)
        env["COPILOT_PROVIDER_BASE_URL"] = self.provider.get(
            "base_url", "https://openrouter.ai/api/v1"
        )
        env["COPILOT_PROVIDER_TYPE"] = self.provider.get("provider_type", "openai")
        env["COPILOT_MODEL"] = self.provider.get("model", "openrouter/free")

        api_key = self.resolve_api_key(env)
        if api_key:
            env["COPILOT_PROVIDER_API_KEY"] = api_key
            env["OPEN_ROUTER_API_KEY"] = api_key
        return env


class ClaudeAdapter(BaseAdapter):
    name = "claude"
    default_binary = "claude"

    def configure_env(self, env: dict) -> dict:
        self.clean_environment(env)
        env["ANTHROPIC_BASE_URL"] = self.provider.get(
            "base_url", "https://openrouter.ai/api/v1"
        )
        model = self.provider.get("model")
        if model:
            env["ANTHROPIC_MODEL"] = model

        api_key = self.resolve_api_key(env)
        if api_key:
            env["ANTHROPIC_API_KEY"] = api_key
            env["ANTHROPIC_AUTH_TOKEN"] = api_key
        return env


class AntigravityAdapter(BaseAdapter):
    name = "antigravity"
    default_binary = "antigravity"

    def configure_env(self, env: dict) -> dict:
        self.clean_environment(env)
        env["OPENAI_API_BASE"] = self.provider.get(
            "base_url", "https://openrouter.ai/api/v1"
        )
        env["MODEL"] = self.provider.get("model", "openrouter/free")

        api_key = self.resolve_api_key(env)
        if api_key:
            env["OPENAI_API_KEY"] = api_key
        return env


class GenericAdapter(BaseAdapter):
    name = "generic"
    default_binary = "aider"

    def configure_env(self, env: dict) -> dict:
        self.clean_environment(env)
        api_key = self.resolve_api_key(env)
        if api_key:
            env["OPENROUTER_API_KEY"] = api_key
            env["OPENAI_API_KEY"] = api_key
        env["OPENAI_API_BASE"] = self.provider.get("base_url", "")
        return env


ADAPTERS = {
    "copilot": CopilotAdapter,
    "claude": ClaudeAdapter,
    "antigravity": AntigravityAdapter,
    "generic": GenericAdapter,
}
