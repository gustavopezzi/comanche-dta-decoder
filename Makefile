CC      = gcc
CFLAGS  = -std=c89 -pedantic -Wall -Wextra -O2
TARGET  = dta2bmp
MAPS    = 1 2 3 4

.PHONY: all run clean

all: $(TARGET)

$(TARGET): dta2bmp.c
	$(CC) $(CFLAGS) dta2bmp.c -o $(TARGET)

# Convert all four maps (run from the folder with the C?.DTA / D?.DTA files)
run: $(TARGET)
	for m in $(MAPS); do ./$(TARGET) $$m; done

clean:
	rm -f $(TARGET) *.bmp
