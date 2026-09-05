#!/usr/bin/env python3
"""
AIHelper.py
-----------
A simple CLI program for asking questions across ChatGPT / Gemini / Claude /
Grok / DeepSeek / Sakana AI.

Key features:
  - Interactive mode keeps a separate conversation history per provider
  - --max-tokens / --temperature let you tune generation parameters
  - --stream enables streaming output (works in both interactive and
    single-question mode)
  - --list-models shows the models available for a provider
"""

import argparse
import os
import sys

PROVIDERS = ["chatgpt", "gemini", "claude", "grok", "deepseek", "sakana"]

DEFAULT_MODELS = {
    "chatgpt": "gpt-4o-mini",
    "gemini": "gemini-3.6-flash",
    "claude": "claude-haiku-4-5-20251001",
    "grok": "grok-4.6",
    "deepseek": "deepseek-v4-flash",
    "sakana": "sakana-namazu",
}

DEFAULT_MAX_TOKENS = 1024

# Configuration for providers that use an OpenAI-compatible API
# (Chat Completions). Only the base_url and the API key env var differ.
OPENAI_COMPATIBLE_CONFIG = {
    "chatgpt": {"api_key_env": "OPENAI_API_KEY", "base_url": None},
    "grok": {"api_key_env": "XAI_API_KEY", "base_url": "https://api.x.ai/v1"},
    "deepseek": {"api_key_env": "DEEPSEEK_API_KEY", "base_url": "https://api.deepseek.com/v1"},
    "sakana": {"api_key_env": "SAKANA_API_KEY", "base_url": "https://api.sakana.ai/v1"},
}


# --- Error message formatting (stateless, so this stays a module function) ---
def describe_error(provider: str, e: Exception) -> str:
    """Turn an SDK exception into a message that explains what likely went wrong.

    A RuntimeError (e.g. missing API key) is already clear enough on its
    own, so it's passed through as-is. For everything else, the OpenAI/
    Anthropic/Google SDKs use mostly consistent exception class names
    (AuthenticationError, RateLimitError, NotFoundError, etc.), so we
    guess the category from the class name and HTTP status code.
    """
    if isinstance(e, RuntimeError):
        return f"[{provider}] {e}"

    if isinstance(e, (ImportError, ModuleNotFoundError)):
        return f"[{provider}] Required library is not installed. Details: {e}"

    name = type(e).__name__
    status = getattr(e, "status_code", None) or getattr(e, "code", None)
    msg = str(e)

    def has(*keywords):
        return any(k in name for k in keywords) or (status and str(status) in keywords)

    if has("AuthenticationError", "PermissionDenied", "401", "403"):
        return f"[{provider}] Authentication error. Please check that your API key is correct. Details: {msg}"
    if has("RateLimitError", "429"):
        return f"[{provider}] Rate limit or quota exceeded. Please wait and try again. Details: {msg}"
    if has("NotFoundError", "404"):
        return f"[{provider}] Model or endpoint not found. Please check the model name (run --list-models {provider} to see what's available). Details: {msg}"
    if has("BadRequestError", "InvalidArgument", "400"):
        return f"[{provider}] There's a problem with the request (check parameters and model name). Details: {msg}"
    if has("APIConnectionError", "ConnectionError", "Timeout", "APITimeoutError"):
        return f"[{provider}] Connection error. Please check your network. Details: {msg}"
    if has("InternalServerError", "500", "503"):
        return f"[{provider}] Server-side error on the provider's end. Please try again later. Details: {msg}"

    return f"[{provider}] Error: {msg}"


def _build_gemini_config(temperature, max_tokens):
    """Build a Gemini generation config object only when needed."""
    if temperature is None and max_tokens is None:
        return None
    from google.genai import types

    kwargs = {}
    if temperature is not None:
        kwargs["temperature"] = temperature
    if max_tokens is not None:
        kwargs["max_output_tokens"] = max_tokens
    return types.GenerateContentConfig(**kwargs)


