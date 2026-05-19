# Computer Graphics and Visualization UNIR

Static analog clock developed in C with classic OpenGL for the UNIR "Informática Gráfica y Visualización" laboratory.

## Project contents

- `solution.c`: source code of the static analog clock.
- `Makefile`: build rules for Linux and report compilation.
- `report/informe.tex`: LaTeX report.
- `captures/reloj.png`: screenshot included in the report.

## Requirements

### Linux

- `gcc`
- OpenGL development libraries
- X11 development libraries
- `make`
- `pdflatex` if you want to compile the report

On Debian/Ubuntu-based systems, the usual packages are:

```bash
sudo apt install build-essential libgl1-mesa-dev libx11-dev texlive-latex-base
```

### Windows

The current implementation uses `X11/GLX`, so it is **not a native Windows build**. The recommended way to run it on Windows is through **WSL**.

You need:

- WSL2 or WSLg installed
- An Ubuntu-based distribution inside WSL
- The same Linux packages listed above

If your Windows setup does not provide WSLg, you also need an X server such as VcXsrv or Xming.

## Build and run

### Linux

Build the program:

```bash
make
```

Run the clock:

```bash
./solution
```

Compile the LaTeX report:

```bash
make report
```

### Windows

#### Option 1: WSLg

Inside your WSL terminal:

```bash
make
./solution
```

This is the simplest option when using Windows 11 with WSLg enabled.

#### Option 2: WSL2 + external X server

1. Start your X server in Windows.
2. Open the project in WSL.
3. Export the display if needed:

```bash
export DISPLAY=:0
```

4. Build and run:

```bash
make
./solution
```

## Notes

- The clock is static by design: the time is captured once at startup and does not update during execution.
- The current repository does not provide native Win32 or GLUT/freeglut support.
- If native Windows execution is required, the source should be ported from `X11/GLX` to a cross-platform windowing layer such as GLUT/freeglut, GLFW, or Win32 + WGL.
