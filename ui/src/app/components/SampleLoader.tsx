import { useState } from 'react';
import { useJuceBridge } from '../../hooks/useJuceBridge';
import type { ThemeColor } from '../App';
import { Music, PlayCircle } from 'lucide-react';

interface SampleLoaderProps {
  colorTheme: 'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink';
}

export function SampleLoader({ colorTheme }: SampleLoaderProps) {
    const { loadSample } = useJuceBridge();
    const [selectedSample, setSelectedSample] = useState<string | null>(null);

    const themeColors = {
        green: { panel: 'bg-green-950/30', border: 'border-green-900/50', text: 'text-green-300', textSecondary: 'text-green-300/80', buttonBg: 'bg-green-900/30 hover:bg-green-800/50', activeBg: 'bg-green-600', textActive: 'text-white' },
        blue: { panel: 'bg-blue-950/30', border: 'border-blue-900/50', text: 'text-blue-300', textSecondary: 'text-blue-300/80', buttonBg: 'bg-blue-900/30 hover:bg-blue-800/50', activeBg: 'bg-blue-600', textActive: 'text-white' },
        purple: { panel: 'bg-purple-950/30', border: 'border-purple-900/50', text: 'text-purple-300', textSecondary: 'text-purple-300/80', buttonBg: 'bg-purple-900/30 hover:bg-purple-800/50', activeBg: 'bg-purple-600', textActive: 'text-white' },
        red: { panel: 'bg-red-950/30', border: 'border-red-900/50', text: 'text-red-300', textSecondary: 'text-red-300/80', buttonBg: 'bg-red-900/30 hover:bg-red-800/50', activeBg: 'bg-red-600', textActive: 'text-white' },
        orange: { panel: 'bg-orange-950/30', border: 'border-orange-900/50', text: 'text-orange-300', textSecondary: 'text-orange-300/80', buttonBg: 'bg-orange-900/30 hover:bg-orange-800/50', activeBg: 'bg-orange-600', textActive: 'text-white' },
        cyan: { panel: 'bg-cyan-950/30', border: 'border-cyan-900/50', text: 'text-cyan-300', textSecondary: 'text-cyan-300/80', buttonBg: 'bg-cyan-900/30 hover:bg-cyan-800/50', activeBg: 'bg-cyan-600', textActive: 'text-white' },
        pink: { panel: 'bg-pink-950/30', border: 'border-pink-900/50', text: 'text-pink-300', textSecondary: 'text-pink-300/80', buttonBg: 'bg-pink-900/30 hover:bg-pink-800/50', activeBg: 'bg-pink-600', textActive: 'text-white' },
    };

    const theme = themeColors[colorTheme];

    const samples = [
        "amen140.wav",
        "amen160.wav",
        "amen180.wav",
        "amen200.wav"
    ];

    const handleLoad = (filename: string) => {
        setSelectedSample(filename);
        if (loadSample) {
            loadSample(filename);
        }
    };

    return (
        <div className={`flex flex-col gap-4 ${theme.panel} rounded-2xl p-6 border ${theme.border} backdrop-blur-sm shadow-xl`}>
            <div className="flex items-center gap-2 mb-2">
                <Music className={`w-5 h-5 ${theme.text}`} />
                <h2 className={`text-lg font-bold ${theme.text}`}>Built-in Samples</h2>
            </div>
            
            <div className="grid grid-cols-2 gap-2">
                {samples.map(sample => (
                    <button
                        key={sample}
                        onClick={() => handleLoad(sample)}
                        className={`flex items-center justify-between px-4 py-3 rounded-lg transition-all border ${
                            selectedSample === sample 
                                ? `${theme.activeBg} ${theme.textActive} border-transparent` 
                                : `${theme.buttonBg} ${theme.text} border-transparent hover:border-white/10`
                        }`}
                    >
                        <span className="text-sm font-medium">{sample.replace('.wav', '')}</span>
                        {selectedSample === sample && <PlayCircle className="w-4 h-4" />}
                    </button>
                ))}
            </div>
            <p className={`text-xs ${theme.textSecondary} opacity-70`}>
                Selecting a sample will automatically switch BPM Sync to "Manual" and set the BPM.
            </p>
        </div>
    );
}