class AIClient:
    """Caches each provider's SDK client on a per-instance basis.

    Previously the Gemini client was managed as a module-level global
    singleton, which made it impossible to run multiple configurations
    (e.g. different API keys) side by side, and awkward to reuse in
    tests or as a library. Creating multiple instances of this class
    gives each one its own independent state.
    """

    def __init__(self):
        self._gemini_client = None
        self._claude_client = None
        self._openai_clients = {}  # provider -> OpenAI client

    # --- Client construction (cached per instance) ---
    def get_gemini_client(self):
        if self._gemini_client is None:
            from google import genai

            api_key = os.environ.get("GOOGLE_API_KEY")
            if not api_key:
                raise RuntimeError("Environment variable GOOGLE_API_KEY is not set.")
            self._gemini_client = genai.Client(api_key=api_key)
        return self._gemini_client

    def get_claude_client(self):
        if self._claude_client is None:
            import anthropic

            api_key = os.environ.get("ANTHROPIC_API_KEY")
            if not api_key:
                raise RuntimeError("Environment variable ANTHROPIC_API_KEY is not set.")
            self._claude_client = anthropic.Anthropic(api_key=api_key)
        return self._claude_client

    def get_openai_compatible_client(self, provider: str):
        if provider not in self._openai_clients:
            from openai import OpenAI

            config = OPENAI_COMPATIBLE_CONFIG[provider]
            api_key = os.environ.get(config["api_key_env"])
            if not api_key:
                raise RuntimeError(f"Environment variable {config['api_key_env']} is not set.")

            client_kwargs = {"api_key": api_key}
            if config["base_url"]:
                client_kwargs["base_url"] = config["base_url"]
            self._openai_clients[provider] = OpenAI(**client_kwargs)
        return self._openai_clients[provider]

    # --- Gemini ---
    def create_gemini_chat_session(self, model: str, temperature=None, max_tokens=None):
        """Create a Chat session for interactive mode. History is kept inside the session."""
        client = self.get_gemini_client()
        config = _build_gemini_config(temperature, max_tokens)
        if config is not None:
            return client.chats.create(model=model, config=config)
        return client.chats.create(model=model)

    def ask_gemini_single(self, prompt: str, model: str, max_tokens=None, temperature=None) -> str:
        session = self.create_gemini_chat_session(model, temperature, max_tokens)
        response = session.send_message(prompt)
        return response.text

    def ask_gemini_single_stream(self, prompt: str, model: str, max_tokens=None, temperature=None):
        session = self.create_gemini_chat_session(model, temperature, max_tokens)
        for chunk in session.send_message_stream(prompt):
            if chunk.text:
                yield chunk.text

    # --- Claude ---
    def ask_claude(self, messages: list, model: str, max_tokens=None, temperature=None) -> str:
        client = self.get_claude_client()
        kwargs = {"model": model, "max_tokens": max_tokens or DEFAULT_MAX_TOKENS, "messages": messages}
        if temperature is not None:
            kwargs["temperature"] = temperature
        response = client.messages.create(**kwargs)
        return "".join(block.text for block in response.content if block.type == "text")

    def ask_claude_stream(self, messages: list, model: str, max_tokens=None, temperature=None):
        client = self.get_claude_client()
        kwargs = {"model": model, "max_tokens": max_tokens or DEFAULT_MAX_TOKENS, "messages": messages}
        if temperature is not None:
            kwargs["temperature"] = temperature
        with client.messages.stream(**kwargs) as stream:
            for text in stream.text_stream:
                yield text

    # --- OpenAI-compatible (chatgpt / grok / deepseek / sakana) ---
    def ask_openai_compatible(self, provider: str, messages: list, model: str, max_tokens=None, temperature=None) -> str:
        client = self.get_openai_compatible_client(provider)
        kwargs = {"model": model, "messages": messages}
        if max_tokens is not None:
            kwargs["max_tokens"] = max_tokens
        if temperature is not None:
            kwargs["temperature"] = temperature
        response = client.chat.completions.create(**kwargs)
        return response.choices[0].message.content

    def ask_openai_compatible_stream(self, provider: str, messages: list, model: str, max_tokens=None, temperature=None):
        client = self.get_openai_compatible_client(provider)
        kwargs = {"model": model, "messages": messages, "stream": True}
        if max_tokens is not None:
            kwargs["max_tokens"] = max_tokens
        if temperature is not None:
            kwargs["temperature"] = temperature
        for chunk in client.chat.completions.create(**kwargs):
            delta = chunk.choices[0].delta.content
            if delta:
                yield delta

    # --- Single question (no history): backs --question / ask() ---
    def ask(self, provider: str, prompt: str, model: str = None, max_tokens=None, temperature=None) -> str:
        if provider not in PROVIDERS:
            raise ValueError(f"Unknown provider: {provider} (choices: {', '.join(PROVIDERS)})")
        model = model or DEFAULT_MODELS[provider]

        if provider == "gemini":
            return self.ask_gemini_single(prompt, model, max_tokens, temperature)
        elif provider == "claude":
            return self.ask_claude([{"role": "user", "content": prompt}], model, max_tokens, temperature)
        else:
            return self.ask_openai_compatible(provider, [{"role": "user", "content": prompt}], model, max_tokens, temperature)

    def ask_stream(self, provider: str, prompt: str, model: str = None, max_tokens=None, temperature=None):
        """Generator that streams the answer to a single question."""
        if provider not in PROVIDERS:
            raise ValueError(f"Unknown provider: {provider} (choices: {', '.join(PROVIDERS)})")
        model = model or DEFAULT_MODELS[provider]

        if provider == "gemini":
            yield from self.ask_gemini_single_stream(prompt, model, max_tokens, temperature)
        elif provider == "claude":
            yield from self.ask_claude_stream([{"role": "user", "content": prompt}], model, max_tokens, temperature)
        else:
            yield from self.ask_openai_compatible_stream(provider, [{"role": "user", "content": prompt}], model, max_tokens, temperature)

    # --- Model listing ---
    def list_models(self, provider: str) -> list:
        if provider == "gemini":
            client = self.get_gemini_client()
            return sorted(m.name.removeprefix("models/") for m in client.models.list())
        elif provider == "claude":
            client = self.get_claude_client()
            return sorted(m.id for m in client.models.list())
        elif provider in OPENAI_COMPATIBLE_CONFIG:
            client = self.get_openai_compatible_client(provider)
            return sorted(m.id for m in client.models.list())
        else:
            raise ValueError(f"Unknown provider: {provider} (choices: {', '.join(PROVIDERS)})")


