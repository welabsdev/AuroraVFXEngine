# Aurora VFX & Astro-Sim Engine

## Low-Level, Open Source Graphics & Relativistic Astrophysics Simulation Engine

The **Aurora VFX Engine** is a high-performance **graphics renderer** and **astrophysics simulation engine** written entirely in modern **C++20**. Rather than relying on a full-featured game engine, Aurora is built directly on top of the **OpenGL Core Profile**, providing explicit control over rendering, memory management, and GPU resources.

The project is designed as both an educational platform for modern computer graphics and a foundation for physically inspired space simulations, combining low-level rendering techniques with real-time visualization of relativistic phenomena.

> [!IMPORTANT]
> **Current Version:** `v0.7.0`  
> **Language Standard:** `C++20`  
> **Graphics API:** `OpenGL 3.3+ Core Profile`  
> **Developer:** **Wenderson Dias** (*welabsdev*)

![Aurora Demo](demo.gif)

---

# Features

- Modern OpenGL 3.3+ renderer
- Custom rendering pipeline
- Explicit GPU resource management
- GLSL shader framework
- GPU-driven particle simulation
- Runtime shader hot reloading
- Custom camera system
- Assimp mesh importing
- Dear ImGui debugging interface
- Physically inspired astrophysics visualization
- Minimal external dependencies

---

# Design Philosophy

Aurora follows a simple set of design principles:

- Minimal abstractions
- Explicit memory management
- GPU-first rendering
- Modular architecture
- Modern C++ practices
- Educational implementation of graphics techniques

Unlike traditional game engines, Aurora focuses on exposing the rendering pipeline while maintaining a clean and extensible architecture suitable for experimentation and research.

---

# Technical Stack

## Graphics & Window System

- **SDL3** — Window management, input handling, timing and OpenGL context creation.
- **GLAD** — OpenGL function loader configured for the Core Profile.

## Mathematics & Assets

- **GLM** — GLSL-compatible mathematics library.
- **Assimp** — Importer for 3D models and mesh data.

## User Interface

- **Dear ImGui** — Immediate-mode debugging interface integrated directly into the rendering pipeline.

---

# Engine Architecture

```text
Application
     │
     ▼
 SDL3 Window
     │
     ▼
OpenGL Context
     │
     ▼
Aurora Engine
 ├── Core
 ├── Renderer
 ├── Camera
 ├── Shader System
 ├── Mesh
 ├── Physics
 ├── UI
 └── Scene
     │
     ▼
 GPU
```

---

# Astrophysics Simulation

Aurora implements several physically inspired visualization techniques executed on the GPU through GLSL shaders.

## Paczyński–Wiita Potential

Implements the pseudo-Newtonian potential for approximating particle motion around compact massive objects.

## Gravitational Lensing

Weak-field Schwarzschild approximation for real-time light deflection.

## Frame Dragging

Approximate Kerr-inspired rotational effects applied to particle trajectories.

## Relativistic Doppler Shift

Dynamic redshift and blueshift effects based on the relative velocity between particles and the observer.

## Space Curvature Visualization

Procedural mesh generation inspired by **Flamm's Paraboloid**.

---

# Performance Goals

Aurora is designed around predictable performance through:

- Modern OpenGL rendering
- Vertex Buffer Objects (VBO)
- Vertex Array Objects (VAO)
- Batch rendering
- Cache-friendly data layouts
- Explicit GPU memory management
- Reduced CPU overhead

---

# Hardware Requirements

| Component | Minimum | Recommended |
|-----------|----------|-------------|
| Operating System | Windows 10 (64-bit) | Windows 11 (64-bit) |
| CPU | Intel Core i5 / AMD Ryzen 5 | Intel Core i7 / AMD Ryzen 7 |
| Memory | 8 GB RAM | 16 GB RAM |
| GPU | NVIDIA GTX 1050 / AMD RX 560 | NVIDIA RTX 3060 / AMD RX 6600 |
| Graphics API | OpenGL 3.3 | OpenGL 4.6 |

---

# Dependencies

Aurora currently depends on the following open-source libraries:

| Library | Purpose |
|----------|---------|
| SDL3 | Windowing, Input and Context Creation |
| GLAD | OpenGL Function Loader |
| GLM | Mathematics |
| Assimp | Model Importing |
| Dear ImGui | Debug Interface |

---

# Building

> [!NOTE]
> Aurora currently targets **Windows** using **Visual Studio** and the **MSVC toolchain**.

Clone the repository:

```bash
git clone https://github.com/welabsdev/AuroraVFXEngine.git
```

Configure your project using **C++20** and link the following libraries:

```text
SDL3.lib
assimp.lib
opengl32.lib
```

Compile using the **Release x64** configuration.

---

# Project Structure

```text
Aurora/
│
├── Assets/
├── Dependencies/
├── Include/
├── Source/
│   ├── Core/
│   ├── Graphics/
│   ├── Physics/
│   ├── Renderer/
│   ├── Scene/
│   ├── Shader/
│   └── UI/
│
├── Shaders/
├── Scripts/
└── Docs/
```

---

# Roadmap

> [!TIP]
> The following features are planned for future releases.

- [x] OpenGL Renderer
- [x] GLSL Shader System
- [x] Assimp Integration
- [x] Dear ImGui
- [ ] Deferred Rendering
- [ ] HDR Rendering
- [ ] Shadow Mapping
- [ ] Physically Based Rendering (PBR)
- [ ] Compute Shader Backend
- [ ] Vulkan Renderer
- [ ] GPU Ray Marching
- [ ] SPH Fluid Simulation
- [ ] Multi-threaded Renderer
- [ ] Linux Support

---

# Contributing

Contributions are welcome.

> [!IMPORTANT]
> Performance improvements, rendering optimizations, shader development, documentation, and bug reports are highly appreciated.

Please open an **Issue** or submit a **Pull Request**.

---

# License

This project is distributed under the **GNU 3.0 LICENSE**.

---

**Aurora VFX & Astro-Sim Engine**

*Built with modern C++, OpenGL, and computational physics.*
