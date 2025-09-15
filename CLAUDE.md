# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

From the `app/` directory:
- `make` - Build the main application (START_CODE executable)
- `make clean` - Clean all object files and executables

The build system automatically handles:
- Building the vecmath library dependency first
- Creating required directories (obj/, lib/)
- Linking with OpenGL libraries (GL, GLU, GLUT)

## Architecture

This is a C++ computer graphics application for curve and surface modeling:

### Core Components
- **vecmath library** (`lib/vecmath/`) - Vector math utilities with Matrix, Vector2f, Vector3f classes
- **Main application** (`app/`) - OpenGL-based curve and surface visualization tool

### Key Modules
- `main.cpp` - OpenGL application entry point with GLUT window management and UI controls
- `curve.h/cpp` - Bezier and B-spline curve generation with CurvePoint structures (vertex, tangent, normal, binormal)
- `surf.h/cpp` - Surface generation from curves with triangle mesh output
- `parse.h/cpp` - SWP file format parser for loading curve/surface definitions
- `camera.h/cpp` - 3D camera controls for scene navigation

### File Format
The application uses SWP files to define curves and surfaces:
- `bez2`/`bsp2` - 2D Bezier/B-spline curves
- `bez3`/`bsp3` - 3D Bezier/B-spline curves
- Control points specified in brackets: `[x y]` or `[x y z]`

### Dependencies
- C++17 compiler
- OpenGL, GLU, GLUT libraries
- Custom vecmath library (built automatically)

### Project Structure
```
app/
├── src/          # Application source files
├── include/      # Application headers
├── Makefile      # Build configuration
lib/vecmath/      # Vector math library
swp/              # Sample curve/surface files
```