def interactive_mode(client: AIClient, initial_provider: str, model_override: str = None,
                      max_tokens: int = None, temperature: float = None, stream: bool = False,
                      initial_question: str = None):
    provider = initial_provider

    # Track which model is currently in use for each provider.
    # --model (from the CLI) applies only to the initial provider;
    # every other provider starts from its own default model
    # (e.g. with --provider grok --model grok-4.6, it wouldn't make
    # sense for Claude to inherit Grok's model name after switching).
    current_models = dict(DEFAULT_MODELS)
    if model_override:
        current_models[initial_provider] = model_override

    print("=== AI Chat CLI ===")
    print(f"Available providers: {', '.join(PROVIDERS)}")
    print(f"Current provider: {provider} (model: {current_models[provider]})")
    if stream:
        print("(Streaming output: ON)")
    print("Type 'provider:gemini' (etc.) to switch providers.")
    print("Type 'model:model-name' to switch the current provider's model.")
    print("Type 'model' alone to show the current model.")
    print("Type 'reset' to clear the current provider's conversation history.")
    print("Type 'models' to list the models available for the current provider.")
    print("Type 'exit' or 'quit' to leave.\n")

    # Gemini's SDK Chat session keeps history internally.
    # For every other provider, we accumulate a list of messages ourselves.
    gemini_chat_session = None
    histories = {p: [] for p in PROVIDERS if p != "gemini"}

    def init_gemini_chat(model_name: str):
        try:
            return client.create_gemini_chat_session(model_name, temperature, max_tokens)
        except Exception as e:
            print(describe_error("gemini", e))
            return None

    if provider == "gemini":
        gemini_chat_session = init_gemini_chat(current_models["gemini"])

    def ask_once(user_input: str):
        """Send one question to the current provider/model and print the answer.

        Shared by the initial --question (if any) and every subsequent line
        read from the prompt, so both paths keep contributing to the same
        conversation history.
        """
        nonlocal gemini_chat_session
        try:
            current_model = current_models[provider]

            if provider == "gemini":
                if gemini_chat_session is None:
                    gemini_chat_session = init_gemini_chat(current_model)

                if stream:
                    print()
                    for chunk_text in gemini_chat_session.send_message_stream(user_input):
                        if chunk_text.text:
                            print(chunk_text.text, end="", flush=True)
                    print("\n")
                else:
                    response = gemini_chat_session.send_message(user_input)
                    print(f"\n{response.text}\n")
            else:
                history = histories[provider]
                history.append({"role": "user", "content": user_input})

                if stream:
                    print()
                    answer_parts = []
                    if provider == "claude":
                        gen = client.ask_claude_stream(history, current_model, max_tokens, temperature)
                    else:
                        gen = client.ask_openai_compatible_stream(provider, history, current_model, max_tokens, temperature)
                    for piece in gen:
                        print(piece, end="", flush=True)
                        answer_parts.append(piece)
                    print("\n")
                    history.append({"role": "assistant", "content": "".join(answer_parts)})
                else:
                    if provider == "claude":
                        answer = client.ask_claude(history, current_model, max_tokens, temperature)
                    else:
                        answer = client.ask_openai_compatible(provider, history, current_model, max_tokens, temperature)
                    history.append({"role": "assistant", "content": answer})
                    print(f"\n{answer}\n")

        except Exception as e:
            print(f"\n{describe_error(provider, e)}\n")

    # If a question was supplied on the command line (--question), answer it
    # first without exiting, then fall straight into the normal interactive
    # loop so the user can keep asking follow-up questions.
    if initial_question:
        print(f"[{provider} / {current_models[provider]}] > {initial_question}")
        ask_once(initial_question)

    while True:
        try:
            user_input = input(f"[{provider} / {current_models[provider]}] > ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nExiting.")
            break

        if not user_input:
            continue

        if user_input.lower() in ("exit", "quit"):
            print("Exiting.")
            break

        if user_input.lower() == "reset":
            if provider == "gemini":
                gemini_chat_session = init_gemini_chat(current_models["gemini"])
            else:
                histories[provider] = []
            print(f"-> Cleared conversation history for {provider}.\n")
            continue

        if user_input.lower() == "model":
            print(f"-> Current model ({provider}): {current_models[provider]}\n")
            continue

        if user_input.lower().startswith("model:"):
            new_model = user_input.split(":", 1)[1].strip()
            if not new_model:
                print("-> Please specify a model name (e.g. model:gpt-4o)\n")
                continue

            current_models[provider] = new_model
            print(f"-> Switched {provider}'s model to {new_model}.")

            if provider == "gemini":
                # Gemini ties the model to the session, so switching models
                # also resets the conversation history.
                gemini_chat_session = init_gemini_chat(new_model)
                print("   (Switching Gemini's model also resets its conversation history)")
            print()
            continue

        if user_input.lower() == "models":
            try:
                models = client.list_models(provider)
                print(f"-> Models available for {provider} ({len(models)}):")
                for m in models:
                    print(f"   - {m}")
                print()
            except Exception as e:
                print(f"\n{describe_error(provider, e)}\n")
            continue

        if user_input.lower().startswith("provider:"):
            new_provider = user_input.split(":", 1)[1].strip().lower()
            if new_provider in PROVIDERS:
                provider = new_provider
                print(f"-> Switched to provider {provider} (model: {current_models[provider]}).\n")

                if provider == "gemini" and gemini_chat_session is None:
                    gemini_chat_session = init_gemini_chat(current_models["gemini"])
            else:
                print(f"-> Unknown provider (choices: {', '.join(PROVIDERS)})\n")
            continue

        ask_once(user_input)


