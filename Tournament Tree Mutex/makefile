.PHONY: build, run, tar, clear, local_build, generate_tests, local_run, all

# я.контест build
build:
	TMP=$$(pwd) bash -c '\
	  for s in "" "-fsanitize=address" "-fsanitize=thread"; do \
	    clang++ $$s -fno-omit-frame-pointer -std=c++14 -pthread -O3 -g -Wall -Wextra -Werror -o ./solution$${s#-fsanitize=} *.cpp; \
	  done'

# я.контест run
run:
	export ASAN_OPTIONS=symbolize=1 \
	&& ./solutionaddress 2>&1 ./input.txt \
	&& ./solutionthread 2>&1 ./input.txt \
	&& ./solution 2>&1 ./input.txt

#########################################################################
# Вспомогательные таргеты для локальной отладки решений и тестов задачи #
#########################################################################

# Запаковывает задачу в архив для загрузки в я.контест
tar:
	@tar cf ../$$(basename $$(pwd)).tar includes solutions tests makefile

# Очистка директории сборки
clear:
	rm -rf build

# Локальная сборка. Примерно повторяет действия сборки в я.контесте
local_build: clear
	mkdir build \
	&& cp $$(pwd)/includes/* ./build/ \
	&& cp $$(pwd)/solutions/solution.h ./build/ \
	&& cp $$(pwd)/makefile ./build/ \
	&& cd build \
	&& make build

# Генерация входных и выходных файлов тестов
generate_tests: local_build
	mkdir -p tests \
	&& cd tests \
	&& rm -rf *.in* \
	&& ../generate_tests.py \
	&& cd ../build \
	&& bash -c '\
	  for t in ../tests/*.in; do \
	    echo -n "Generating $$t.out from $$t ... " \
	    && cp $$t ./input.txt \
	    && make -s run > $$t.out \
	    && echo "Done"; \
	  done'

# Локальный запуск. Примерно повторяет действия запуска в я.контесте
local_run: local_build
	cd build \
	&& bash -c '\
	  for t in ../tests/*.in; do \
	    TEST_OUT_NAME=output.$$(basename $$t).txt; \
	    echo "Testing $$t ... (see build/$$TEST_OUT_NAME)" \
	    && cp $$t ./input.txt \
	    && make -s run > "./$$TEST_OUT_NAME" \
	    && diff $$t.out ./$$TEST_OUT_NAME \
	    && echo OK; \
	  done'


all: local_run

