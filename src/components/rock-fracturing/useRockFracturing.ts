'use client';

import { useState, useRef, useCallback, useEffect } from 'react';

// Type definitions for the WASM module
interface RockFracturingModule {
  _generateRock: (fractureType: number, resolution: number, seed: number, tileSize: number) => number;
  _generateCliff: (fractureType: number, resolution: number, seed: number, tileSize: number) => number;
  _getVertices: () => number;
  _getNormals: () => number;
  _getIndices: () => number;
  _getVertexCount: () => number;
  _getTriangleCount: () => number;
  _getVertexDataSize: () => number;
  _getIndexDataSize: () => number;
  HEAPF32: Float32Array;
  HEAPU32: Uint32Array;
}

export interface MeshData {
  vertices: Float32Array;
  normals: Float32Array;
  indices: Uint32Array;
  vertexCount: number;
  triangleCount: number;
}

export interface GenerationParams {
  fractureType: number;
  resolution: number;
  seed: number;
  tileSize: number;
}

// Declare the global function created by the Emscripten module
declare global {
  var createRockFracturingModule: (config?: Record<string, unknown>) => Promise<RockFracturingModule>;
}

export function useRockFracturing() {
  const [isLoading, setIsLoading] = useState(false);
  const [isModuleReady, setIsModuleReady] = useState(false);
  const [meshData, setMeshData] = useState<MeshData | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [generationTime, setGenerationTime] = useState<number>(0);
  const moduleRef = useRef<RockFracturingModule | null>(null);

  // Load WASM module via script tag
  useEffect(() => {
    let mounted = true;

    async function loadModule() {
      try {
        // Check if the script is already loaded
        if (typeof window.createRockFracturingModule === 'function') {
          const rockModule = await window.createRockFracturingModule({
            locateFile: (path: string) => `/wasm/${path}?v=4`
          });
          if (mounted) {
            moduleRef.current = rockModule;
            setIsModuleReady(true);
          }
          return;
        }

        // Load the script dynamically
        const script = document.createElement('script');
        script.src = '/wasm/rock_fracturing.js?v=4';
        script.async = true;

        await new Promise<void>((resolve, reject) => {
          script.onload = () => resolve();
          script.onerror = () => reject(new Error('Failed to load WASM script'));
          document.head.appendChild(script);
        });

        // Initialize the module
        if (typeof window.createRockFracturingModule !== 'function') {
          throw new Error('WASM module factory not found after script load');
        }

        const rockModule = await window.createRockFracturingModule({
          locateFile: (path: string) => `/wasm/${path}?v=4`
        });

        if (mounted) {
          moduleRef.current = rockModule;
          setIsModuleReady(true);
        }
      } catch (err) {
        if (mounted) {
          setError(`Failed to load WASM module: ${err instanceof Error ? err.message : String(err)}`);
        }
      }
    }

    loadModule();

    return () => { mounted = false; };
  }, []);

  const generate = useCallback(async (params: GenerationParams) => {
    if (!moduleRef.current) {
      console.error('[WASM] Module not loaded');
      setError('WASM module not loaded');
      return null;
    }

    setIsLoading(true);
    setError(null);

    // Use setTimeout to allow UI to update before heavy computation
    await new Promise(resolve => setTimeout(resolve, 50));

    try {
      const mod = moduleRef.current;
      const startTime = performance.now();

      console.log('[WASM] Starting generation, fractureType:', params.fractureType, 'has _generateCliff:', !!mod._generateCliff);

      // Use generateCliff for cliff type (4), generateRock for others
      const triCount = params.fractureType === 4
        ? mod._generateCliff(
            params.fractureType,
            params.resolution,
            params.seed,
            params.tileSize
          )
        : mod._generateRock(
            params.fractureType,
            params.resolution,
            params.seed,
            params.tileSize
          );

      const endTime = performance.now();
      console.log('[WASM] Generation complete, triCount:', triCount);
      setGenerationTime(endTime - startTime);

      if (triCount <= 0) {
        setError('Generation produced no triangles. Try different parameters.');
        setMeshData(null);
        setIsLoading(false);
        return null;
      }

      const vertexDataSize = mod._getVertexDataSize();
      const indexDataSize = mod._getIndexDataSize();
      const verticesPtr = mod._getVertices();
      const normalsPtr = mod._getNormals();
      const indicesPtr = mod._getIndices();

      const vertices = new Float32Array(vertexDataSize);
      const normals = new Float32Array(vertexDataSize);
      const indices = new Uint32Array(indexDataSize);

      for (let i = 0; i < vertexDataSize / 4; i++) {
        vertices[i] = mod.HEAPF32[verticesPtr / 4 + i];
        normals[i] = mod.HEAPF32[normalsPtr / 4 + i];
      }
      for (let i = 0; i < indexDataSize / 4; i++) {
        indices[i] = mod.HEAPU32[indicesPtr / 4 + i];
      }

      const data: MeshData = {
        vertices,
        normals,
        indices,
        vertexCount: mod._getVertexCount(),
        triangleCount: triCount,
      };

      setMeshData(data);
      setIsLoading(false);
      return data;
    } catch (err) {
      setError(`Generation failed: ${err instanceof Error ? err.message : String(err)}`);
      setMeshData(null);
      setIsLoading(false);
      return null;
    }
  }, []);

  return {
    isLoading,
    isModuleReady,
    meshData,
    error,
    generationTime,
    generate,
  };
}
