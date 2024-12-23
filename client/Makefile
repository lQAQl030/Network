include ../Make.defines

PROGS =	clean client

all:	${PROGS}

client:
		${CXX} -o client client.cpp -lsfml-graphics -lsfml-window -lsfml-system
		
clean:
		rm -f ${PROGS} ${CLEANFILES}
