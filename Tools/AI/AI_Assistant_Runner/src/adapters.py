import os
from abc import ABC, abstractmethod
from typing import List

API_KEY_FALLBACK_ENV_VARS = (
    "OPEN_ROUTER_API_KEY",
    "ANTHROPIC_API_KEY",
    "OPENAI_API_KEY",
)


class BaseAdapter(ABC):
    name: str = "base"
    default_binary: str = ""
    api_key_target_vars: tuple[str, ...] = ()

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

        provider_tool_config = self.provider.get("tools", {}).get(self.name, {})
        self.tool_config = {**self.tool_config, **provider_tool_config}

    def resolve_api_key(self, env: dict) -> str:
        if self.provider.get("api_key"):
            return str(self.provider["api_key"]).strip()

        raw_val = str(self.provider.get("api_key_env_var", "")).strip()
        if raw_val:
            if raw_val in env:
                return env[raw_val]
            if raw_val.startswith("sk-") or "-" in raw_val:
                return raw_val

        for var_name in self.api_key_env_vars():
            if env.get(var_name):
                return env[var_name]
        return ""

    def api_key_env_vars(self) -> tuple[str, ...]:
        raw_val = str(self.provider.get("api_key_env_var", "")).strip()
        if raw_val and (raw_val.startswith("sk-") or "-" in raw_val):
            return ()
        return tuple(
            dict.fromkeys(
                ([raw_val] if raw_val else []) + list(API_KEY_FALLBACK_ENV_VARS)
            )
        )

    def has_inline_api_key(self) -> bool:
        raw_val = str(self.provider.get("api_key_env_var", "")).strip()
        return bool(
            self.provider.get("api_key") or raw_val.startswith("sk-") or "-" in raw_val
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
    api_key_target_vars = ("COPILOT_PROVIDER_API_KEY", "OPEN_ROUTER_API_KEY")

    def configure_env(self, env: dict) -> dict:
        self.clean_environment(env)
        if self.provider.get("native", False):
            for var in (
                "COPILOT_PROVIDER_BASE_URL",
                "COPILOT_PROVIDER_TYPE",
                "COPILOT_PROVIDER_API_KEY",
                "COPILOT_MODEL",
                "OPEN_ROUTER_API_KEY",
            ):
                env.pop(var, None)
            return env

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
    api_key_target_vars = ("ANTHROPIC_API_KEY", "ANTHROPIC_AUTH_TOKEN")

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
    api_key_target_vars = ("OPENAI_API_KEY",)

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
    api_key_target_vars = ("OPENROUTER_API_KEY", "OPENAI_API_KEY")

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
