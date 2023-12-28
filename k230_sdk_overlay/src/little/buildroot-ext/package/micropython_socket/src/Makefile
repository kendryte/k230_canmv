CROSS_COMPILE = riscv64-unknown-linux-gnu-
CC=$(CROSS_COMPILE)gcc

TARGET = micropython_socket_server
OBJS = micropython_socket_server.c



all:
	${CC} ${OBJS} -Wall -g -o ${TARGET}  $(EXTHEAD) $(EXT_LIBS)
	@echo "Compile done."
clean:
	@rm -f *.o ${TARGET}
	@echo "Clean done."
