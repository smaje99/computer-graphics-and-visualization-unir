CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -pedantic -O2
LDFLAGS := -lX11 -lGL -lm
TARGET := solution
SOURCE := solution.c
REPORT_DIR := report
REPORT_TEX := $(REPORT_DIR)/informe.tex

.PHONY: all clean report

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

report:
	cd $(REPORT_DIR) && pdflatex -interaction=nonstopmode informe.tex

clean:
	rm -f $(TARGET)
	rm -f $(REPORT_DIR)/*.aux $(REPORT_DIR)/*.fdb_latexmk $(REPORT_DIR)/*.fls $(REPORT_DIR)/*.log $(REPORT_DIR)/*.out $(REPORT_DIR)/*.pdf
