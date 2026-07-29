#!/usr/bin/env python3
"""
gen_web_assets.py

Gera os headers C++ (PROGMEM, gzip) da interface web a partir dos
arquivos-fonte legiveis em web_src/. Nao depende de nenhuma biblioteca
externa (so a stdlib do Python 3), entao roda em qualquer maquina sem
instalar nada.

Uso:
    python3 tools/gen_web_assets.py

Rode este script sempre que editar algo em web_src/. Os headers gerados
ficam em include/Web/ e sao os arquivos que o firmware realmente usa
-- nao edite os headers gerados a mao, edite o web_src/ e rode o script
de novo.

Como funciona:
  1. Le style.css, script.js, shell.html e cada fragmento em pages/.
  2. Minifica de forma conservadora (so espacos/comentarios, nunca
     reescreve logica) -- CSS e HTML sao encolhidos de verdade; o JS
     so passa por uma limpeza leve, o grosso da compressao dele fica
     por conta do gzip mesmo.
  3. Calcula um hash curto do CSS/JS finais e substitui os
     placeholders __CSS_VER__/__JS_VER__ do shell.html, para o
     navegador so baixar de novo quando o conteudo realmente mudou
     (cache-busting automatico).
  4. Comprime cada asset final com gzip (nivel maximo, sem timestamp
     para o resultado ser sempre igual em builds diferentes).
  5. Escreve os arrays de bytes em PROGMEM dentro dos headers em
     include/Web/.
"""

import gzip
import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
WEB_SRC = ROOT / "web_src"
INCLUDE_WEB = ROOT / "Web"

BANNER = (
    "// ============================================================\n"
    "// ARQUIVO GERADO AUTOMATICAMENTE por tools/gen_web_assets.py\n"
    "// NAO EDITE ESTE ARQUIVO A MAO -- edite o correspondente em\n"
    "// web_src/ e rode: python3 tools/gen_web_assets.py\n"
    "// ============================================================\n"
)


def minify_css(css: str) -> str:
    css = re.sub(r"/\*.*?\*/", "", css, flags=re.S)          # comentarios
    css = re.sub(r"\s+", " ", css)                            # colapsa espacos
    css = re.sub(r"\s*([{}:;,])\s*", r"\1", css)               # espacos junto a pontuacao
    css = re.sub(r";}", "}", css)                              # ; final antes de }
    return css.strip()


def minify_html(html: str) -> str:
    html = re.sub(r"<!--.*?-->", "", html, flags=re.S)         # comentarios
    html = re.sub(r">\s+<", "><", html)                        # espaco entre tags
    html = re.sub(r"\s+", " ", html)                           # colapsa espacos
    return html.strip()


def light_clean_js(js: str) -> str:
    # Minificacao de JS de verdade tem risco de quebrar o codigo (ASI,
    # strings, regex...). Aqui so tiramos linhas em branco e espacos
    # nas pontas de cada linha -- 100% seguro. O gzip cuida do resto.
    lines = [ln.strip() for ln in js.splitlines()]
    lines = [ln for ln in lines if ln != ""]
    return "\n".join(lines) + "\n"


def gzip_bytes(data: bytes) -> bytes:
    return gzip.compress(data, compresslevel=9, mtime=0)


def short_hash(data: bytes) -> str:
    return hashlib.sha1(data).hexdigest()[:8]


def to_c_array(data: bytes, var_name: str) -> str:
    rows = []
    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        rows.append(", ".join(f"0x{b:02x}" for b in chunk))
    body = ",\n    ".join(rows)
    return (
        f"inline constexpr uint8_t {var_name}[] PROGMEM = {{\n"
        f"    {body}\n"
        f"}};\n"
        f"inline constexpr size_t {var_name}_LEN = sizeof({var_name});\n"
    )


def write_header(path: Path, namespace: str, includes: str, body: str):
    path.parent.mkdir(parents=True, exist_ok=True)
    content = (
        BANNER
        + "#pragma once\n\n"
        + includes
        + f"\nnamespace {namespace}\n{{\n\n"
        + body
        + "\n}\n"
    )
    path.write_text(content, encoding="utf-8")


def main():
    if not WEB_SRC.exists():
        sys.exit(f"web_src/ nao encontrado em {WEB_SRC}")

    # ---- CSS ----
    css_src = (WEB_SRC / "style.css").read_text(encoding="utf-8")
    css_min = minify_css(css_src)
    css_gz = gzip_bytes(css_min.encode("utf-8"))
    css_ver = short_hash(css_min.encode("utf-8"))

    write_header(
        INCLUDE_WEB / "Style" / "WebStyle.h",
        "Web::Style",
        "#include <Arduino.h>\n",
        to_c_array(css_gz, "CSS_GZ"),
    )

    # ---- JS ----
    js_src = (WEB_SRC / "script.js").read_text(encoding="utf-8")
    js_min = light_clean_js(js_src)
    js_gz = gzip_bytes(js_min.encode("utf-8"))
    js_ver = short_hash(js_min.encode("utf-8"))

    write_header(
        INCLUDE_WEB / "Script" / "WebScript.h",
        "Web::Script",
        "#include <Arduino.h>\n",
        to_c_array(js_gz, "JS_GZ"),
    )

    # ---- Shell (index) ----
    shell_src = (WEB_SRC / "shell.html").read_text(encoding="utf-8")
    shell_src = shell_src.replace("__CSS_VER__", css_ver).replace("__JS_VER__", js_ver)
    shell_min = minify_html(shell_src)
    shell_gz = gzip_bytes(shell_min.encode("utf-8"))

    write_header(
        INCLUDE_WEB / "Shell.h",
        "Web::Shell",
        "#include <Arduino.h>\n",
        to_c_array(shell_gz, "PAGE_GZ"),
    )

    # ---- Fragmentos de pagina ----
    pages = {
        "status.html": ("StatusPage.h", "STATUS_PAGE_GZ"),
        "offsets.html": ("OffsetPage.h", "OFFSET_PAGE_GZ"),
        "pid.html": ("PidPage.h", "PID_PAGE_GZ"),
        "system.html": ("SystemPage.h", "SYSTEM_PAGE_GZ"),
        "lora.html": ("LoraPage.h", "LORA_PAGE_GZ"),
        "flight.html": ("FlightPage.h", "FLIGHT_PAGE_GZ"),
        "logs.html":("LogsPage.h", "LOG_PAGE_GZ"),
        "console.html":("ConsolePage.h", "CONSOLE_PAGE_GZ")
    }

    total_gz = len(css_gz) + len(js_gz) + len(shell_gz)

    for src_name, (header_name, var_name) in pages.items():
        html_src = (WEB_SRC / "pages" / src_name).read_text(encoding="utf-8")
        html_min = minify_html(html_src)
        html_gz = gzip_bytes(html_min.encode("utf-8"))
        total_gz += len(html_gz)

        write_header(
            INCLUDE_WEB / "Pages" / header_name,
            "Web::Pages",
            "#include <Arduino.h>\n",
            to_c_array(html_gz, var_name),
        )

    print("OK - headers gerados em", INCLUDE_WEB)
    print(f"style.css : {len(css_src):6d} B fonte -> {len(css_min):6d} B min -> {len(css_gz):5d} B gzip")
    print(f"script.js : {len(js_src):6d} B fonte -> {len(js_min):6d} B min -> {len(js_gz):5d} B gzip")
    print(f"shell.html: {len(shell_src):6d} B fonte -> {len(shell_min):6d} B min -> {len(shell_gz):5d} B gzip")
    print(f"total (css+js+shell+paginas) em flash: {total_gz} bytes")


if __name__ == "__main__":
    main()
