LAB1_DIR := labs/lab-01-reloj-analogico
LAB2_DIR := labs/lab-02-proyecciones-3d
VECTORIAL_DIR := extras/calculo-vectorial-figura-1

.PHONY: all lab1 lab2 vectorial reports clean

all: lab1 lab2 vectorial

lab1:
	$(MAKE) -C $(LAB1_DIR)

lab2:
	$(MAKE) -C $(LAB2_DIR)

vectorial:
	$(MAKE) -C $(VECTORIAL_DIR)

reports:
	$(MAKE) -C $(LAB1_DIR) report
	$(MAKE) -C $(LAB2_DIR) report

clean:
	$(MAKE) -C $(LAB1_DIR) clean
	$(MAKE) -C $(LAB2_DIR) clean
	$(MAKE) -C $(VECTORIAL_DIR) clean
