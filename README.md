# ✨ Aurora VFX & Astro-Sim Engine

> **Open-source graphics renderer and relativistic astrophysics simulation engine built from scratch in modern C++.**

Aurora is a lightweight graphics engine focused on **low-level rendering**, **GPU-driven simulations**, and **real-time astrophysical visualization**. Instead of relying on a full-featured game engine, Aurora communicates directly with the OpenGL API, providing explicit control over rendering, memory management, and GPU resources.

Designed primarily as a research and educational project, Aurora combines modern rendering techniques with physically inspired simulations of black holes, gravitational lensing, relativistic effects, and procedural space-time visualization.

---

## ✨ Features

- 🚀 Modern OpenGL 3.3+ Core Profile renderer
- ⚡ Custom C++23 rendering architecture
- 🛰️ GPU-based particle simulations
- 🎨 GLSL shader framework
- 📦 Model loading with Assimp
- 🖥️ SDL3 windowing and input system
- 📐 GLM mathematics library
- 🛠️ Dear ImGui debugging interface
- 🔥 Runtime shader hot-reloading
- 🌌 Relativistic astrophysics visualization
- 🧮 Custom math utilities
- 🧩 Minimal external dependencies

---

## 📷 Preview

<p align="center">
    <img src="demo.gif" width="900">
</p>

---

# 🎯 Philosophy

Aurora follows a simple philosophy:

- Minimal abstractions
- Explicit memory management
- GPU-first rendering
- Modern C++ architecture
- Open-source development
- Educational implementation of graphics techniques

Rather than hiding rendering behind large engine frameworks, Aurora exposes the graphics pipeline while keeping the code clean, modular and extensible.

---

# 🧰 Technology Stack

| Library | Purpose |
|----------|---------|
| **SDL3** | Window creation, input handling and OpenGL context |
| **GLAD** | OpenGL function loader |
| **GLM** | GLSL-compatible mathematics library |
| **Assimp** | 3D model importer |
| **Dear ImGui** | Runtime debugging interface |

---

# 🏗️ Engine Architecture

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
Aurora Renderer
 ├── Camera
 ├── Renderer
 ├── Shader System
 ├── Mesh
 ├── Texture
 ├── Physics
 ├── UI
 └── Scene
     │
     ▼
 GPU
```

---

# 🌌 Astrophysics Simulation

Aurora includes several physically inspired visualization techniques.

## 🌠 Paczyński–Wiita Potential

Pseudo-Newtonian approximation used to simulate particle motion near compact massive objects.

## 🔭 Gravitational Lensing

Weak-field Schwarzschild approximation for real-time light deflection.

## 🌀 Frame Dragging

Approximate Kerr-inspired rotational effects applied to particle trajectories.

## 🌈 Relativistic Doppler Effect

Dynamic redshift and blueshift effects based on relative velocity.

## 🕳️ Space Curvature

Procedural mesh generation inspired by **Flamm's Paraboloid**.

---

# ⚙️ Performance Goals

Aurora emphasizes predictable performance through:

- Vertex Buffer Objects (VBO)
- Vertex Array Objects (VAO)
- Shader-based rendering
- Explicit GPU resource management
- Cache-friendly data layouts
- Reduced CPU overhead
- Batch rendering
- Modern OpenGL pipeline

---

# 💻 Requirements

| Component | Minimum | Recommended |
|------------|----------|-------------|
| Operating System | Windows 10 (64-bit) | Windows 11 (64-bit) |
| CPU | Intel Core i5 / Ryzen 5 | Intel Core i7 / Ryzen 7 |
| RAM | 8 GB | 16 GB |
| GPU | GTX 1050 / RX 560 | RTX 3060 / RX 6600 |
| API | OpenGL 3.3 | OpenGL 4.6 |

---

# 🔧 Dependencies

- SDL3
- GLAD
- GLM
- Assimp
- Dear ImGui

---

# 🚀 Building

Clone the repository:

```bash
git clone https://github.com/yourusername/AuroraVFXEngine.git
```

Configure your project using **C++23** and link against:

```text
SDL3.lib
assimp.lib
opengl32.lib
```

Compile in **Release x64**.

---

# 📂 Project Structure

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

# 🗺️ Roadmap

- [x] OpenGL Renderer
- [x] GLSL Shader System
- [x] ImGui Integration
- [x] Assimp Mesh Loader
- [ ] Deferred Rendering
- [ ] HDR Rendering
- [ ] Shadow Mapping
- [ ] Bloom
- [ ] Physically Based Rendering (PBR)
- [ ] Compute Shader Backend
- [ ] Vulkan Renderer
- [ ] GPU Ray Marching
- [ ] SPH Fluid Simulation
- [ ] Multi-threaded Renderer
- [ ] Linux Support

---

# 🤝 Contributing

Contributions are welcome!

You can help by:

- Reporting bugs
- Improving performance
- Optimizing shaders
- Refactoring engine systems
- Improving documentation
- Implementing new rendering techniques

Please open an Issue or Pull Request.

---

# 📜 License

This project is licensed under the **GNU 3.0 License**.

---

<p align="center">

**Aurora VFX & Astro-Sim Engine**

*Built with modern C++, OpenGL and a passion for computer graphics.*

</p>
