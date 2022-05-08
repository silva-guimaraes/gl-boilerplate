CC=gcc
FLAGS=-Wall -Wextra 
TARGET=learnopengl
GLAD=../glad/src/glad.c
LIBS=-lglfw3 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm

main: learnopengl.c 
	$(CC) $(FLAGS)  $(TARGET).c $(GLAD) -o $(TARGET).out $(LIBS)
run: main 
	./$(TARGET).out
