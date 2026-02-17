Multi-Threaded Software Renderer (C++17)
A high-performance CPU-based 3D renderer built from scratch, inspired by the Tiny Renderer curriculum. This project focuses on the low-level mechanics of the graphics pipeline, memory architecture, and high-performance parallel computing.

🚀 Technical & Engineering Highlights
Tile-Based Parallel Rasterization: Screen is partitioned into 16x16 tiles and processed via a thread pool to maximize cache locality and CPU throughput.

Lock-Free Synchronization: Implemented a thread-safe Z-buffer using std::atomic operations, eliminating the performance bottleneck of Mutex locks during concurrent fragment shading.

Raw Binary I/O: Direct manipulation of the TGA (Truevision Graphics Adapter) file format for image output, demonstrating low-level data handling without external image libraries.

Custom Linear Algebra Library:

Built a template-based library for Vectors and Matrices (
2×2
 to 
4×4
).

Column-Major Convention: Matrices are stored and processed in column-major order to align with standard graphics API conventions.

Compile-Time Safety: Extensive use of static_assert and C++ Concepts to validate matrix dimensions and numeric types during compilation.

Perspective-Correct Interpolation: High-fidelity attribute mapping (UVs, Normals, Tangents) using barycentric coordinates adjusted for the 
1/w
 depth component.

🛠 Graphics Pipeline Features
Coordinate Systems: Implemented standard Right-Handed coordinate system with Look-At camera transformations and Perspective projection.

Blinn-Phong Shading: Full lighting model including ambient, diffuse, and specular components.

Advanced Mapping: Support for Normal Mapping (Tangent space) and Specular Mapping to simulate complex surface details.

Shadow Mapping (PCF): Robust shadow generation using Percentage Closer Filtering (
3×3
 kernel) for softened, realistic shadow edges.

Post-Processing (SSAO): Screen Space Ambient Occlusion pass optimized for high memory throughput to simulate global illumination effects.

Alpha Testing: Custom fragment discard logic for handling complex geometry like foliage and transparent textures.

📊 Performance Optimizations
Atomic Counter Tasking: Worker threads fetch tasks via fetch_add, ensuring optimal load balancing across all CPU cores.

Zero-Copy Intent: Optimized data flow using constant references (const T&) to minimize unnecessary memory allocations and object copying.
