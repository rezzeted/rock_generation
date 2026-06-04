# Rock Fracturing - WebAssembly Port

A real-time 3D rock fracturing simulation ported to WebAssembly using Emscripten and C++, with a Next.js frontend and Three.js rendering.

![Rock Fracturing](https://img.shields.io/badge/WebAssembly-C%2B%2B-654FF0?logo=wasm) ![Next.js](https://img.shields.io/badge/Next.js-16-black?logo=next.js) ![Three.js](https://img.shields.io/badge/Three.js-r170-black?logo=three.js)

Based on the paper: **"Modeling Rocky Scenery using Implicit Blocks"** (Paris et al., The Visual Computer 2020)

Original repository: [aparis69/Rock-fracturing](https://github.com/aparis69/Rock-fracturing)

## Features

- **4 Fracture Types**: Equidimensional, Tabular, Rhombohedral, Polyhedral
- **Real-time Generation**: Rock meshes generated in WebAssembly at interactive speeds
- **3D Viewer**: OrbitControls, PBR materials, soft shadows, auto-rotation
- **Adjustable Parameters**: Resolution, tile size, random seed
- **Procedural Textures**: Perlin noise-based warping (no external texture files needed)

## Architecture

```
wasm/rock-fracturing/       # C++ source code (Emscripten)
  rock_fracturing.cpp       # Complete implementation (vec, noise, blocks, MC)
  mc_tables.inc             # Marching Cubes lookup table
  convhull_3d_impl.inc      # Convex hull implementation

public/wasm/                # Compiled WASM output
  rock_fracturing.js        # Emscripten JS loader
  rock_fracturing.wasm      # WebAssembly binary

src/
  app/page.tsx              # Main page
  components/rock-fracturing/
    RockViewer.tsx           # Three.js 3D viewer
    ControlPanel.tsx         # Parameter controls
    useRockFracturing.ts    # WASM integration hook
```

## How It Works

1. **Block Generation**: Poisson-distributed sample points define block centers
2. **Fracture Planes**: Circular fractures are placed based on the selected type
3. **Cluster Computation**: BFS clustering groups points separated by fractures
4. **SDF Construction**: Each block cluster becomes a smooth signed distance field
5. **Warping**: Procedural Perlin noise warping adds surface detail
6. **Marching Cubes**: The SDF is polygonized into a triangle mesh
7. **Rendering**: Three.js renders the mesh with PBR materials and shadows

## Building the WASM Module

Requires [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html):

```bash
cd wasm/rock-fracturing
emcc rock_fracturing.cpp \
  -o rock_fracturing.js \
  -s EXPORTED_FUNCTIONS='["_generateRock","_getVertices","_getNormals","_getIndices","_getVertexCount","_getTriangleCount","_getVertexDataSize","_getIndexDataSize"]' \
  -s EXPORTED_RUNTIME_METHODS='["HEAPF32","HEAPU32","ccall","cwrap"]' \
  -s MODULARIZE=1 \
  -s EXPORT_NAME='createRockFracturingModule' \
  -O3 \
  -s ALLOW_MEMORY_GROWTH=1

cp rock_fracturing.js ../../public/wasm/
cp rock_fracturing.wasm ../../public/wasm/
```

## Running the Frontend

```bash
npm install
npm run dev
```

Open http://localhost:3000

## License

The original Rock Fracturing project is by [Adrien Paris](https://github.com/aparis69). This WebAssembly port follows the same academic/research use terms.
