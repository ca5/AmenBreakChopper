import { useState, useEffect } from 'react';
import { ChevronLeft, ChevronRight, ChevronDown, ChevronUp, Settings } from 'lucide-react';
import { useJuceBridge } from '../../hooks/useJuceBridge';

interface ControlPanelProps {
  colorTheme: 'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink';
  onThemeChange: (theme: 'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink') => void;
}

export function ControlPanel({ colorTheme, onThemeChange }: ControlPanelProps) {
  const { parameters, sendParameter } = useJuceBridge();
  const [currentPage, setCurrentPage] = useState(0);

  // Helpers for parameter mapping
  const getParam = (id: string, def: number) => parameters[id] ?? def;
  const setParam = (id: string, val: number) => {
    // console.log(`[UI] Parameter Change - ID: ${id}, Value: ${val}`);
    sendParameter(id, val);
  };

  // Mapped States (derived from parameters or defaults)
  const controlMode = getParam('controlMode', 0) > 0.5 ? 'OSC' : 'MIDI';
  const getIntParam = (id: string, def: number) => Math.round(getParam(id, def));

  const midiInCh = getIntParam('midiInputChannel', 0);
  const midiOutCh = getIntParam('midiOutputChannel', 1);
  const seqResetCC = getIntParam('midiCcSeqReset', 93);
  const hrdResetCC = getIntParam('midiCcHardReset', 106);
  const sftResetCC = getIntParam('midiCcSoftReset', 97);

  // New Delay CC params
  const dlyAdjFwdCC = getIntParam('midiCcDelayAdjustFwd', 21);
  const dlyAdjBwdCC = getIntParam('midiCcDelayAdjustBwd', 19);
  const dlyAdjStep = getIntParam('delayAdjustCcStep', 64);

  // Modes: 0=Gate-On, 1=Gate-Off, 2=Toggle (Assumed)
  const seqResetModeVal = getIntParam('midiCcSeqResetMode', 0);
  const hrdResetModeVal = getIntParam('midiCcHardResetMode', 0);
  const sftResetModeVal = getIntParam('midiCcSoftResetMode', 0);

  const oscSendPrt = getIntParam('oscSendPort', 9001);
  const oscRecvPrt = getIntParam('oscReceivePort', 9002);

  // Standalone Params
  const bpmMode = getParam('bpmSyncMode', 0); // 0=Host, 1=MIDI
  const inputChanL = getIntParam('inputChanL', 1);
  const inputChanR = getIntParam('inputChanR', 2);

  const [hostIP, setHostIP] = useState('127.0.0.1'); // Local only for now

  // Theme colors
  const themeColors = {
    green: { panel: 'bg-green-950/30', border: 'border-green-900/50', text: 'text-green-300', textSecondary: 'text-green-300/80', textTertiary: 'text-green-400/60', buttonBg: 'bg-green-900/30 hover:bg-green-800/50', inputBg: 'bg-slate-800/50 border-green-900/50 focus:border-green-600', accentBg: 'bg-green-600', accentSlider: 'accent-green-600' },
    blue: { panel: 'bg-blue-950/30', border: 'border-blue-900/50', text: 'text-blue-300', textSecondary: 'text-blue-300/80', textTertiary: 'text-blue-400/60', buttonBg: 'bg-blue-900/30 hover:bg-blue-800/50', inputBg: 'bg-slate-800/50 border-blue-900/50 focus:border-blue-600', accentBg: 'bg-blue-600', accentSlider: 'accent-blue-600' },
    purple: { panel: 'bg-purple-950/30', border: 'border-purple-900/50', text: 'text-purple-300', textSecondary: 'text-purple-300/80', textTertiary: 'text-purple-400/60', buttonBg: 'bg-purple-900/30 hover:bg-purple-800/50', inputBg: 'bg-slate-800/50 border-purple-900/50 focus:border-purple-600', accentBg: 'bg-purple-600', accentSlider: 'accent-purple-600' },
    red: { panel: 'bg-red-950/30', border: 'border-red-900/50', text: 'text-red-300', textSecondary: 'text-red-300/80', textTertiary: 'text-red-400/60', buttonBg: 'bg-red-900/30 hover:bg-red-800/50', inputBg: 'bg-slate-800/50 border-red-900/50 focus:border-red-600', accentBg: 'bg-red-600', accentSlider: 'accent-red-600' },
    orange: { panel: 'bg-orange-950/30', border: 'border-orange-900/50', text: 'text-orange-300', textSecondary: 'text-orange-300/80', textTertiary: 'text-orange-400/60', buttonBg: 'bg-orange-900/30 hover:bg-orange-800/50', inputBg: 'bg-slate-800/50 border-orange-900/50 focus:border-orange-600', accentBg: 'bg-orange-600', accentSlider: 'accent-orange-600' },
    cyan: { panel: 'bg-cyan-950/30', border: 'border-cyan-900/50', text: 'text-cyan-300', textSecondary: 'text-cyan-300/80', textTertiary: 'text-cyan-400/60', buttonBg: 'bg-cyan-900/30 hover:bg-cyan-800/50', inputBg: 'bg-slate-800/50 border-cyan-900/50 focus:border-cyan-600', accentBg: 'bg-cyan-600', accentSlider: 'accent-cyan-600' },
    pink: { panel: 'bg-pink-950/30', border: 'border-pink-900/50', text: 'text-pink-300', textSecondary: 'text-pink-300/80', textTertiary: 'text-pink-400/60', buttonBg: 'bg-pink-900/30 hover:bg-pink-800/50', inputBg: 'bg-slate-800/50 border-pink-900/50 focus:border-pink-600', accentBg: 'bg-pink-600', accentSlider: 'accent-pink-600' },
  };

  const theme = themeColors[colorTheme];

  const modeOptions = ['Gate-On', 'Gate-Off', 'Toggle'];

  const pages = controlMode === 'MIDI' ? [
    {
      title: 'MIDI Configuration',
      content: (
        <div className="space-y-4">
          {/* MIDI In Channel */}
          <div className="flex items-center justify-between">
            <label className={`text-sm ${theme.textSecondary}`}>MIDI In Channel</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={midiInCh}
                onChange={(e) => setParam('midiInputChannel', Number(e.target.value))}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button onClick={() => setParam('midiInputChannel', Math.max(0, midiInCh - 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>-</button>
              <button onClick={() => setParam('midiInputChannel', Math.min(15, midiInCh + 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>+</button>
            </div>
          </div>

          {/* MIDI Out Channel */}
          <div className="flex items-center justify-between">
            <label className={`text-sm ${theme.textSecondary}`}>MIDI Out Channel</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={midiOutCh}
                onChange={(e) => setParam('midiOutputChannel', Number(e.target.value))}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button onClick={() => setParam('midiOutputChannel', Math.max(0, midiOutCh - 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>-</button>
              <button onClick={() => setParam('midiOutputChannel', Math.min(15, midiOutCh + 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>+</button>
            </div>
          </div>

          {/* Sequence Reset CC */}
          <div className="flex flex-col gap-1.5">
            <label className={`text-sm ${theme.textSecondary}`}>Sequence Reset CC</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={seqResetCC}
                onChange={(e) => setParam('midiCcSeqReset', Number(e.target.value))}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button onClick={() => setParam('midiCcSeqReset', Math.max(0, seqResetCC - 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>-</button>
              <button onClick={() => setParam('midiCcSeqReset', Math.min(127, seqResetCC + 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>+</button>
              <select
                value={modeOptions[seqResetModeVal]}
                onChange={(e) => setParam('midiCcSeqResetMode', modeOptions.indexOf(e.target.value))}
                className={`flex-1 px-2 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} focus:outline-none min-w-[80px] text-xs`}
              >
                {modeOptions.map(opt => <option key={opt}>{opt}</option>)}
              </select>
            </div>
          </div>

          {/* Hard Reset CC */}
          <div className="flex flex-col gap-1.5">
            <label className={`text-sm ${theme.textSecondary}`}>Hard Reset CC</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={hrdResetCC}
                onChange={(e) => setParam('midiCcHardReset', Number(e.target.value))}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button onClick={() => setParam('midiCcHardReset', Math.max(0, hrdResetCC - 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>-</button>
              <button onClick={() => setParam('midiCcHardReset', Math.min(127, hrdResetCC + 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>+</button>
              <select
                value={modeOptions[hrdResetModeVal]}
                onChange={(e) => setParam('midiCcHardResetMode', modeOptions.indexOf(e.target.value))}
                className={`flex-1 px-2 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} focus:outline-none min-w-[80px] text-xs`}
              >
                {modeOptions.map(opt => <option key={opt}>{opt}</option>)}
              </select>
            </div>
          </div>

          {/* Soft Reset CC */}
          <div className="flex flex-col gap-1.5">
            <label className={`text-sm ${theme.textSecondary}`}>Soft Reset CC</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={sftResetCC}
                onChange={(e) => setParam('midiCcSoftReset', Number(e.target.value))}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button onClick={() => setParam('midiCcSoftReset', Math.max(0, sftResetCC - 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>-</button>
              <button onClick={() => setParam('midiCcSoftReset', Math.min(127, sftResetCC + 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>+</button>
              <select
                value={modeOptions[sftResetModeVal]}
                onChange={(e) => setParam('midiCcSoftResetMode', modeOptions.indexOf(e.target.value))}
                className={`flex-1 px-2 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} focus:outline-none min-w-[80px] text-xs`}
              >
                {modeOptions.map(opt => <option key={opt}>{opt}</option>)}
              </select>
            </div>
          </div>

          <div className={`h-px ${theme.border} my-4`} />
          <h4 className={`text-xs uppercase font-bold ${theme.textTertiary} mb-2`}>Delay Adjust CC</h4>

          {/* Delay Adjust CC Fwd */}
          <div className="flex items-center justify-between">
            <label className={`text-sm ${theme.textSecondary}`}>CC Forward</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={dlyAdjFwdCC}
                onChange={(e) => setParam('midiCcDelayAdjustFwd', Number(e.target.value))}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button onClick={() => setParam('midiCcDelayAdjustFwd', Math.max(0, dlyAdjFwdCC - 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>-</button>
              <button onClick={() => setParam('midiCcDelayAdjustFwd', Math.min(127, dlyAdjFwdCC + 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>+</button>
            </div>
          </div>

          {/* Delay Adjust CC Bwd */}
          <div className="flex items-center justify-between">
            <label className={`text-sm ${theme.textSecondary}`}>CC Backward</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={dlyAdjBwdCC}
                onChange={(e) => setParam('midiCcDelayAdjustBwd', Number(e.target.value))}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button onClick={() => setParam('midiCcDelayAdjustBwd', Math.max(0, dlyAdjBwdCC - 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>-</button>
              <button onClick={() => setParam('midiCcDelayAdjustBwd', Math.min(127, dlyAdjBwdCC + 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>+</button>
            </div>
          </div>

          {/* Delay Adjust Step */}
          <div className="flex items-center justify-between">
            <label className={`text-sm ${theme.textSecondary}`}>CC Step</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={dlyAdjStep}
                onChange={(e) => setParam('delayAdjustCcStep', Number(e.target.value))}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button onClick={() => setParam('delayAdjustCcStep', Math.max(1, dlyAdjStep - 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>-</button>
              <button onClick={() => setParam('delayAdjustCcStep', Math.min(128, dlyAdjStep + 1))} className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}>+</button>
            </div>
          </div>

        </div>
      ),
    },
  ] : [
    {
      title: 'OSC Configuration',
      content: (
        <div className="space-y-4">
          {/* Host IP Address */}
          <div className="flex items-center justify-between">
            <label className={`text-sm ${theme.textSecondary}`}>Host IP Address</label>
            <input
              type="text"
              value={hostIP}
              onChange={(e) => setHostIP(e.target.value)}
              className={`w-48 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} focus:outline-none`}
            />
          </div>

          {/* Send Port */}
          <div className="flex items-center justify-between">
            <label className={`text-sm ${theme.textSecondary}`}>Send Port</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={oscSendPrt}
                onChange={(e) => setParam('oscSendPort', Number(e.target.value))}
                className={`w-24 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button
                onClick={() => setParam('oscSendPort', Math.max(1, oscSendPrt - 1))}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                -
              </button>
              <button
                onClick={() => setParam('oscSendPort', Math.min(65535, oscSendPrt + 1))}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                +
              </button>
            </div>
          </div>

          {/* Receive Port */}
          <div className="flex items-center justify-between">
            <label className={`text-sm ${theme.textSecondary}`}>Receive Port</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={oscRecvPrt}
                onChange={(e) => setParam('oscReceivePort', Number(e.target.value))}
                className={`w-24 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button
                onClick={() => setParam('oscReceivePort', Math.max(1, oscRecvPrt - 1))}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                -
              </button>
              <button
                onClick={() => setParam('oscReceivePort', Math.min(65535, oscRecvPrt + 1))}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                +
              </button>
            </div>
          </div>
        </div>
      ),
    },
  ];

  return (
    <div className={`flex flex-col gap-6 ${theme.panel} rounded-2xl p-6 border ${theme.border} backdrop-blur-sm shadow-xl`}>
      
      {/* Configuration Header */}
      <div className="flex items-center justify-between mb-2">
        <h2 className={`text-lg font-bold ${theme.text} flex items-center gap-2`}>
            Configuration
        </h2>
      </div>

      <div className={`p-4 ${theme.inputBg} rounded-xl border ${theme.border}`}>

            {/* Theme Selection */}
            <div className="flex items-center justify-between mb-6">
            <span className={`text-sm ${theme.textSecondary}`}>Color Theme</span>
            <select
                value={colorTheme}
                onChange={(e) => onThemeChange(e.target.value as any)}
                className={`px-3 py-1.5 rounded ${theme.inputBg} border ${theme.border} ${theme.text} text-xs focus:outline-none`}
            >
                <option value="green">Green</option>
                <option value="blue">Blue</option>
                <option value="purple">Purple</option>
                <option value="red">Red</option>
                <option value="orange">Orange</option>
                <option value="cyan">Cyan</option>
                <option value="pink">Pink</option>
            </select>
            </div>

            <div className={`h-px ${theme.border} mb-6`} />

            {/* Audio & Sync (Standalone) */}
            <div className="mb-6">
                <h4 className={`text-xs uppercase font-bold ${theme.textTertiary} mb-3`}>Audio & Sync</h4>
                
                {/* BPM Sync Mode */}
                <div className="flex items-center justify-between mb-3">
                    <span className={`text-sm ${theme.textSecondary}`}>BPM Sync Source</span>
                    <div className="flex gap-2">
                    <button
                        onClick={() => setParam('bpmSyncMode', 0)}
                        className={`px-3 py-1.5 rounded-lg text-xs font-medium transition-all ${bpmMode < 0.5
                        ? `${theme.accentBg} text-white`
                        : `bg-slate-700/50 ${theme.text}`
                        }`}
                    >
                        HOST
                    </button>
                    <button
                        onClick={() => setParam('bpmSyncMode', 1)}
                        className={`px-3 py-1.5 rounded-lg text-xs font-medium transition-all ${bpmMode >= 0.5
                        ? `${theme.accentBg} text-white`
                        : `bg-slate-700/50 ${theme.text}`
                        }`}
                    >
                        MIDI CLOCK
                    </button>
                    </div>
                </div>

                {/* Input Channels (Restored) */}
                <div className="flex items-center justify-between">
                    <span className={`text-sm ${theme.textSecondary}`}>Input Channels (L / R)</span>
                    <div className="flex gap-2 items-center">
                        {/* L */}
                        <div className="flex items-center gap-1">
                            <span className="text-[10px] text-slate-500">L:</span>
                            <div className="flex items-center">
                                <button onClick={() => setParam('inputChanL', Math.max(1, inputChanL - 1))} className={`w-6 py-1 ${theme.buttonBg} ${theme.text} rounded-l text-xs`}>-</button>
                                <span className={`w-8 py-1 ${theme.inputBg} ${theme.textSecondary} text-center text-xs border-y border-slate-700`}>{inputChanL}</span>
                                <button onClick={() => setParam('inputChanL', Math.min(8, inputChanL + 1))} className={`w-6 py-1 ${theme.buttonBg} ${theme.text} rounded-r text-xs`}>+</button>
                            </div>
                        </div>
                            {/* R */}
                            <div className="flex items-center gap-1">
                            <span className="text-[10px] text-slate-500">R:</span>
                            <div className="flex items-center">
                                <button onClick={() => setParam('inputChanR', Math.max(1, inputChanR - 1))} className={`w-6 py-1 ${theme.buttonBg} ${theme.text} rounded-l text-xs`}>-</button>
                                <span className={`w-8 py-1 ${theme.inputBg} ${theme.textSecondary} text-center text-xs border-y border-slate-700`}>{inputChanR}</span>
                                <button onClick={() => setParam('inputChanR', Math.min(8, inputChanR + 1))} className={`w-6 py-1 ${theme.buttonBg} ${theme.text} rounded-r text-xs`}>+</button>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <div className={`h-px ${theme.border} mb-6`} />

            {/* Control Mode (Moved Here) */}
            <div className="flex items-center justify-between mb-6">
                <span className={`text-sm ${theme.textSecondary}`}>Control Mode:</span>
                <div className="flex gap-2">
                <button
                    onClick={() => {
                    setParam('controlMode', 0); // MIDI
                    setCurrentPage(0);
                    }}
                    className={`px-4 py-2 rounded-lg text-sm font-medium transition-all ${controlMode === 'MIDI'
                    ? `${theme.accentBg} text-white`
                    : `bg-slate-700/50 ${theme.text}`
                    }`}
                >
                    MIDI
                </button>
                <button
                    onClick={() => {
                    setParam('controlMode', 1); // OSC
                    setCurrentPage(0);
                    }}
                    className={`px-4 py-2 rounded-lg text-sm font-medium transition-all ${controlMode === 'OSC'
                    ? `${theme.accentBg} text-white`
                    : `bg-slate-700/50 ${theme.text}`
                    }`}
                >
                    OSC
                </button>
                </div>
            </div>

            <div className={`h-px ${theme.border} mb-6`} />

            {/* Page Header within Container */}
            <div className="flex items-center justify-between mb-4">
                <h2 className={`${theme.text} font-medium text-sm`}>{pages[currentPage].title}</h2>
            </div>

            {/* Content */}
            {pages[currentPage].content}

      </div>
    </div>
  );
}
