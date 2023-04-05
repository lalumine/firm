MODEL_PATH=apps/tools/normgraph

FIRM_LOG_LEVEL=LOG_WARN
PROC_LOG_LEVEL=LOG_INFO
CC=clang++
CFLAGS += -I. -Iimpl -O3 -std=c++20 ${LOG_LEVEL} -DNDEBUG


all: firm vectcmp format divide process

firm: apps/main.cpp
	${CC} ${CFLAGS} -DLOG_LEVEL=${FIRM_LOG_LEVEL} $^ -o firm

vectcmp: apps/tools/compare/vectcmp.cpp
	${CC} ${CFLAGS} -DLOG_LEVEL=${PROC_LOG_LEVEL} $^ -o $@

format: ${MODEL_PATH}/format.cpp
	${CC} ${CFLAGS} -DLOG_LEVEL=${PROC_LOG_LEVEL} $^ -o $@

divide: ${MODEL_PATH}/divide.cpp
	${CC} ${CFLAGS} -DLOG_LEVEL=${PROC_LOG_LEVEL} $^ -o $@

process: ${MODEL_PATH}/process.cpp
	${CC} ${CFLAGS} -DLOG_LEVEL=${PROC_LOG_LEVEL} $^ -o $@

clean:
	rm -f firm vectcmp format divide process

.PHONY : clean
