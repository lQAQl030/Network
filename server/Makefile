include ../Make.defines

PROGS =	clean server

all:	${PROGS}

server:
	${CXX} -fsanitize=address -g -pthread -o server server.cpp Timer.cpp
		
clean:
		rm -f ${PROGS} ${CLEANFILES}
