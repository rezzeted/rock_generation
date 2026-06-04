'use client';

import { Button } from '@/components/ui/button';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Label } from '@/components/ui/label';
import { Slider } from '@/components/ui/slider';
import { Select, SelectContent, SelectItem, SelectTrigger, SelectValue } from '@/components/ui/select';
import { Switch } from '@/components/ui/switch';
import { Badge } from '@/components/ui/badge';
import { Separator } from '@/components/ui/separator';
import { Loader2, RotateCcw, Sparkles, Dices } from 'lucide-react';
import type { GenerationParams } from './useRockFracturing';

interface ControlPanelProps {
  params: GenerationParams;
  onParamsChange: (params: GenerationParams) => void;
  onGenerate: () => void;
  onRandomSeed: () => void;
  isLoading: boolean;
  isModuleReady: boolean;
  generationTime: number;
  meshData: { vertexCount: number; triangleCount: number } | null;
  wireframe: boolean;
  onWireframeChange: (v: boolean) => void;
  autoRotate: boolean;
  onAutoRotateChange: (v: boolean) => void;
}

const FRACTURE_TYPES = [
  { value: '0', label: 'Equidimensional', description: 'Three orthogonal joint sets', icon: '◻️' },
  { value: '1', label: 'Tabular', description: 'Parallel bedding planes', icon: '▬' },
  { value: '2', label: 'Rhombohedral', description: 'Oblique-shaped equidimensional', icon: '◇' },
  { value: '3', label: 'Polyhedral', description: 'Irregular jointing', icon: '⬡' },
  { value: '4', label: 'Cliff', description: 'Rocky wall from fractured blocks', icon: '⛰️' },
] as const;

