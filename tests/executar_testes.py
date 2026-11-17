from pathlib import Path
import subprocess
import sys
import os

RAIZ = Path(__file__).resolve().parent.parent
NOME_BIN = "lexer_test.exe" if os.name == "nt" else "lexer_test"
EXECUTAVEL = RAIZ / NOME_BIN

# fallback caso compile sem .exe no WSL/Git Bash ou com .exe
if not EXECUTAVEL.exists() and (RAIZ / "lexer_test.exe").exists():
    EXECUTAVEL = RAIZ / "lexer_test.exe"
elif not EXECUTAVEL.exists() and (RAIZ / "lexer_test").exists():
    EXECUTAVEL = RAIZ / "lexer_test"


def executar(arquivo, esperado, sintatico=False):
    comando = [str(EXECUTAVEL)]

    if sintatico:
        comando.append("--sintatico")

    comando.append(str(arquivo))

    resultado = subprocess.run(
        comando,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
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

    if sucesso:
        print("\nTodos os testes passaram.")
        return 0

    print("\nA suíte apresentou falhas.")
    return 1


if __name__ == "__main__":
    sys.exit(main())