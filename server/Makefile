include ../Make.defines

PROGS =	clean server

all:	${PROGS}

server:
		${CXX} -g -o server server.cpp Timer.cpp -pthread
		
clean:
		rm -f ${PROGS} ${CLEANFILES}
