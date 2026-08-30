# configuracoes gerais
cc = gcc
flex_cmd = flex
bison_cmd = bison
lex_src = src/lexer.l
parser_src = src/parser.y
main_src = src/main.c
lex_out = src/lex.yy.c
parser_out = src/parser.tab.c
parser_header = src/parser.tab.h
test_dir_validos = tests/validos
test_dir_invalidos = tests/invalidos

# ajusta comandos de acordo com o sistema operacional
ifeq ($(OS),Windows_NT)
	target = lexer_test.exe
	exec = .\\$(target)
	rm = powershell -Command "Remove-Item -ErrorAction SilentlyContinue $(lex_out), $(parser_out), $(parser_header), src/parser.output, $(target)"
	run_tests_validos = powershell -Command 'Get-ChildItem $(test_dir_validos)/*.c -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "testando valido:" $$_.Name -ForegroundColor Green; Get-Content $$_.FullName | $(exec) }'
	run_tests_invalidos = powershell -Command 'Get-ChildItem $(test_dir_invalidos)/*.c -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "testando invalido:" $$_.Name -ForegroundColor Red; Get-Content $$_.FullName | $(exec) }'
else
	target = lexer_test
	exec = ./$(target)
	rm = rm -f $(lex_out) $(parser_out) $(parser_header) src/parser.output $(target)
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

# regra padrao: compila e roda todos os testes
all: build test

# compila o analisador (Flex + Bison + gcc)
build: $(lex_src) $(parser_src) $(main_src)
	$(bison_cmd) -d -v -o $(parser_out) $(parser_src)
	$(flex_cmd) -o $(lex_out) $(lex_src)
	$(cc) -Wall -o $(target) $(lex_out) $(parser_out) $(main_src)

# executa no terminal para digitar os testes manualmente
run: build
	@$(exec)

# roda todos os testes (validos e invalidos)
test: test-validos test-invalidos

# roda apenas os testes validos (parte da carol)
test-validos: build
	@echo "--- iniciando testes validos ---"
	@$(run_tests_validos)
	@echo "fim dos testes validos"

# roda apenas os testes invalidos (sua parte da issue)
test-invalidos: build
	@echo "--- iniciando testes invalidos ---"
	@$(run_tests_invalidos)
	@echo "fim dos testes invalidos"

# apaga os arquivos gerados
clean:
	@$(rm)