

# ✨ Aurora VFX & Astro-Sim Engine

## Low-Level, Open Source Graphics & Astrophysics Simulation Engine.

The **Aurora VFX Engine** is a high-performance **Graphics Renderer and Relativistic Astrophysics Simulator** built entirely from scratch in C++. By avoiding the overhead of commercial engines, Aurora communicates directly with the GPU via low-level API states, focusing on extreme memory optimization, custom matrix manipulation, and mass parallelization of physical equations.

> [!IMPORTANT]
> **Current Version:** `v0.7.0 (Beta - Ultimate Pro)`  
> **Target Standard:** C++23 / OpenGL Core Profile 3.3+  
> **Architect:** Wenderson Dias (*Siliconarch Softwares*)


![demo gif](https://raw.githubusercontent.com/welabsdev/AuroraVFXEngine/refs/heads/main/demo.gif)
---

## Technical Stack & Core Dependencies

Aurora relies on a minimalistic and carefully chosen set of open-source libraries to maintain total control over hardware allocation, preventing hidden garbage collection cycles.

### 1. External Graphics & System Backends
* **SDL3 (Simple DirectMedia Layer v3.0+):** Handles OS-level window abstraction, native hardware inputs (6DOF mouse/keyboard interaction), high-resolution timing, and OpenGL context creation.
* **GLAD (v0.1.36+):** Multi-language OpenGL Function Pointer Loader. Configured strictly for **OpenGL 3.3+ Core Profile** to enforce modern programmable pipeline paradigms (disabling legacy immediate-mode functions).

### 2. Math & Asset Parsing
* **GLM (OpenGL Mathematics v0.9.9+):** A header-only C++ mathematics library structured strictly tailored to GLSL specifications. Used for 3D coordinate transformations, Quaternion-based camera rotation, and viewport projections.
* **Assimp (Open Asset Import Library v5.4+):** Advanced 3D asset parsing core. Abstracting mesh reading, vertex arrays, and geometric buffers (currently optimizing raw `.obj` parsing into custom Vertex Buffer Objects).

### 3. Internal Tools & Runtime Environment
* **Dear ImGui (v1.90+):** Bloat-free Immediate Mode Graphical User Interface. Rendered natively inside the OpenGL pipeline to expose engine diagnostics, VRAM manipulation hooks, and physical constants adjustments.
* **AURA Live Parser:** Standard C++ Runtime Engine designed to parse and hot-reload local `.aura` scripts directly into RAM, bypassing disk-write delays and compiling logic live onto the GPU state machine.

---

## Architecture & Relativistic Physics Shaders

All physics calculations are processed mass-parallelized directly inside **GLSL (OpenGL Shading Language)** kernels to handle heavy loads without choking the CPU.

* **Gravitational Physics (Paczyński–Wiita):** Implements the pseudo-Newtonian potential `F = -GM / (r - r_s)²` to simulate the Inner Stable Circular Orbit (ISCO) around cosmic singularities.
* **Einsteinian Gravitational Lensing:** Light bending is calculated dynamically per-vertex based on Schwarzschild radius deflection: `α ≈ 4GM / (c²r)`.
* **Kerr Metric Frame-Dragging:** Simulates space-time twisting inside the Ergosphere (*Lense-Thirring effect*), affecting particle velocity vectors based on angular momentum `J`.
* **Relativistic Doppler Effect:** Direct runtime shifting of vertex colors. Particles rushing toward the viewport encounter *Blueshift*, while escaping particles trigger *Redshift*.
* **Space-Time Topology:** Generates a real-time procedural 3D mesh mapping Flamm's Paraboloid equations: `z = -2√(r_s(r - r_s))`.

---

## Hardware & System Requirements

| Specification | Minimum Requirement | Recommended Specification |
| :--- | :--- | :--- |
| **Operating System** | Windows 10/11 (64-bit) | Windows 11 (64-bit) |
| **Processor (CPU)** | Intel Core i5 / AMD Ryzen 5 | Intel Core i7 / AMD Ryzen 7 (with high single-core clock) |
| **Memory (RAM)** | 8 GB RAM | 16 GB RAM |
| **Graphics (GPU)** | Dedicated NVIDIA GTX 1050 / AMD RX 560 | NVIDIA RTX 3060 / AMD RX 6600 or higher |
| **API Support** | OpenGL 3.3 Core Profile | OpenGL 4.6 Core Profile |
| **VRAM Extensions** | Standard Driver Support | `GL_NVX_gpu_memory_info` or `GL_ATI_meminfo` |

---

## 🛠️ Compilation Guide: Visual Studio 2026

Follow these exact steps to set up, link, and compile the Aurora Engine using the **Visual Studio 2026** toolset.

### 1. Prerequisites & Dependencies Structure
Download the development binaries/headers of the dependencies and organize them in a dedicated folder (e.g., `C:\SDKs\`) or inside your project root directory:


AuroraEngine/
│
├── dependencies/
│   ├── include/ (Add GLM, SDL3, Assimp, GLAD, and ImGui headers here)
│   └── lib/     (Add SDL3.lib, assimp-vc143-mt.lib here)
├── src/         (Your .cpp and .h engine source files)
└── glad.c       (The native C loader file generated via glad.dav1d.de)
2. Project Generation in Visual Studio 2026
Open Visual Studio 2026 and select Create a new project.

Select Empty Project (C++) and click Next. Name it AuroraEngine.

Right-click on the project in Solution Explorer -> Add -> Existing Item and select all your .cpp, .h files, including glad.c and the core ImGui implementation files (imgui*.cpp).

3. Compiler & Linker Configuration
Right-click on your project name in the Solution Explorer and choose Properties. Set the Configuration dropdown to All Configurations and Platform to x64.

A. Set Language Standard (C++23)
Navigate to Configuration Properties -> General -> C++ Language Standard.

Set it to ISO C++23 Standard (/std:c++23).

B. Include Headers
Navigate to Configuration Properties -> C/C++ -> General -> Additional Include Directories.

Paste the path to your header files folder: $(ProjectDir)dependencies\include (or your absolute path like C:\SDKs\include).

C. Setup Library Binaries
Navigate to Configuration Properties -> Linker -> General -> Additional Library Directories.

Paste the path to your compiled static libraries: $(ProjectDir)dependencies\lib.

D. Link Inputs
Navigate to Configuration Properties -> Linker -> Input -> Additional Dependencies.

Click Edit and add the following lines manually:

Plaintext
SDL3.lib
assimp-vc143-mt.lib
opengl32.lib
4. Build and Execution
Change the deployment target in the top toolbar from Debug to Release and architecture to x64.

Press Ctrl + Shift + B to compile the engine core.

CRITICAL STEP: Copy SDL3.dll and assimp-vc143-mt.dll from your dependency folder and paste them directly into the output build folder where your AuroraEngine.exe was generated (usually $(SolutionDir)x64\Release\).

Press F5 to execute.

🤝 Open Source Contribution
Aurora is an independent project dedicated to the low-level graphics community. Contributions are highly welcome!

Bug Reports: File an issue describing hardware specifications, driver versions, and GLSL error logs.

GPU Optimization: Pull Requests focusing on reducing draw calls, thread orchestration, or custom allocator layouts are highly appreciated.

Developed with precision, C++, and raw math by Wenderson Dias / Siliconarch Softwares.
