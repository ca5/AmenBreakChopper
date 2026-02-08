import { useState, useEffect } from 'react';
import { WaveformDisplay } from './components/WaveformDisplay';
import { ControlPanel } from './components/ControlPanel';
import { MidiController } from './components/MidiController';
import { MidiControllerAdvanced } from './components/MidiControllerAdvanced';
import { ToggleButtons } from './components/ToggleButtons';
import { RotateCcw, Settings, ArrowLeft, ChevronDown } from 'lucide-react';
import { useJuceBridge } from '../hooks/useJuceBridge';
import Ca5LogoPng from '../ca5logo.png';

export default function App() {
  // Always playing in plugin mode
  const isPlaying = true;
  const [view, setView] = useState<'main' | 'config'>('main');

  const [activeSlices, setActiveSlices] = useState<Set<number>>(
    new Set(Array.from({ length: 16 }, (_, i) => i)) // All slices active by default
  );
  const [colorTheme, setColorTheme] = useState<'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink'>('green');
  const [currentScreen, setCurrentScreen] = useState<'timing' | 'midi' | 'toggle'>('timing');
  const [viewMode, setViewMode] = useState<'waveform' | 'midiController'>('waveform');

  // Hoisted state for status display
  const [originalPlayhead, setOriginalPlayhead] = useState(0);
  const [triggeredPlayhead, setTriggeredPlayhead] = useState<number | null>(null);

  // Use the bridge
  const { parameters, sendParameter, addEventListener, performHardReset, isStandalone, loadSample } = useJuceBridge();

  // Scroll locking logic for Standalone
  useEffect(() => {
    const root = document.getElementById('root');
    if (root) {
      if (!isStandalone) {
         // Plugin mode: Always allow scrolling
         root.style.overflowY = 'auto';
      } else {
         // Standalone mode: Only allow scrolling in Config view
         if (view === 'main') {
           // Reset scroll position before locking
           window.scrollTo(0, 0);
           root.scrollTop = 0;
           root.style.overflowY = 'hidden';
         } else {
           root.style.overflowY = 'auto';
         }
      }
    }
  }, [view, isStandalone]);

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
  
  // inputEnabled: true = EXT INPUT, false = internal sample
  const inputEnabled = parameters['inputEnabled'] !== undefined 
    ? parameters['inputEnabled'] > 0.5 
    : false; // Default to false (internal sample) until parameter is received

  // Read audioSource parameter from JUCE
  // audioSource: 0=EXT INPUT, 1=Amen140, 2=Amen160, 3=Amen180, 4=Amen200
  // Note: We're sending the index directly, not normalized values
  const audioSourceParam = parameters['audioSource'] !== undefined 
    ? Math.round(parameters['audioSource']) // Use value directly as index
    : 0;

  // Debug logging
  if (parameters['audioSource'] !== undefined) {
    console.log('[UI] audioSource raw value:', parameters['audioSource'], '→ index:', audioSourceParam);
  }

  // Map audioSource index to dropdown value
  const audioSourceValue = (() => {
    switch (audioSourceParam) {
      case 0: return 'ext';
      case 1: return 'amen140.wav';
      case 2: return 'amen160.wav';
      case 3: return 'amen180.wav';
      case 4: return 'amen200.wav';
      default: return 'ext';
    }
  })();

  // Track selected sample name for UI display (deprecated, kept for compatibility)
  const [currentSample, setCurrentSample] = useState<string>('amen140.wav');

  // MIDI Controller enabled state
  const midiControllerEnabled = parameters['midiControllerEnabled'] !== undefined
    ? parameters['midiControllerEnabled'] > 0.5
    : false;



  return (
    <div className={`h-screen bg-gradient-to-br ${theme.bgGradient} flex flex-col pt-[env(safe-area-inset-top)] pb-[env(safe-area-inset-bottom)]`}>
      {/* Header */}
      <header className={`px-4 py-2 border-b ${theme.borderColor} backdrop-blur-sm bg-slate-900/50`}>
        <div className="flex items-center justify-between">
          <div className="flex-1">
            <h1 className="text-white text-sm font-medium flex items-center gap-3">
            <div className="flex items-center gap-2">
                <div className="relative">
                     <select
                        className={`w-40 px-3 py-1 border rounded-full text-xs font-bold tracking-wider ${
                           inputEnabled 
                           ? 'bg-green-500/20 text-green-400 border-green-500/50' 
                           : 'bg-slate-800/80 text-orange-400 border-orange-500/50'
                        } appearance-none focus:outline-none transition-all cursor-pointer`}
                        value={audioSourceValue} 
                        onChange={(e) => {
                            const val = e.target.value;
                            if (val === 'ext') {
                                // EXT INPUT selected
                                sendParameter('audioSource', 0); // 0 = EXT INPUT
                                sendParameter('inputEnabled', 1);
                                // When switching to EXT INPUT, set BPM sync to HOST mode
                                // This allows DAW BPM to be used in plugin mode
                                sendParameter('bpmSyncMode', 0);
                            } else {
                                // Internal sample selected
                                // Map sample name to audioSource index
                                let audioSourceIndex = 0;
                                if (val === 'amen140.wav') audioSourceIndex = 1;
                                else if (val === 'amen160.wav') audioSourceIndex = 2;
                                else if (val === 'amen180.wav') audioSourceIndex = 3;
                                else if (val === 'amen200.wav') audioSourceIndex = 4;
                                
                                sendParameter('audioSource', audioSourceIndex);
                                
                                // Update local state for UI consistency
                                setCurrentSample(val);

                                // Force Input Disabled immediately (UI feedback)
                                sendParameter('inputEnabled', 0);

                                // Load Sample (this will set bpmSyncMode to MANUAL automatically)
                                if (loadSample) {
                                  loadSample(val);
                                }
                            }
                        }}
                     >
                        <option value="ext">EXT INPUT</option>
                        <option value="amen140.wav">Amen Break 140</option>
                        <option value="amen160.wav">Amen Break 160</option>
                        <option value="amen180.wav">Amen Break 180</option>
                        <option value="amen200.wav">Amen Break 200</option>
                     </select>
                     <div className="absolute inset-y-0 right-0 flex items-center px-2 pointer-events-none">
                        <ChevronDown size={14} className={inputEnabled ? 'text-green-400' : 'text-orange-400'} />
                     </div>
                </div>
            </div>
            </h1>
          </div>

           {/* Settings / Back Button */}
           <button
             onClick={() => setView(view === 'main' ? 'config' : 'main')}
             className={`p-2 rounded-full hover:bg-white/10 ${theme.textPrimary} transition-colors`}
           >
             {view === 'main' ? <Settings className="w-5 h-5" /> : <ArrowLeft className="w-5 h-5" />}
           </button>
        </div>
      </header>

      {/* Main Content */}
      <main className="flex-1 flex flex-col p-6 gap-2 overflow-y-auto">
        {view === 'main' ? (
          <>
            {/* Waveform Display or MIDI Controller Advanced */}
            <div className="relative">
              {/* View Mode Toggle Button - Only show when MIDI Controller is enabled */}
              {midiControllerEnabled && (
                <div className="absolute top-2 left-2 z-10">
                  <button
                    onClick={() => setViewMode(viewMode === 'waveform' ? 'midiController' : 'waveform')}
                    className={`px-3 py-1 rounded-md text-xs font-bold transition-all ${theme.buttonSecondaryBg} ${theme.textPrimary} border ${theme.borderColor} shadow-md`}
                  >
                    {viewMode === 'waveform' ? 'Wave' : 'MIDI Controller Advanced'}
                  </button>
                </div>
              )}

              {viewMode === 'waveform' ? (
                <WaveformDisplay
                  activeSlices={activeSlices}
                  isPlaying={isPlaying}
                  colorTheme={colorTheme}
                  originalPlayhead={originalPlayhead}
                  triggeredPlayhead={triggeredPlayhead}
                />
              ) : (
                <MidiControllerAdvanced colorTheme={colorTheme} />
              )}
            </div>

            {/* Performance Controls / MIDI Controller */}
            <div className={`flex flex-col items-center justify-between px-3 py-2 gap-2 rounded-xl border ${theme.borderColor} ${theme.panelBg}`}>
              {/* Titles are now clickable - No separate toggle button needed */}

              {/* Content Area */}
              {currentScreen === 'midi' && midiControllerEnabled ? (
                <MidiController 
                  colorTheme={colorTheme} 
                  onSwitchToTiming={() => setCurrentScreen('timing')}
                />
              ) : (
                <>
              <div className="flex items-center gap-2 w-full justify-between">
                {midiControllerEnabled && (
                  <button
                    onClick={() => setCurrentScreen('midi')}
                    className={`px-3 py-1 rounded-md text-xs font-bold transition-all ${theme.buttonSecondaryBg} ${theme.textPrimary} border ${theme.borderColor}`}
                  >
                    TIMING
                  </button>
                )}
                {!midiControllerEnabled && (
                  <span className={`text-xs font-bold tracking-wider ${theme.textSecondary}`}>TIMING</span>
                )}

                <div className="flex items-center gap-2">
                  <button
                    onClick={() => {
                      performHardReset();
                      handleResetDelayAdjust();
                    }}
                    className="px-3 py-1 rounded-md bg-red-600 hover:bg-red-500 text-white font-bold text-xs shadow-sm transition-transform active:scale-95 flex items-center gap-1.5"
                    title="Hard Reset & Zero Adjust"
                  >
                    <RotateCcw className="w-3 h-3" />
                    <span>HARD RESET</span>
                  </button>
                </div>
              </div>
              <div className="flex items-center gap-2 w-full justify-end">
                <div className="flex items-center gap-1.5 flex-1 max-w-xl bg-slate-900/40 p-1 rounded-lg border border-slate-700/30">
                  <button
                    onClick={() => sendParameter('delayAdjust', Math.max(-1000, delayAdjustValue - 10))}
                    className={`px-1.5 py-1 rounded hover:bg-white/10 ${theme.textSecondary} transition-colors font-mono text-xs`}
                  >
                    &lt;
                  </button>

                  <div className="flex-1 px-1.5">
                    <input
                      type="range"
                      min="-1000"
                      max="1000"
                      value={delayAdjustValue}
                      onChange={(e) => sendParameter('delayAdjust', Number(e.target.value))}
                      className={`w-full h-1 bg-slate-700 rounded-lg appearance-none cursor-pointer ${theme.accentColor === 'text-green-400' ? 'accent-green-500' : theme.accentColor === 'text-blue-400' ? 'accent-blue-500' : theme.accentColor === 'text-purple-400' ? 'accent-purple-500' : theme.accentColor === 'text-red-400' ? 'accent-red-500' : theme.accentColor === 'text-orange-400' ? 'accent-orange-500' : theme.accentColor === 'text-cyan-400' ? 'accent-cyan-500' : 'accent-pink-500'}`}
                    />
                  </div>

                  <button
                    onClick={() => sendParameter('delayAdjust', Math.min(1000, delayAdjustValue + 10))}
                    className={`px-1.5 py-1 rounded hover:bg-white/10 ${theme.textSecondary} transition-colors font-mono text-xs`}
                  >
                    &gt;
                  </button>

                  <div className="w-px h-3 bg-slate-600/50 mx-0.5"></div>

                  <input
                    type="number"
                    value={delayAdjustValue}
                    onChange={(e) => sendParameter('delayAdjust', Math.min(1000, Math.max(-1000, Number(e.target.value))))}
                    className="w-14 bg-transparent text-right font-mono text-xs focus:outline-none text-slate-200"
                  />
                  <span className={`text-[10px] ${theme.textSecondary} ml-0.5 mr-1`}>ms</span>
                </div>
              </div>
                </>
              )}
            </div>

            {/* Channel Setting - Only visible when MIDI Controller is enabled */}
            {midiControllerEnabled && <ToggleButtons colorTheme={colorTheme} />}
          </>
        ) : (
          <div className="space-y-6">
            <ControlPanel colorTheme={colorTheme} onThemeChange={handleThemeChange} />
          </div>
        )}
      </main>

      {/* Footer Info */}
      <footer className={`px-6 py-3 border-t ${theme.borderColor} bg-slate-900/50`}>
        <div className={`flex items-center justify-between text-xs ${theme.textTertiary}`}>
          <div className="flex items-center gap-1.5">
             <span className="opacity-80">produced by</span>
             <a href="https://ca5.github.io/AmenBreakChopper_doc/" target="_blank" rel="noreferrer" className="hover:opacity-80 transition-opacity" title="Documentation">
               <img src={Ca5LogoPng} alt="ca5" className="h-5 w-5" />
             </a>
          </div>
          <a href="https://ca5.github.io/AmenBreakChopper_doc/" target="_blank" rel="noreferrer" className="hover:text-white transition-colors">
            Docs
          </a>
        </div>
      </footer>
    </div>
  );
}