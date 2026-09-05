#!/usr/bin/env python3
"""
AIHelper_ja.py
--------------
ChatGPT / Gemini / Claude / Grok / DeepSeek / Sakana AI を切り替えて
質問できるシンプルなCLIプログラム。

主な機能:
  - 対話モードでは各プロバイダごとに会話履歴を保持
  - --max-tokens / --temperature で生成パラメータを調整可能
  - --stream でストリーミング出力（対話モード・単発質問の両方に対応）
  - --list-models で利用可能なモデル一覧を表示
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

# OpenAI 互換API（Chat Completions）を使うプロバイダの設定。
# base_url とAPIキーの環境変数名だけが異なる。
OPENAI_COMPATIBLE_CONFIG = {
    "chatgpt": {"api_key_env": "OPENAI_API_KEY", "base_url": None},
    "grok": {"api_key_env": "XAI_API_KEY", "base_url": "https://api.x.ai/v1"},
    "deepseek": {"api_key_env": "DEEPSEEK_API_KEY", "base_url": "https://api.deepseek.com/v1"},
    "sakana": {"api_key_env": "SAKANA_API_KEY", "base_url": "https://api.sakana.ai/v1"},
}


# --- エラーメッセージの整形（状態を持たないのでモジュール関数のまま） ---
def describe_error(provider: str, e: Exception) -> str:
    """SDK例外の種類に応じて、原因が分かりやすいメッセージを組み立てる。

    APIキー未設定などのRuntimeErrorはそのまま十分わかりやすいので通す。
    それ以外はOpenAI/Anthropic/Google の各SDKで例外クラス名の付け方が
    ほぼ共通（AuthenticationError, RateLimitError, NotFoundError など）
    なので、クラス名とHTTPステータスコードから種別を推測する。
    """
    if isinstance(e, RuntimeError):
        return f"[{provider}] {e}"

    if isinstance(e, (ImportError, ModuleNotFoundError)):
        return f"[{provider}] 必要なライブラリがインストールされていません。詳細: {e}"

    name = type(e).__name__
    status = getattr(e, "status_code", None) or getattr(e, "code", None)
    msg = str(e)

    def has(*keywords):
        return any(k in name for k in keywords) or (status and str(status) in keywords)

    if has("AuthenticationError", "PermissionDenied", "401", "403"):
        return f"[{provider}] 認証エラーです。APIキーが正しいか確認してください。詳細: {msg}"
    if has("RateLimitError", "429"):
        return f"[{provider}] レート制限またはクォータ超過です。しばらく待って再試行してください。詳細: {msg}"
    if has("NotFoundError", "404"):
        return f"[{provider}] モデルまたはエンドポイントが見つかりません。モデル名を確認してください（--list-models {provider} で一覧表示できます）。詳細: {msg}"
    if has("BadRequestError", "InvalidArgument", "400"):
        return f"[{provider}] リクエスト内容に問題があります（パラメータやモデル名を確認してください）。詳細: {msg}"
    if has("APIConnectionError", "ConnectionError", "Timeout", "APITimeoutError"):
        return f"[{provider}] 接続エラーです。ネットワーク状況を確認してください。詳細: {msg}"
    if has("InternalServerError", "500", "503"):
        return f"[{provider}] プロバイダ側のサーバーエラーです。しばらくしてから再試行してください。詳細: {msg}"

    return f"[{provider}] エラー: {msg}"


def _build_gemini_config(temperature, max_tokens):
    """Gemini用の生成設定オブジェクトを必要な時だけ作る。"""
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
    """各プロバイダのSDKクライアントをインスタンス単位でキャッシュするクラス。

    以前はモジュールレベルのグローバル変数でGeminiクライアントを
    シングルトン管理していたが、それだと複数の設定（別APIキーなど）を
    同時に扱えず、テストやライブラリとしての再利用もしづらい。
    このクラスのインスタンスを複数作れば、それぞれ独立した状態を持てる。
    """

    def __init__(self):
        self._gemini_client = None
        self._claude_client = None
        self._openai_clients = {}  # provider -> OpenAI クライアント

    # --- クライアント生成（各インスタンス内でキャッシュ） ---
    def get_gemini_client(self):
        if self._gemini_client is None:
            from google import genai

            api_key = os.environ.get("GOOGLE_API_KEY")
            if not api_key:
                raise RuntimeError("環境変数 GOOGLE_API_KEY が設定されていません。")
            self._gemini_client = genai.Client(api_key=api_key)
        return self._gemini_client

    def get_claude_client(self):
        if self._claude_client is None:
            import anthropic

            api_key = os.environ.get("ANTHROPIC_API_KEY")
            if not api_key:
                raise RuntimeError("環境変数 ANTHROPIC_API_KEY が設定されていません。")
            self._claude_client = anthropic.Anthropic(api_key=api_key)
        return self._claude_client

    def get_openai_compatible_client(self, provider: str):
        if provider not in self._openai_clients:
            from openai import OpenAI

            config = OPENAI_COMPATIBLE_CONFIG[provider]
            api_key = os.environ.get(config["api_key_env"])
            if not api_key:
                raise RuntimeError(f"環境変数 {config['api_key_env']} が設定されていません。")

            client_kwargs = {"api_key": api_key}
            if config["base_url"]:
                client_kwargs["base_url"] = config["base_url"]
            self._openai_clients[provider] = OpenAI(**client_kwargs)
        return self._openai_clients[provider]

    # --- Gemini ---
    def create_gemini_chat_session(self, model: str, temperature=None, max_tokens=None):
        """対話モード用 Chat セッション生成。履歴はセッション内部で保持される。"""
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

    # --- OpenAI 互換 (chatgpt / grok / deepseek / sakana) ---
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

    # --- 単発質問（履歴なし）: --question / ask() 互換 ---
    def ask(self, provider: str, prompt: str, model: str = None, max_tokens=None, temperature=None) -> str:
        if provider not in PROVIDERS:
            raise ValueError(f"不明なプロバイダです: {provider}（選択肢: {', '.join(PROVIDERS)}）")
        model = model or DEFAULT_MODELS[provider]

        if provider == "gemini":
            return self.ask_gemini_single(prompt, model, max_tokens, temperature)
        elif provider == "claude":
            return self.ask_claude([{"role": "user", "content": prompt}], model, max_tokens, temperature)
        else:
            return self.ask_openai_compatible(provider, [{"role": "user", "content": prompt}], model, max_tokens, temperature)

    def ask_stream(self, provider: str, prompt: str, model: str = None, max_tokens=None, temperature=None):
        """単発質問をストリーミングで返すジェネレータ。"""
        if provider not in PROVIDERS:
            raise ValueError(f"不明なプロバイダです: {provider}（選択肢: {', '.join(PROVIDERS)}）")
        model = model or DEFAULT_MODELS[provider]

        if provider == "gemini":
            yield from self.ask_gemini_single_stream(prompt, model, max_tokens, temperature)
        elif provider == "claude":
            yield from self.ask_claude_stream([{"role": "user", "content": prompt}], model, max_tokens, temperature)
        else:
            yield from self.ask_openai_compatible_stream(provider, [{"role": "user", "content": prompt}], model, max_tokens, temperature)

    # --- モデル一覧 ---
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
            raise ValueError(f"不明なプロバイダです: {provider}（選択肢: {', '.join(PROVIDERS)}）")


def interactive_mode(client: AIClient, initial_provider: str, model_override: str = None,
                      max_tokens: int = None, temperature: float = None, stream: bool = False,
                      initial_question: str = None):
    provider = initial_provider

    # プロバイダごとに現在使用中のモデルを保持する。
    # --model はCLI起動時に指定された初期プロバイダにのみ適用し、
    # それ以外のプロバイダはデフォルトモデルから始める
    # （例えば --provider grok --model grok-4.6 のとき、
    #  claude に切り替えたらClaudeのモデル名が必要になるのは不自然なため）。
    current_models = dict(DEFAULT_MODELS)
    if model_override:
        current_models[initial_provider] = model_override

    print("=== 生成AI 質問プログラム ===")
    print(f"利用可能なプロバイダ: {', '.join(PROVIDERS)}")
    print(f"現在のプロバイダ: {provider}（モデル: {current_models[provider]}）")
    if stream:
        print("(ストリーミング出力: ON)")
    print("『provider:gemini』のように入力するとプロバイダを切り替えられます。")
    print("『model:モデル名』のように入力すると現在のプロバイダのモデルを切り替えられます。")
    print("『model』だけ入力すると現在のモデルを表示します。")
    print("『reset』でその時点のプロバイダの会話履歴をクリアします。")
    print("『models』で現在のプロバイダの利用可能モデル一覧を表示します。")
    print("『exit』または『quit』で終了します。\n")

    # Gemini はSDKのChatセッションが履歴を保持する。
    # それ以外のプロバイダは messages のリストを自前で蓄積する。
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
        """現在のプロバイダ/モデルへ1回分の質問を送って回答を表示する。

        起動時の --question（あれば）と、その後プロンプトから入力される
        質問の両方から呼ばれる共通処理。こうすることで、両方が同じ会話
        履歴に積み重なっていく。
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

    # --question が指定されていた場合は、まずそれに回答してから
    # プロセスを終了せず、そのまま通常の対話ループへ入り、
    # 続けて質問できるようにする。
    if initial_question:
        print(f"[{provider} / {current_models[provider]}] 質問> {initial_question}")
        ask_once(initial_question)

    while True:
        try:
            user_input = input(f"[{provider} / {current_models[provider]}] 質問> ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\n終了します。")
            break

        if not user_input:
            continue

        if user_input.lower() in ("exit", "quit"):
            print("終了します。")
            break

        if user_input.lower() == "reset":
            if provider == "gemini":
                gemini_chat_session = init_gemini_chat(current_models["gemini"])
            else:
                histories[provider] = []
            print(f"→ {provider} の会話履歴をクリアしました。\n")
            continue

        if user_input.lower() == "model":
            print(f"→ 現在のモデル（{provider}）: {current_models[provider]}\n")
            continue

        if user_input.lower().startswith("model:"):
            new_model = user_input.split(":", 1)[1].strip()
            if not new_model:
                print("→ モデル名を指定してください（例: model:gpt-4o）\n")
                continue

            current_models[provider] = new_model
            print(f"→ {provider} のモデルを {new_model} に切り替えました。")

            if provider == "gemini":
                # Geminiはモデルとセッションが結びついているため、
                # モデルを切り替えると会話履歴もリセットされる。
                gemini_chat_session = init_gemini_chat(new_model)
                print("  （Geminiはモデル切り替えに伴い会話履歴もリセットされます）")
            print()
            continue

        if user_input.lower() == "models":
            try:
                models = client.list_models(provider)
                print(f"→ {provider} で利用可能なモデル ({len(models)}件):")
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
                print(f"→ プロバイダを {provider} に切り替えました（モデル: {current_models[provider]}）。\n")

                if provider == "gemini" and gemini_chat_session is None:
                    gemini_chat_session = init_gemini_chat(current_models["gemini"])
            else:
                print(f"→ 不明なプロバイダです（選択肢: {', '.join(PROVIDERS)}）\n")
            continue

        ask_once(user_input)


