import { useState, useEffect } from 'react';
import { WaveformDisplay } from './components/WaveformDisplay';
import { ControlPanel } from './components/ControlPanel';
import { RotateCcw } from 'lucide-react';
import { useJuceBridge } from '../hooks/useJuceBridge';

export default function App() {
  // Always playing in plugin mode
  const isPlaying = true;

  const [activeSlices, setActiveSlices] = useState<Set<number>>(
    new Set(Array.from({ length: 16 }, (_, i) => i)) // All slices active by default
  );
  const [colorTheme, setColorTheme] = useState<'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink'>('green');

  // Hoisted state for status display
  const [originalPlayhead, setOriginalPlayhead] = useState(0);
  const [triggeredPlayhead, setTriggeredPlayhead] = useState<number | null>(null);

  // Use the bridge
  const { parameters, sendParameter, addEventListener, performHardReset } = useJuceBridge();

  // Sync theme with JUCE parameter
  useEffect(() => {
    if (typeof parameters.colorTheme === 'number') {
      const themes: (typeof colorTheme)[] = ['green', 'blue', 'purple', 'red', 'orange', 'cyan', 'pink'];
      const themeName = themes[Math.floor(parameters.colorTheme)];
      if (themeName && themeName !== colorTheme) {
        console.log(`[UI] Theme synced from host: ${themeName}`);
        setColorTheme(themeName);
      }
    }
  }, [parameters.colorTheme]);

  // Listen for JUCE events at App level
  useEffect(() => {
    const removeListener = addEventListener('note', (data: any) => {
      // note1: 0-15 (Triggered / Note Sequence)
      if (typeof data.note1 === 'number' && data.note1 >= 0 && data.note1 <= 15) {
        setTriggeredPlayhead(data.note1);
      }
      // note2: 32-47 (Original)
      if (typeof data.note2 === 'number' && data.note2 >= 32 && data.note2 <= 47) {
        setOriginalPlayhead(data.note2 - 32);
      }

      // Clear triggered playhead after a short delay for visual feedback
      // (Optional: depending on how fast events come in. 
      //  The previous logic might have relied on continuous updates or auto-clear.)
      //  Actually, if note1 is the current playing slice, we might want it to stay lit.
      //  But usually 'note off' isn't sent here? 
      //  Let's keep it simple: just update state.
    });

    return () => {
      removeListener();
    };
  }, [addEventListener]);

  const resetSlices = () => {
    console.log('[UI] Reset Slices clicked');
    setActiveSlices(new Set(Array.from({ length: 16 }, (_, i) => i)));
  };

  const handleThemeChange = (newTheme: typeof colorTheme) => {
    console.log(`[UI] Theme changed to: ${newTheme}`);
    setColorTheme(newTheme);

    // Send to JUCE
    const themes = ['green', 'blue', 'purple', 'red', 'orange', 'cyan', 'pink'];
    const index = themes.indexOf(newTheme);
    if (index >= 0) {
      sendParameter('colorTheme', index);
    }
  };

  // Color theme configurations
  const themeColors = {
    green: {
      bgGradient: 'from-slate-900 via-green-950 to-slate-900',
      borderColor: 'border-green-900/50',
      textPrimary: 'text-green-300',
      textSecondary: 'text-green-300/70',
      textTertiary: 'text-green-300/60',
      buttonBg: 'bg-green-600 hover:bg-green-500',
      buttonSecondaryBg: 'bg-green-900/50 hover:bg-green-800/70',
      panelBg: 'bg-green-950/30',
      accentColor: 'text-green-400',
    },
    blue: {
      bgGradient: 'from-slate-900 via-blue-950 to-slate-900',
      borderColor: 'border-blue-900/50',
      textPrimary: 'text-blue-300',
      textSecondary: 'text-blue-300/70',
      textTertiary: 'text-blue-300/60',
      buttonBg: 'bg-blue-600 hover:bg-blue-500',
      buttonSecondaryBg: 'bg-blue-900/50 hover:bg-blue-800/70',
      panelBg: 'bg-blue-950/30',
      accentColor: 'text-blue-400',
    },
    purple: {
      bgGradient: 'from-slate-900 via-purple-950 to-slate-900',
      borderColor: 'border-purple-900/50',
      textPrimary: 'text-purple-300',
      textSecondary: 'text-purple-300/70',
      textTertiary: 'text-purple-300/60',
      buttonBg: 'bg-purple-600 hover:bg-purple-500',
      buttonSecondaryBg: 'bg-purple-900/50 hover:bg-purple-800/70',
      panelBg: 'bg-purple-950/30',
      accentColor: 'text-purple-400',
    },
    red: {
      bgGradient: 'from-slate-900 via-red-950 to-slate-900',
      borderColor: 'border-red-900/50',
      textPrimary: 'text-red-300',
      textSecondary: 'text-red-300/70',
      textTertiary: 'text-red-300/60',
      buttonBg: 'bg-red-600 hover:bg-red-500',
      buttonSecondaryBg: 'bg-red-900/50 hover:bg-red-800/70',
      panelBg: 'bg-red-950/30',
      accentColor: 'text-red-400',
    },
    orange: {
      bgGradient: 'from-slate-900 via-orange-950 to-slate-900',
      borderColor: 'border-orange-900/50',
      textPrimary: 'text-orange-300',
      textSecondary: 'text-orange-300/70',
      textTertiary: 'text-orange-300/60',
      buttonBg: 'bg-orange-600 hover:bg-orange-500',
      buttonSecondaryBg: 'bg-orange-900/50 hover:bg-orange-800/70',
      panelBg: 'bg-orange-950/30',
      accentColor: 'text-orange-400',
    },
    cyan: {
      bgGradient: 'from-slate-900 via-cyan-950 to-slate-900',
      borderColor: 'border-cyan-900/50',
      textPrimary: 'text-cyan-300',
      textSecondary: 'text-cyan-300/70',
      textTertiary: 'text-cyan-300/60',
      buttonBg: 'bg-cyan-600 hover:bg-cyan-500',
      buttonSecondaryBg: 'bg-cyan-900/50 hover:bg-cyan-800/70',
      panelBg: 'bg-cyan-950/30',
      accentColor: 'text-cyan-400',
    },
    pink: {
      bgGradient: 'from-slate-900 via-pink-950 to-slate-900',
      borderColor: 'border-pink-900/50',
      textPrimary: 'text-pink-300',
      textSecondary: 'text-pink-300/70',
      textTertiary: 'text-pink-300/60',
      buttonBg: 'bg-pink-600 hover:bg-pink-500',
      buttonSecondaryBg: 'bg-pink-900/50 hover:bg-pink-800/70',
      panelBg: 'bg-pink-950/30',
      accentColor: 'text-pink-400',
    },
  };

  const theme = themeColors[colorTheme];

  const handleResetDelayAdjust = () => {
    sendParameter('delayAdjust', 0);
  };

  // Render Delay Adjust value safely
  const delayAdjustValue = parameters['delayAdjust'] ? Math.round(parameters['delayAdjust']) : 0;
  const inputEnabled = (parameters['inputEnabled'] ?? 1) > 0.5;

  return (
    <div className={`min-h-screen bg-gradient-to-br ${theme.bgGradient} flex flex-col`}>
      {/* Header */}
      <header className={`px-6 py-4 border-b ${theme.borderColor} backdrop-blur-sm bg-slate-900/50`}>
        <div className="flex items-center justify-between">
          <div className="flex-1">
            <h1 className="text-white font-semibold flex items-center gap-3">
              AmenBreakChopper
              <button
                onClick={() => sendParameter('inputEnabled', inputEnabled ? 0 : 1)}
                className={`ml-4 px-3 py-1 rounded-full text-[10px] font-bold tracking-wider border transition-all ${
                  inputEnabled
                    ? 'bg-green-500/20 text-green-400 border-green-500/50 shadow-[0_0_12px_rgba(34,197,94,0.3)]'
                    : 'bg-slate-800/50 text-slate-500 border-slate-700/50 hover:bg-slate-800'
                }`}
              >
                {inputEnabled ? 'ON' : 'MUTED'}
              </button>
            </h1>
          </div>

        </div>
      </header>

      {/* Main Content */}
      <main className="flex-1 flex flex-col p-6 gap-6 overflow-hidden">
        {/* Waveform Display */}
        <WaveformDisplay
          activeSlices={activeSlices}
          isPlaying={isPlaying}
          colorTheme={colorTheme}
          originalPlayhead={originalPlayhead}
          triggeredPlayhead={triggeredPlayhead}
        />

        {/* Control Panel */}
        {/* Performance Controls (Relocated) */}
        <div className={`flex flex-col items-center justify-between px-4 py-3 gap-3 rounded-xl border ${theme.borderColor} ${theme.panelBg}`}>
          <div className="flex items-center gap-4 w-full justify-between">
            <span className={`text-xs font-bold tracking-wider ${theme.textSecondary}`}>TIMING</span>

            <div className="flex items-center gap-2">
              <button
                onClick={() => {
                  performHardReset();
                  handleResetDelayAdjust();
                }}
                className="px-4 py-1.5 rounded-md bg-red-600 hover:bg-red-500 text-white font-bold text-sm shadow-sm transition-transform active:scale-95 flex items-center gap-2"
                title="Hard Reset & Zero Adjust"
              >
                <RotateCcw className="w-4 h-4" />
                <span>HARD RESET</span>
              </button>
            </div>
          </div>
          <div className="flex items-center gap-4 w-full justify-end">
            <div className="flex items-center gap-2 flex-1 max-w-xl bg-slate-900/40 p-1.5 rounded-lg border border-slate-700/30">
              <button
                onClick={() => sendParameter('delayAdjust', Math.max(-4096, delayAdjustValue - 1))}
                className={`px-2 py-1.5 rounded hover:bg-white/10 ${theme.textSecondary} transition-colors font-mono text-xs`}
              >
                &lt;
              </button>

              <div className="flex-1 px-2">
                <input
                  type="range"
                  min="-4096"
                  max="4096"
                  value={delayAdjustValue}
                  onChange={(e) => sendParameter('delayAdjust', Number(e.target.value))}
                  className={`w-full h-1.5 bg-slate-700 rounded-lg appearance-none cursor-pointer ${theme.accentColor === 'text-green-400' ? 'accent-green-500' : theme.accentColor === 'text-blue-400' ? 'accent-blue-500' : theme.accentColor === 'text-purple-400' ? 'accent-purple-500' : theme.accentColor === 'text-red-400' ? 'accent-red-500' : theme.accentColor === 'text-orange-400' ? 'accent-orange-500' : theme.accentColor === 'text-cyan-400' ? 'accent-cyan-500' : 'accent-pink-500'}`}
                />
              </div>

              <button
                onClick={() => sendParameter('delayAdjust', Math.min(4096, delayAdjustValue + 1))}
                className={`px-2 py-1.5 rounded hover:bg-white/10 ${theme.textSecondary} transition-colors font-mono text-xs`}
              >
                &gt;
              </button>

              <div className="w-px h-4 bg-slate-600/50 mx-1"></div>

              <input
                type="number"
                value={delayAdjustValue}
                onChange={(e) => sendParameter('delayAdjust', Math.min(4096, Math.max(-4096, Number(e.target.value))))}
                className="w-16 bg-transparent text-right font-mono text-xs focus:outline-none text-slate-200"
              />
              <span className={`text-[10px] ${theme.textSecondary} ml-0.5 mr-2`}>smpl</span>
            </div>
          </div>


          {/* Separate Slice Reset Button (Optional: Keep here or leave in header? User said "Hard Reset" integrate. I'll keep Slice Reset in Header as it matches "Overview" context, or move it here for total unification. I'll leave Slice Reset in header for now as the request focused on Hard Reset/Adjust.) */}
        </div>

        <ControlPanel colorTheme={colorTheme} onThemeChange={handleThemeChange} />
      </main>

      {/* Footer Info */}
      <footer className={`px-6 py-3 border-t ${theme.borderColor} bg-slate-900/50`}>
        <div className={`flex items-center justify-between text-xs ${theme.textTertiary}`}>
          <span>{activeSlices.size} / 16 slices active</span>
          <div className="flex gap-4">
            <span>Original: {originalPlayhead + 1}/16</span>
            <span>Triggered: {triggeredPlayhead !== null ? triggeredPlayhead + 1 : '-'}/16</span>
          </div>
        </div>
      </footer>
    </div>
  );
}