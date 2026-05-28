LAB1_DIR := labs/lab-01-reloj-analogico
LAB7_DIR := labs/lab-07-proyecciones-3d
VECTORIAL_DIR := extras/calculo-vectorial-figura-1

.PHONY: all lab1 lab7 vectorial reports clean

all: lab1 lab7 vectorial

lab1:
	$(MAKE) -C $(LAB1_DIR)

lab7:
	$(MAKE) -C $(LAB7_DIR)

vectorial:
	$(MAKE) -C $(VECTORIAL_DIR)

reports:
	$(MAKE) -C $(LAB1_DIR) report
	$(MAKE) -C $(LAB7_DIR) report

clean:
	$(MAKE) -C $(LAB1_DIR) clean
	$(MAKE) -C $(LAB7_DIR) clean
	$(MAKE) -C $(VECTORIAL_DIR) clean