export function ControlPanel({
  params,
  onParamsChange,
  onGenerate,
  onRandomSeed,
  isLoading,
  isModuleReady,
  generationTime,
  meshData,
  wireframe,
  onWireframeChange,
  autoRotate,
  onAutoRotateChange,
}: ControlPanelProps) {
  const selectedType = FRACTURE_TYPES.find(t => t.value === String(params.fractureType));

  return (
    <div className="flex flex-col gap-4">
      {/* Fracture Type */}
      <Card className="border-stone-700/50 bg-stone-900/80">
        <CardHeader className="pb-3">
          <CardTitle className="text-sm font-medium text-stone-300 flex items-center gap-2">
            <Sparkles className="h-4 w-4 text-amber-500" />
            Fracture Type
          </CardTitle>
        </CardHeader>
        <CardContent className="space-y-3">
          <Select
            value={String(params.fractureType)}
            onValueChange={(v) => onParamsChange({ ...params, fractureType: parseInt(v) })}
          >
            <SelectTrigger className="w-full bg-stone-800 border-stone-600 text-stone-200">
              <SelectValue />
            </SelectTrigger>
            <SelectContent className="bg-stone-800 border-stone-600">
              {FRACTURE_TYPES.map((type) => (
                <SelectItem key={type.value} value={type.value} className="text-stone-200 focus:bg-stone-700 focus:text-stone-100">
                  <span className="mr-2">{type.icon}</span>
                  {type.label}
                </SelectItem>
              ))}
            </SelectContent>
          </Select>
          {selectedType && (
            <p className="text-xs text-stone-500">{selectedType.description}</p>
          )}
        </CardContent>
      </Card>

      {/* Generation Parameters */}
      <Card className="border-stone-700/50 bg-stone-900/80">
        <CardHeader className="pb-3">
          <CardTitle className="text-sm font-medium text-stone-300 flex items-center gap-2">
            <Dices className="h-4 w-4 text-amber-500" />
            Parameters
          </CardTitle>
        </CardHeader>
        <CardContent className="space-y-5">
          {/* Resolution */}
          <div className="space-y-2">
            <div className="flex justify-between items-center">
              <Label className="text-xs text-stone-400">Resolution</Label>
              <Badge variant="secondary" className="text-xs bg-stone-700 text-stone-300">
                {params.resolution}
              </Badge>
            </div>
            <Slider
              value={[params.resolution]}
              onValueChange={([v]) => onParamsChange({ ...params, resolution: v })}
              min={40}
              max={200}
              step={10}
              className="py-1"
            />
            <p className="text-xs text-stone-600">Higher = more detail but slower</p>
          </div>

          {/* Tile Size */}
          <div className="space-y-2">
            <div className="flex justify-between items-center">
              <Label className="text-xs text-stone-400">Tile Size</Label>
              <Badge variant="secondary" className="text-xs bg-stone-700 text-stone-300">
                {params.tileSize.toFixed(0)}
              </Badge>
            </div>
            <Slider
              value={[params.tileSize]}
              onValueChange={([v]) => onParamsChange({ ...params, tileSize: v })}
              min={10}
              max={40}
              step={2}
              className="py-1"
            />
          </div>

          {/* Seed */}
          <div className="space-y-2">
            <div className="flex justify-between items-center">
              <Label className="text-xs text-stone-400">Random Seed</Label>
              <Button
                variant="ghost"
                size="sm"
                onClick={onRandomSeed}
                className="h-6 px-2 text-xs text-amber-400 hover:text-amber-300"
              >
                <RotateCcw className="h-3 w-3 mr-1" />
                Randomize
              </Button>
            </div>
            <Slider
              value={[params.seed]}
              onValueChange={([v]) => onParamsChange({ ...params, seed: v })}
              min={1}
              max={9999}
              step={1}
              className="py-1"
            />
            <p className="text-xs text-stone-600 text-center font-mono">#{params.seed}</p>
          </div>
        </CardContent>
      </Card>

      {/* Display Options */}
      <Card className="border-stone-700/50 bg-stone-900/80">
        <CardHeader className="pb-3">
          <CardTitle className="text-sm font-medium text-stone-300">
            Display
          </CardTitle>
        </CardHeader>
        <CardContent className="space-y-3">
          <div className="flex items-center justify-between">
            <Label className="text-xs text-stone-400">Wireframe</Label>
            <Switch checked={wireframe} onCheckedChange={onWireframeChange} />
          </div>
          <div className="flex items-center justify-between">
            <Label className="text-xs text-stone-400">Auto Rotate</Label>
            <Switch checked={autoRotate} onCheckedChange={onAutoRotateChange} />
          </div>
        </CardContent>
      </Card>

      {/* Generate Button */}
      <Button
        onClick={onGenerate}
        disabled={isLoading || !isModuleReady}
        className="w-full bg-amber-600 hover:bg-amber-500 text-white font-semibold py-5 text-base shadow-lg shadow-amber-600/20 transition-all"
        size="lg"
      >
        {isLoading ? (
          <>
            <Loader2 className="h-5 w-5 mr-2 animate-spin" />
            Generating...
          </>
        ) : !isModuleReady ? (
          <>
            <Loader2 className="h-5 w-5 mr-2 animate-spin" />
            Loading WASM...
          </>
        ) : (
          <>
            <Sparkles className="h-5 w-5 mr-2" />
            {params.fractureType === 4 ? 'Generate Cliff' : 'Generate Rock'}
          </>
        )}
      </Button>

      {/* Stats */}
      {meshData && (
        <Card className="border-stone-700/50 bg-stone-900/80">
          <CardContent className="pt-4 space-y-2">
            <div className="flex justify-between text-xs">
              <span className="text-stone-500">Vertices</span>
              <span className="text-stone-300 font-mono">{meshData.vertexCount.toLocaleString()}</span>
            </div>
            <div className="flex justify-between text-xs">
              <span className="text-stone-500">Triangles</span>
              <span className="text-stone-300 font-mono">{meshData.triangleCount.toLocaleString()}</span>
            </div>
            <Separator className="bg-stone-700" />
            <div className="flex justify-between text-xs">
              <span className="text-stone-500">Generation Time</span>
              <span className="text-amber-400 font-mono">{generationTime.toFixed(0)}ms</span>
            </div>
          </CardContent>
        </Card>
      )}
    </div>
  );
}
