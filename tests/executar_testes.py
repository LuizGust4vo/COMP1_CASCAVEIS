from pathlib import Path
import difflib
import os
import subprocess
import sys

RAIZ = Path(__file__).resolve().parent.parent
NOME_BIN = "lexer_test.exe" if os.name == "nt" else "lexer_test"
EXECUTAVEL = RAIZ / NOME_BIN

# fallback caso compile sem .exe no WSL/Git Bash ou com .exe
if not EXECUTAVEL.exists() and (RAIZ / "lexer_test.exe").exists():
    EXECUTAVEL = RAIZ / "lexer_test.exe"
elif not EXECUTAVEL.exists() and (RAIZ / "lexer_test").exists():
    EXECUTAVEL = RAIZ / "lexer_test"


def normalizar_saida(texto):
    return texto.replace("\r\n", "\n").strip()


def executar(arquivo, esperado, sintatico=False):
    comando = [str(EXECUTAVEL)]

    if sintatico:
        comando.append("--sintatico")

    comando.append(str(arquivo))

    resultado = subprocess.run(
        comando,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
    )

    passou = resultado.returncode == esperado

    status = "OK" if passou else "FALHOU"

    print(
        f"{status}: {arquivo.relative_to(RAIZ)} "
        f"(esperado={esperado}, obtido={resultado.returncode})"
    )

    if not passou:
        if resultado.stdout:
            print("stdout:")
            print(resultado.stdout)

        if resultado.stderr:
            print("stderr:")
            print(resultado.stderr)

    return passou


def executar_diretorio(diretorio, esperado, sintatico=False):
    passou = True

    for arquivo in sorted(diretorio.glob("*.c")):
        if not executar(arquivo, esperado, sintatico):
            passou = False

    return passou


def executar_teste_ir(arquivo_c, arquivo_ir):
    if not arquivo_ir.exists():
        print(
            f"FALHOU (gabarito ausente): {arquivo_c.relative_to(RAIZ)} "
            f"(esperado: {arquivo_ir.relative_to(RAIZ)})"
        )
        return False

    resultado = subprocess.run(
        [str(EXECUTAVEL), "--ir", str(arquivo_c)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
    )

    if resultado.returncode != 0:
        print(
            f"FALHOU (IR): {arquivo_c.relative_to(RAIZ)} "
            f"(esperado=0, obtido={resultado.returncode})"
        )
        if resultado.stdout:
            print("stdout:")
            print(resultado.stdout)
        if resultado.stderr:
            print("stderr:")
            print(resultado.stderr)
        return False

    obtido = normalizar_saida(resultado.stdout)
    esperado = normalizar_saida(arquivo_ir.read_text(encoding="utf-8"))

    if obtido == esperado:
        print(f"OK (IR): {arquivo_c.relative_to(RAIZ)}")
        return True

    print(f"FALHOU (divergencia IR): {arquivo_c.relative_to(RAIZ)}")
    diff = difflib.unified_diff(
        esperado.splitlines(),
        obtido.splitlines(),
        fromfile=str(arquivo_ir.relative_to(RAIZ)),
        tofile="saida_obtida",
        lineterm="",
    )
    for linha in diff:
        print("  ", linha)
    return False


def executar_suite_ir():
    dir_entradas = RAIZ / "tests" / "ir" / "entradas"
    dir_esperados = RAIZ / "tests" / "ir" / "esperados"
    entradas = sorted(dir_entradas.glob("*.c"))

    if not entradas:
        print("FALHOU: nenhum teste de IR encontrado em tests/ir/entradas/")
        return False

    passou = True
    for arquivo_c in entradas:
        arquivo_ir = dir_esperados / f"{arquivo_c.stem}.ir"
        if not executar_teste_ir(arquivo_c, arquivo_ir):
            passou = False

    for arquivo_ir in sorted(dir_esperados.glob("*.ir")):
        if not (dir_entradas / f"{arquivo_ir.stem}.c").exists():
            print(
                f"FALHOU (gabarito sem entrada): "
                f"{arquivo_ir.relative_to(RAIZ)}"
            )
            passou = False

    return passou


def executar_ir_invalidos(diretorio):
    passou = True

    for arquivo in sorted(diretorio.glob("*.c")):
        resultado = subprocess.run(
            [str(EXECUTAVEL), "--ir", str(arquivo)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        gerou_ir = "func " in resultado.stdout or "endfunc" in resultado.stdout

        if resultado.returncode != 0 and not gerou_ir:
            print(f"OK (IR bloqueado): {arquivo.relative_to(RAIZ)}")
        else:
            print(
                f"FALHOU (IR deveria ser bloqueado): {arquivo.relative_to(RAIZ)} "
                f"(codigo={resultado.returncode}, gerou_ir={gerou_ir})"
            )
            passou = False

    return passou


def main():
    if not EXECUTAVEL.exists():
        print("ERRO: lexer_test não encontrado.")
        print("Execute 'make build' antes dos testes.")
        return 1

    sucesso = True

    print("\n=== TESTES SINTÁTICOS VÁLIDOS ===")
    if not executar_diretorio(
        RAIZ / "tests" / "validos",
        esperado=0,
        sintatico=True
    ):
        sucesso = False

    print("\n=== TESTES SINTÁTICOS INVÁLIDOS ===")
    if not executar_diretorio(
        RAIZ / "tests" / "invalidos",
        esperado=1,
        sintatico=True
    ):
        sucesso = False

    print("\n=== TESTES SEMÂNTICOS VÁLIDOS ===")
    if not executar_diretorio(
        RAIZ / "tests" / "semanticos" / "validos",
        esperado=0
    ):
        sucesso = False

    print("\n=== TESTES SEMÂNTICOS INVÁLIDOS ===")
    if not executar_diretorio(
        RAIZ / "tests" / "semanticos" / "invalidos",
        esperado=1
    ):
        sucesso = False

    print("\n=== TESTES DE GERAÇÃO DE CÓDIGO INTERMEDIÁRIO (IR) ===")
    if not executar_suite_ir():
        sucesso = False

    print("\n=== BLOQUEIO DE IR EM PROGRAMAS SEMANTICAMENTE INVÁLIDOS ===")
    if not executar_ir_invalidos(RAIZ / "tests" / "semanticos" / "invalidos"):
        sucesso = False

    if sucesso:
        print("\nTodos os testes passaram.")
        return 0

    print("\nA suíte apresentou falhas.")
    return 1


if __name__ == "__main__":
    sys.exit(main())