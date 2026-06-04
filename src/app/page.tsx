'use client';

import { useState, useCallback } from 'react';
import { RockViewer } from '@/components/rock-fracturing/RockViewer';
import { ControlPanel } from '@/components/rock-fracturing/ControlPanel';
import { useRockFracturing, type GenerationParams } from '@/components/rock-fracturing/useRockFracturing';
import { Badge } from '@/components/ui/badge';
import { Github, BookOpen } from 'lucide-react';

const DEFAULT_PARAMS: GenerationParams = {
  fractureType: 0,
  resolution: 60,
  seed: 42,
  tileSize: 20,
};

export default function Home() {
  const { isLoading, isModuleReady, meshData, error, generationTime, generate } = useRockFracturing();
  const [params, setParams] = useState<GenerationParams>(DEFAULT_PARAMS);
  const [wireframe, setWireframe] = useState(false);
  const [autoRotate, setAutoRotate] = useState(true);

  const handleGenerate = useCallback(() => {
    generate(params);
  }, [generate, params]);

  const handleRandomSeed = useCallback(() => {
    setParams(p => ({ ...p, seed: Math.floor(Math.random() * 9999) + 1 }));
  }, []);

  return (
    <div className="min-h-screen flex flex-col bg-[#0f0f1a] text-white">
      {/* Header */}
      <header className="flex items-center justify-between px-4 py-3 border-b border-stone-800/60 bg-stone-950/50 backdrop-blur-md">
        <div className="flex items-center gap-3">
          <div className="w-8 h-8 rounded-lg bg-amber-600/20 flex items-center justify-center text-lg">
            🪨
          </div>
          <div>
            <h1 className="text-sm font-bold text-stone-200 tracking-wide">Rock Fracturing</h1>
            <p className="text-[10px] text-stone-500">Implicit Block Modeling • WebAssembly</p>
          </div>
        </div>
        <div className="flex items-center gap-3">
          <Badge
            variant="outline"
            className={`text-[10px] px-2 py-0.5 ${
              isModuleReady
                ? 'border-emerald-600/50 text-emerald-400'
                : 'border-amber-600/50 text-amber-400'
            }`}
          >
            <span className={`inline-block w-1.5 h-1.5 rounded-full mr-1.5 ${
              isModuleReady ? 'bg-emerald-400' : 'bg-amber-400 animate-pulse'
            }`} />
            {isModuleReady ? 'WASM Ready' : 'Loading WASM...'}
          </Badge>
          <a
            href="https://github.com/aparis69/Rock-fracturing"
            target="_blank"
            rel="noopener noreferrer"
            className="text-stone-500 hover:text-stone-300 transition-colors"
          >
            <Github className="h-4 w-4" />
          </a>
          <a
            href="https://aparis69.github.io/public_html/projects/paris2020_Blocks.html"
            target="_blank"
            rel="noopener noreferrer"
            className="text-stone-500 hover:text-stone-300 transition-colors"
          >
            <BookOpen className="h-4 w-4" />
          </a>
        </div>
      </header>

      {/* Main Content */}
      <main className="flex-1 flex overflow-hidden">
        {/* 3D Viewer */}
        <div className="flex-1 relative">
          <RockViewer
            meshData={meshData}
            isLoading={isLoading}
            wireframe={wireframe}
            autoRotate={autoRotate}
          />
          {/* Error overlay */}
          {error && (
            <div className="absolute bottom-4 left-4 right-4">
              <div className="bg-red-900/80 backdrop-blur-sm border border-red-700/50 rounded-lg px-4 py-3 text-red-200 text-sm">
                {error}
              </div>
            </div>
          )}
          {/* Info overlay */}
          {meshData && !isLoading && (
            <div className="absolute top-4 left-4">
              <div className="bg-black/40 backdrop-blur-sm rounded-lg px-3 py-2 text-xs text-stone-400 space-y-0.5">
                <p>🖱️ Left-click + drag to rotate</p>
                <p>🔍 Scroll to zoom</p>
                <p>✋ Right-click + drag to pan</p>
              </div>
            </div>
          )}
        </div>

        {/* Control Panel */}
        <aside className="w-72 border-l border-stone-800/60 bg-stone-950/30 backdrop-blur-md overflow-y-auto p-3 custom-scrollbar">
          <ControlPanel
            params={params}
            onParamsChange={setParams}
            onGenerate={handleGenerate}
            onRandomSeed={handleRandomSeed}
            isLoading={isLoading}
            isModuleReady={isModuleReady}
            generationTime={generationTime}
            meshData={meshData ? { vertexCount: meshData.vertexCount, triangleCount: meshData.triangleCount } : null}
            wireframe={wireframe}
            onWireframeChange={setWireframe}
            autoRotate={autoRotate}
            onAutoRotateChange={setAutoRotate}
          />
        </aside>
      </main>

      {/* Footer */}
      <footer className="px-4 py-2 border-t border-stone-800/60 bg-stone-950/50 backdrop-blur-md">
        <div className="flex items-center justify-between text-[10px] text-stone-600">
          <p>
            Based on &quot;Modeling Rocky Scenery using Implicit Blocks&quot; (Paris et al., TVC 2020)
          </p>
          <p>Ported to WebAssembly with Emscripten</p>
        </div>
      </footer>
    </div>
  );
}
