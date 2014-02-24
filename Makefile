CC = gcc
TARGET = server
RM = rm -f
FLAGS = -lpthread -g -Wall
SRC = parameters.c shttpd.c request.c worker.c mime.c
OBJS = parameters.o shttpd.o worker.o request.o mime.o
%.o: %.c
	$(CC) -c -g $(SRC)
all:$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(FLAGS)
clean:
	$(RM) $(TARGET) $(OBJS)
