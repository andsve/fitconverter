CC = x86_64-w64-mingw32-gcc
CFLAGS = -Wall -Wextra
CFLAGS += -DDEBUG
CFLAGS += -DHAVE_STRCPY_S
CFLAGS += -DHAVE_FOPEN_S
CFLAGS += -DHAVE_FREAD_S
# CFLAGS += -DHAVE_SPRINTF_S
CFLAGS += -DHAVE_STRCAT_S
CFLAGS += -DHAVE_MEMCPY_S
# CFLAGS += -DHAVE_MEMSET_S
CFLAGS += -DHAVE_STRNLEN_S
CFLAGS += -DHAVE_FTELLI64
CFLAGS += -DHAVE_FSEEKI64
CFLAGS += -DHAVE_FTELLO64
CFLAGS += -DHAVE_FSEEKO64

LDFLAGS = -mwindows -lcomdlg32 -municode ./libTinyTIFF_Release.a

OBJS = $(SRCS:.c=.o)
SRCS = main.c tinytiffwriter.c tinytiff_ctools_internal.c
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