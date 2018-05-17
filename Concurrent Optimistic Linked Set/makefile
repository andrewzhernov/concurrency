.PHONY: build, run, tar, clear, local_build, generate_tests, local_run, all

# я.контест build
build:
	TMP=$$(pwd) bash -c 'clang++ -std=c++14 -pthread -O0 -g -Wall -Wextra -Werror -o ./solution *.cpp && for s in address thread; do clang++ -std=c++14 -fsanitize=$$s -O3 -g -Wall -Wextra -Werror -o ./solution_$$s *.cpp; done'

# я.контест run
run:
	./solution 2>&1 ./input.txt && ./solution_address 2>&1 ./input.txt && ./solution_thread 2>&1 ./input.txt

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
	mkdir build && cp $$(pwd)/includes/* ./build/ \
	&& cp $$(pwd)/solutions/solution.h ./build/ \
	&& cp $$(pwd)/makefile ./build/ \
	&& cd build && make build

# Генерация входных и выходных файлов тестов
generate_tests: local_build
	cd tests && rm -rf *.in* && ../generate_tests.py --workers_range 1,8,1 --items_log_range 2,0,4,1 && cd ../build && bash -c 'for t in ../tests/*.in; do echo -n "Generating $$t.out from $$t ... " && cp $$t ./input.txt && make -s run > $$t.out && echo "Done"; done'

# Локальный запуск. Примерно повторяет действия запуска в я.контесте
local_run: local_build
	cd build && bash -c 'for t in ../tests/*.in; do echo "Testing $$t ..." && cp $$t ./input.txt && make -s run > ./output.txt && diff $$t.out ./output.txt && echo OK; done'

all: local_run

