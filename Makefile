CC = x86_64-w64-mingw32-gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -mwindows -lcomdlg32 -municode

OBJS = $(SRCS:.c=.o)
SRCS = main.c
TARGET = fit_converter.exe

all: $(TARGET)

# $(TARGET): $(SRCS)
#	$(CC) $(SRCS) $(CFLAGS) $(LDFLAGS) -o $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) 