def main():
    parser = argparse.ArgumentParser(description="A CLI for switching between ChatGPT / Gemini / Claude / Grok / DeepSeek / Sakana AI")
    parser.add_argument(
        "--provider", "-p",
        choices=PROVIDERS,
        default="gemini",
        help="Which AI provider to use (default: gemini)",
    )
    parser.add_argument(
        "--model", "-m",
        default=None,
        help="Model name to use",
    )
    parser.add_argument(
        "--question", "-q",
        default=None,
        help="The question to ask",
    )
    parser.add_argument(
        "--max-tokens",
        type=int,
        default=None,
        help=f"Maximum output tokens (default: {DEFAULT_MAX_TOKENS})",
    )
    parser.add_argument(
        "--temperature",
        type=float,
        default=None,
        help="Sampling temperature (roughly 0.0-2.0; if unset, each provider's default is used)",
    )
    parser.add_argument(
        "--stream",
        action="store_true",
        help="Stream the response as it's generated",
    )
    parser.add_argument(
        "--list-models",
        nargs="?",
        const="all",
        default=None,
        metavar="PROVIDER",
        choices=PROVIDERS + ["all"],
        help="List available models and exit. Omit the provider to list all of them.",
    )
    args = parser.parse_args()

    client = AIClient()

    if args.list_models:
        targets = PROVIDERS if args.list_models == "all" else [args.list_models]
        exit_code = 0
        for p in targets:
            try:
                models = client.list_models(p)
                print(f"=== {p} ({len(models)}) ===")
                for m in models:
                    print(f"  - {m}")
            except Exception as e:
                print(describe_error(p, e), file=sys.stderr)
                exit_code = 1
        sys.exit(exit_code)

    # --question no longer exits the process after answering: it just
    # supplies the first question, and the program then keeps running so
    # further questions can be asked interactively.
    interactive_mode(client, args.provider, args.model, args.max_tokens, args.temperature, args.stream,
                      initial_question=args.question)


if __name__ == "__main__":
    main()
