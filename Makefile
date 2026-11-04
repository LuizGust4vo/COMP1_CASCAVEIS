cc = gcc
flex_cmd = flex
bison_cmd = bison
lex_src = src/lexer.l
parser_src = src/parser.y
main_src = src/main.c
ast_src = src/ast.c
ast_header = src/ast.h
ts_src = src/tabela_simbolos.c
ts_header = src/tabela_simbolos.h
sem_src = src/semantico.c
sem_header = src/semantico.h
ast_test_src = tests/ast/teste_ast.c
ts_test_src = tests/simbolos/teste_simbolos.c
lex_out = src/lex.yy.c
parser_out = src/parser.tab.c
parser_header = src/parser.tab.h
test_dir_validos = tests/validos
test_dir_invalidos = tests/invalidos

ifeq ($(OS),Windows_NT)
	target = lexer_test.exe
	exec = .\\$(target)
	ast_target = ast_test.exe
	ast_exec = .\\$(ast_target)
	ts_target = simbolos_test.exe
	ts_exec = .\\$(ts_target)
	rm = powershell -Command "Remove-Item -ErrorAction SilentlyContinue $(lex_out), $(parser_out), $(parser_header), src/parser.output, $(target), $(ast_target), $(ts_target), ast_saida.tmp"
	run_tests_validos = powershell -Command 'Get-ChildItem $(test_dir_validos)/*.c -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "testando valido:" $$_.Name -ForegroundColor Green; Get-Content $$_.FullName | $(exec) }'
	run_tests_invalidos = powershell -Command 'Get-ChildItem $(test_dir_invalidos)/*.c -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "testando invalido:" $$_.Name -ForegroundColor Red; Get-Content $$_.FullName | $(exec) }'
else
	target = lexer_test
	exec = ./$(target)
	ast_target = ast_test
	ast_exec = ./$(ast_target)
	ts_target = simbolos_test
	ts_exec = ./$(ts_target)
	rm = rm -f $(lex_out) $(parser_out) $(parser_header) src/parser.output $(target) $(ast_target) $(ts_target) ast_saida.tmp
	run_tests_validos = for file in $(test_dir_validos)/*.c; do \
		if [ -f "$$file" ]; then \
			echo "\033[1;32mtestando valido: $$file\033[0m"; \
			$(exec) < "$$file" || true; \
		fi \
	done
	run_tests_invalidos = for file in $(test_dir_invalidos)/*.c; do \
		if [ -f "$$file" ]; then \
			echo "\033[1;31mtestando invalido: $$file\033[0m"; \
			$(exec) < "$$file" || true; \
		fi \
	done
endif

all: build test

build: $(lex_src) $(parser_src) $(main_src) $(ast_src) $(ast_header) $(ts_src) $(ts_header) $(sem_src) $(sem_header)
	$(bison_cmd) -d -v -o $(parser_out) $(parser_src)
	$(flex_cmd) -o $(lex_out) $(lex_src)
	$(cc) -Wall -o $(target) $(lex_out) $(parser_out) $(ast_src) $(ts_src) $(sem_src) $(main_src)

run: build
	@$(exec)

test: test-validos test-invalidos test-ast test-simbolos test-semanticos

test-validos: build
	@echo "--- iniciando testes validos ---"
	@$(run_tests_validos)
	@echo "fim dos testes validos"

test-invalidos: build
	@echo "--- iniciando testes invalidos ---"
	@$(run_tests_invalidos)
	@echo "fim dos testes invalidos"

test-ast: build $(ast_test_src)
	$(cc) -Wall -o $(ast_target) $(lex_out) $(parser_out) $(ast_src) $(ts_src) $(ast_test_src)
	@echo "--- iniciando testes da AST ---"
	@$(ast_exec)

test-simbolos: build $(ts_test_src)
	$(cc) -Wall -o $(ts_target) $(lex_out) $(parser_out) $(ast_src) $(ts_src) $(ts_test_src)
	@echo "--- iniciando testes da tabela de simbolos ---"
	@$(ts_exec)

# testes semanticos: validos devem sair com 0, invalidos com 1
test-semanticos: build
	@echo "--- iniciando testes semanticos ---"
	@falhas=0; \
	for file in tests/semanticos/validos/*.c; do \
		if $(exec) "$$file" > /dev/null 2>&1; then echo "ok (aceito): $$file"; \
		else echo "FALHOU (deveria aceitar): $$file"; falhas=$$((falhas+1)); fi; \
	done; \
	for file in tests/semanticos/invalidos/*.c; do \
		if $(exec) "$$file" > /dev/null 2>&1; then echo "FALHOU (deveria rejeitar): $$file"; falhas=$$((falhas+1)); \
		else echo "ok (rejeitado): $$file"; fi; \
	done; \
	if [ $$falhas -ne 0 ]; then echo "Testes semanticos: $$falhas falha(s)"; exit 1; fi; \
	echo "Testes semanticos: todos passaram"

clean:
	@$(rm)