def main():
    parser = argparse.ArgumentParser(description="ChatGPT / Gemini / Claude / Grok / DeepSeek / Sakana AI を切り替えて質問できるプログラム")
    parser.add_argument(
        "--provider", "-p",
        choices=PROVIDERS,
        default="gemini",
        help="使用する生成AI（デフォルト: gemini）",
    )
    parser.add_argument(
        "--model", "-m",
        default=None,
        help="使用するモデル名",
    )
    parser.add_argument(
        "--question", "-q",
        default=None,
        help="質問文",
    )
    parser.add_argument(
        "--max-tokens",
        type=int,
        default=None,
        help=f"最大出力トークン数（デフォルト: {DEFAULT_MAX_TOKENS}）",
    )
    parser.add_argument(
        "--temperature",
        type=float,
        default=None,
        help="生成の温度（0.0〜2.0程度、指定しない場合は各プロバイダのデフォルト）",
    )
    parser.add_argument(
        "--stream",
        action="store_true",
        help="応答をストリーミング（逐次）表示する",
    )
    parser.add_argument(
        "--list-models",
        nargs="?",
        const="all",
        default=None,
        metavar="PROVIDER",
        choices=PROVIDERS + ["all"],
        help="利用可能なモデル一覧を表示して終了。プロバイダ名を省略すると全プロバイダ分を表示。",
    )
    args = parser.parse_args()

    client = AIClient()

    if args.list_models:
        targets = PROVIDERS if args.list_models == "all" else [args.list_models]
        exit_code = 0
        for p in targets:
            try:
                models = client.list_models(p)
                print(f"=== {p} ({len(models)}件) ===")
                for m in models:
                    print(f"  - {m}")
            except Exception as e:
                print(describe_error(p, e), file=sys.stderr)
                exit_code = 1
        sys.exit(exit_code)

    # --question を指定してもそこでプロセスを終了しない。
    # 最初の質問として使い、その後はそのまま対話モードへ入って
    # 続けて質問できるようにする。
    interactive_mode(client, args.provider, args.model, args.max_tokens, args.temperature, args.stream,
                      initial_question=args.question)


if __name__ == "__main__":
    main()
