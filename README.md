# Crowd Optimization

[![Engine](https://img.shields.io/badge/Engine-Unreal%20Engine%205-blue.svg)](https://www.unrealengine.com/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B%20%7C%20Blueprints-orange.svg)]()
[![Paradigm](https://img.shields.io/badge/Architecture-Data--Oriented%20Design%20(DOD)-green.svg)]()

A performance benchmarking and systems architecture project designed to isolate and quantify the real-world performance gains of **Data-Oriented Design (DOD)** versus traditional Object-Oriented Programming (OOP) in Unreal Engine.

---

## Key Highlights

* **Architectural Isolation Across 3 Versions:** Compares identical crowd workloads (100–250 zombies in Idle vs. Shooting states) implemented in **Naive Blueprints**, **Naive C++ (OOP)**, and **Data-Oriented C++ (DOD)**.
* **Cache-Friendly Memory Layout (SoA):** Refactored entity logic from standard Array of Structures (AoS) to Structure of Arrays (SoA), ensuring contiguous memory layout and maximizing CPU L1/L2 cache hits.
* **Custom Throttled Pathfinding & Pooling:** Implemented zero-allocation Object Pooling and a custom pathfinding update system using staggered timers and local threat prioritization to prevent frame spikes.
* **Game Thread Latency Halved:** Slashed Game Thread execution time from **24.0ms down to 12.3ms**, doubling average framerate from **~41 FPS to 81 FPS** and maintaining a stable 60 FPS at 250+ active entities.

---

## 📊 Benchmark Results (100 Active Agents)

| Version | Architecture / Paradigm | Game Thread (Avg) | Framerate (Avg) | Lows (Min FPS) |
| :--- | :--- | :--- | :--- | :--- |
| **V1: Blueprint** | OOP (`ACharacter` encapsulation) | 24.0 ms | 41.2 FPS | 26.1 FPS *(Stutters in Combat)* |
| **V2: Native C++** | OOP (C++ & Event Delegates) | 23.7 ms | 41.7 FPS | 36.8 FPS *(Stable Pacing)* |
| **V3: Data-Oriented** | **DOD (SoA + Pooling + Throttling)** | **12.3 ms** | **81.0 FPS** | **66.9 FPS** *(2x Performance)* |

> **Key Takeaway:** Migrating from Blueprint to C++ stabilized framerate dips, but moving to Data-Oriented Design delivered the true architectural breakthrough by eliminating `ACharacter` overhead.

---

## 🚀 Getting Started

### Prerequisites
* Unreal Engine 5.x
* Visual Studio 2022 (with C++ Game Development workload)

### Installation & Execution

1. **Clone the repository:**
   ```bash
   git clone https://github.com/ThibaultClodion/Crowd-Optimization.git
   ```

2. **Generate Project Files:**
   * Right-click `CrowdOptimization.uproject` $\rightarrow$ **Generate Visual Studio project files**.
   * Open the `.sln` and build the solution in `Development Editor` mode.

3. **Test the levels:**
   * Open `CrowdOptimization.uproject` in Unreal Editor.
   * Open the test level or the demo level then press **Play in Editor (PIE)**

---

## Links
* **Portfolio Demo:** [Crowd Optimization Demo](https://portfolio-thibaultclodion.my.canva.site/portfolio/crowd-optimization)
