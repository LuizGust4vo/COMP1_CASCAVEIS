from pathlib import Path
import difflib
import os
import subprocess
import sys
import tempfile

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


TIMEOUT_PYTHON = 10


def rodar_compilador_python(arquivo_c, arquivo_py, extras=()):
    return subprocess.run(
        [str(EXECUTAVEL), *extras, "--python", str(arquivo_py), str(arquivo_c)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
    )


def executar_teste_e2e(arquivo_c, arquivo_esperado, dir_saida, extras=(), rotulo="E2E"):
    nome = arquivo_c.relative_to(RAIZ)

    if not arquivo_esperado.exists():
        print(f"FALHOU (gabarito ausente): {nome} "
              f"(esperado: {arquivo_esperado.relative_to(RAIZ)})")
        return False

    arquivo_py = dir_saida / f"{arquivo_c.stem}{''.join(extras).replace('-', '_')}.py"
    compilacao = rodar_compilador_python(arquivo_c, arquivo_py, extras)

    if compilacao.returncode != 0 or not arquivo_py.exists():
        print(f"FALHOU (compilacao C -> Python): {nome} "
              f"(codigo={compilacao.returncode}, gerou_py={arquivo_py.exists()})")
        if compilacao.stderr:
            print("stderr:")
            print(compilacao.stderr)
        return False

    try:
        execucao = subprocess.run(
            [sys.executable, str(arquivo_py)],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=TIMEOUT_PYTHON,
        )
    except subprocess.TimeoutExpired:
        print(f"FALHOU (timeout de {TIMEOUT_PYTHON}s ao executar Python): {nome}")
        return False

    if execucao.returncode != 0:
        print(f"FALHOU (execucao do Python gerado): {nome} "
              f"(codigo={execucao.returncode})")
        if execucao.stderr:
            print("stderr:")
            print(execucao.stderr)
        return False

    obtido = normalizar_saida(execucao.stdout)
    esperado = normalizar_saida(arquivo_esperado.read_text(encoding="utf-8"))

    if obtido == esperado:
        print(f"OK ({rotulo}): {nome}")
        return True

    print(f"FALHOU (saida divergente): {nome}")
    diff = difflib.unified_diff(
        esperado.splitlines(),
        obtido.splitlines(),
        fromfile=str(arquivo_esperado.relative_to(RAIZ)),
        tofile="saida_obtida",
        lineterm="",
    )
    for linha in diff:
        print("  ", linha)
    return False


def executar_suite_e2e(extras=(), rotulo="E2E"):
    dir_entradas = RAIZ / "tests" / "e2e" / "entradas"
    dir_esperados = RAIZ / "tests" / "e2e" / "esperados"
    entradas = sorted(dir_entradas.glob("*.c"))

    if not entradas:
        print("FALHOU: nenhum teste end-to-end encontrado em tests/e2e/entradas/")
        return False

    passou = True
    with tempfile.TemporaryDirectory(prefix="e2e_") as tmp:
        for arquivo_c in entradas:
            esperado = dir_esperados / f"{arquivo_c.stem}.out"
            if not executar_teste_e2e(arquivo_c, esperado, Path(tmp), extras, rotulo):
                passou = False

    for arquivo_out in sorted(dir_esperados.glob("*.out")):
        if not (dir_entradas / f"{arquivo_out.stem}.c").exists():
            print(f"FALHOU (gabarito sem entrada): {arquivo_out.relative_to(RAIZ)}")
            passou = False

    return passou


def executar_e2e_invalidos(diretorios):
    """Entradas invalidas (lexicas, sintaticas ou semanticas) nao podem gerar Python."""
    passou = True

    with tempfile.TemporaryDirectory(prefix="e2e_inv_") as tmp:
        for diretorio in diretorios:
            for arquivo in sorted(diretorio.glob("*.c")):
                arquivo_py = Path(tmp) / f"{arquivo.stem}.py"
                resultado = rodar_compilador_python(arquivo, arquivo_py)
                gerou_py = arquivo_py.exists()

                if resultado.returncode != 0 and not gerou_py:
                    print(f"OK (Python bloqueado): {arquivo.relative_to(RAIZ)}")
                else:
                    print(f"FALHOU (Python deveria ser bloqueado): "
                          f"{arquivo.relative_to(RAIZ)} "
                          f"(codigo={resultado.returncode}, gerou_py={gerou_py})")
                    passou = False

    return passou


def gera_ir(arquivo_c, extras=()):
    return subprocess.run(
        [str(EXECUTAVEL), *extras, "--ir", str(arquivo_c)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
    )


def conta_instrucoes(saida_ir):
    return sum(
        1 for linha in normalizar_saida(saida_ir).splitlines()
        if linha.strip() and not linha.startswith("Analise concluida")
    )


def executar_suite_otimizacao():
    """
    Para cada programa de tests/otimizacao/entradas:
      - a RI otimizada (--otimizar --ir) e igual ao gabarito .ir;
      - a RI otimizada tem menos instrucoes que a original;
      - o Python gerado com e sem --otimizar imprime o gabarito .out.
    """
    dir_entradas = RAIZ / "tests" / "otimizacao" / "entradas"
    dir_esperados = RAIZ / "tests" / "otimizacao" / "esperados"
    entradas = sorted(dir_entradas.glob("*.c"))

    if not entradas:
        print("FALHOU: nenhum teste de otimizacao em tests/otimizacao/entradas/")
        return False

    passou = True
    with tempfile.TemporaryDirectory(prefix="opt_") as tmp:
        for arquivo_c in entradas:
            nome = arquivo_c.relative_to(RAIZ)
            gabarito_ir = dir_esperados / f"{arquivo_c.stem}.ir"
            gabarito_out = dir_esperados / f"{arquivo_c.stem}.out"

            original = gera_ir(arquivo_c)
            otimizada = gera_ir(arquivo_c, ("--otimizar",))

            if original.returncode != 0 or otimizada.returncode != 0:
                print(f"FALHOU (geracao de RI): {nome}")
                passou = False
                continue

            if not gabarito_ir.exists():
                print(f"FALHOU (gabarito ausente): {gabarito_ir.relative_to(RAIZ)}")
                passou = False
            else:
                obtido = normalizar_saida(otimizada.stdout)
                esperado = normalizar_saida(gabarito_ir.read_text(encoding="utf-8"))
                if obtido == esperado:
                    print(f"OK (RI otimizada): {nome}")
                else:
                    print(f"FALHOU (RI otimizada divergente): {nome}")
                    for linha in difflib.unified_diff(
                        esperado.splitlines(), obtido.splitlines(),
                        fromfile=str(gabarito_ir.relative_to(RAIZ)),
                        tofile="saida_obtida", lineterm="",
                    ):
                        print("  ", linha)
                    passou = False

            antes = conta_instrucoes(original.stdout)
            depois = conta_instrucoes(otimizada.stdout)
            if depois < antes:
                print(f"OK (reducao {antes} -> {depois} instrucoes): {nome}")
            else:
                print(f"FALHOU (sem reducao: {antes} -> {depois} instrucoes): {nome}")
                passou = False

            for extras, rotulo in (((), "execucao sem otimizar"),
                                   (("--otimizar",), "execucao otimizada")):
                if not executar_teste_e2e(arquivo_c, gabarito_out, Path(tmp), extras, rotulo):
                    passou = False

    return passou


def executar_somente_otimizacao():
    sucesso = True

    print("\n=== OTIMIZACOES DO CODIGO INTERMEDIARIO ===")
    if not executar_suite_otimizacao():
        sucesso = False

    print("\n=== TESTES END-TO-END COM --otimizar ===")
    if not executar_suite_e2e(("--otimizar",), "E2E otimizado"):
        sucesso = False

    return sucesso


DIRETORIOS_INVALIDOS_E2E = [
    RAIZ / "tests" / "invalidos",
    RAIZ / "tests" / "semanticos" / "invalidos",
]


def executar_somente_e2e():
    sucesso = True

    print("\n=== TESTES END-TO-END (C -> PYTHON -> EXECUCAO) ===")
    if not executar_suite_e2e():
        sucesso = False

    print("\n=== BLOQUEIO DE CODIGO FINAL EM PROGRAMAS INVALIDOS ===")
    if not executar_e2e_invalidos(DIRETORIOS_INVALIDOS_E2E):
        sucesso = False

    return sucesso


def main():
    if not EXECUTAVEL.exists():
        print("ERRO: lexer_test não encontrado.")
        print("Execute 'make build' antes dos testes.")
        return 1

    if "--e2e" in sys.argv[1:]:
        if executar_somente_e2e():
            print("\nTodos os testes end-to-end passaram.")
            return 0
        print("\nOs testes end-to-end apresentaram falhas.")
        return 1

    if "--otimizacao" in sys.argv[1:]:
        if executar_somente_otimizacao():
            print("\nTodos os testes de otimizacao passaram.")
            return 0
        print("\nOs testes de otimizacao apresentaram falhas.")
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

    if not executar_somente_e2e():
        sucesso = False

    if not executar_somente_otimizacao():
        sucesso = False

    if sucesso:
        print("\nTodos os testes passaram.")
        return 0

    print("\nA suíte apresentou falhas.")
    return 1


if __name__ == "__main__":
    sys.exit(main())