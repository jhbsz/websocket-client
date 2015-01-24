CROSS=gcc
LIBS = -ljson-c -lwebsockets  -luuid -lsqlite3
OBJS = websocket_client.c
TAG=websocket_client
main: ${OBJS}
	${CROSS} -o ${TAG} ${OBJS} ${LIBS}
clean:
	 @rm -vf ${TAG} *.o *~
