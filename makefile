CC = gcc
CFLAGS = -Wall

# Main targets
all: monitor treasure_hub calculate_score treasure_manager

# Individual targets
monitor: monitor.c
	$(CC) $(CFLAGS) -o monitor monitor.c

treasure_hub: treasure_hub.c
	$(CC) $(CFLAGS) -o treasure_hub treasure_hub.c

calculate_score: calculate_score.c
	$(CC) $(CFLAGS) -o calculate_score calculate_score.c

treasure_manager: treasure_manager.c
	$(CC) $(CFLAGS) -o treasure_manager treasure_manager.c

# Clean target
clean:
	rm -f monitor treasure_hub calculate_score treasure_manager

# Phony targets
.PHONY: all clean

