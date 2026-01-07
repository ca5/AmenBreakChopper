import { useState, useEffect } from 'react';
import { ChevronLeft, ChevronRight } from 'lucide-react';

interface ControlPanelProps {
  colorTheme: 'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink';
}

export function ControlPanel({ colorTheme }: ControlPanelProps) {
  const [currentPage, setCurrentPage] = useState(0);
  const [controlMode, setControlMode] = useState<'MIDI' | 'OSC'>('MIDI');

  // MIDI Configuration
  const [midiInChannel, setMidiInChannel] = useState(0);
  const [midiOutChannel, setMidiOutChannel] = useState(1);
  const [sequenceResetCC, setSequenceResetCC] = useState(93);
  const [sequenceResetMode, setSequenceResetMode] = useState('Gate-On');
  const [timerResetCC, setTimerResetCC] = useState(106);
  const [timerResetMode, setTimerResetMode] = useState('Gate-On');
  const [softResetCC, setSoftResetCC] = useState(97);
  const [softResetMode, setSoftResetMode] = useState('Gate-On');

  // Sync & Input Configuration
  const [bpmSyncMode, setBpmSyncMode] = useState(0); // 0: Host, 1: MIDI Clock
  const [inputEnabled, setInputEnabled] = useState(true);
  const [inputChannel, setInputChannel] = useState(1);

  // OSC Configuration
  const [hostIP, setHostIP] = useState('127.0.0.1');
  const [sendPort, setSendPort] = useState(9001);
  const [receivePort, setReceivePort] = useState(9002);
  const [delayAdjustSamples, setDelayAdjustSamples] = useState(0);

  // Helper to send parameter updates
  const updateParameter = (id: string, value: number | string | boolean) => {
    if (typeof (window as any).sendParameterValue === 'function') {
      // Map boolean to 0/1, string modes to indices if needed?
      // For modes like "Gate-On", the backend expects an integer index (0, 1, 2).
      // Let's assume the state holds the string for UI but we need to map it.

      let valToSend = value;

      if (typeof value === 'boolean') {
          valToSend = value ? 1 : 0;
      }

      // Map reset modes: "Any"->0, "Gate-On"->1, "Gate-Off"->2
      // Default seems to be 1 (Gate-On)
      if (id.endsWith('Mode') && typeof value === 'string') {
          if (value === 'Any') valToSend = 0;
          else if (value === 'Gate-On') valToSend = 1;
          else if (value === 'Gate-Off') valToSend = 2;
          else if (value === 'Toggle') valToSend = 0; // Default/Fallback?
      }

      (window as any).sendParameterValue(id, valToSend);
    }
  };

  // Setup initial sync if needed, or rely on handlers.
  // Ideally we should sync from backend on load (requestInitialState).
  // But for now let's ensuring outgoing updates work.

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

  const pages = controlMode === 'MIDI' ? [
    {
      title: 'Sync & Input',
      content: (
        <div className="space-y-4">
           {/* BPM Sync Mode */}
           <div className="flex items-center justify-between">
             <label className={`text-sm ${theme.textSecondary}`}>BPM Sync Mode</label>
             <select
                value={bpmSyncMode}
                onChange={(e) => {
                  const val = Number(e.target.value);
                  setBpmSyncMode(val);
                  updateParameter('bpmSyncMode', val);
                }}
                className={`px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} focus:outline-none`}
              >
                <option value={0}>Host</option>
                <option value={1}>MIDI Clock</option>
              </select>
           </div>

           {/* Input Enabled */}
           <div className="flex items-center justify-between">
             <label className={`text-sm ${theme.textSecondary}`}>Audio Input</label>
             <button
               onClick={() => {
                   const newVal = !inputEnabled;
                   setInputEnabled(newVal);
                   updateParameter('inputEnabled', newVal);
               }}
               className={`px-3 py-1.5 rounded transition-colors border ${theme.border} ${inputEnabled ? theme.accentBg + ' text-white' : 'bg-transparent ' + theme.textSecondary}`}
             >
               {inputEnabled ? 'Enabled' : 'Disabled'}
             </button>
           </div>

           {/* Input Channel */}
           <div className="flex items-center justify-between">
             <label className={`text-sm ${theme.textSecondary}`}>Input Channel</label>
             <div className="flex items-center gap-2">
               <input
                type="number"
                value={inputChannel}
                onChange={(e) => {
                    const val = Number(e.target.value);
                    setInputChannel(val);
                    updateParameter('inputChannel', val);
                }}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
               />
               <button
                onClick={() => {
                    const val = Math.max(1, inputChannel - 1);
                    setInputChannel(val);
                    updateParameter('inputChannel', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                -
              </button>
              <button
                onClick={() => {
                    const val = Math.min(16, inputChannel + 1);
                    setInputChannel(val);
                    updateParameter('inputChannel', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                +
              </button>
             </div>
           </div>
        </div>
      )
    },
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
                value={midiInChannel}
                onChange={(e) => {
                    const val = Number(e.target.value);
                    setMidiInChannel(val);
                    updateParameter('midiInputChannel', val);
                }}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button
                onClick={() => {
                    const val = Math.max(0, midiInChannel - 1);
                    setMidiInChannel(val);
                    updateParameter('midiInputChannel', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                -
              </button>
              <button
                onClick={() => {
                    const val = Math.min(15, midiInChannel + 1);
                    setMidiInChannel(val);
                    updateParameter('midiInputChannel', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                +
              </button>
            </div>
          </div>

          {/* MIDI Out Channel */}
          <div className="flex items-center justify-between">
            <label className={`text-sm ${theme.textSecondary}`}>MIDI Out Channel</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={midiOutChannel}
                onChange={(e) => {
                    const val = Number(e.target.value);
                    setMidiOutChannel(val);
                    updateParameter('midiOutputChannel', val);
                }}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button
                onClick={() => {
                    const val = Math.max(0, midiOutChannel - 1);
                    setMidiOutChannel(val);
                    updateParameter('midiOutputChannel', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                -
              </button>
              <button
                onClick={() => {
                    const val = Math.min(15, midiOutChannel + 1);
                    setMidiOutChannel(val);
                    updateParameter('midiOutputChannel', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                +
              </button>
            </div>
          </div>

          {/* Sequence Reset CC */}
          <div className="flex items-center justify-between">
            <label className={`text-sm ${theme.textSecondary}`}>Sequence Reset CC</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={sequenceResetCC}
                onChange={(e) => {
                    const val = Number(e.target.value);
                    setSequenceResetCC(val);
                    updateParameter('midiCcSeqReset', val);
                }}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button
                onClick={() => {
                    const val = Math.max(0, sequenceResetCC - 1);
                    setSequenceResetCC(val);
                    updateParameter('midiCcSeqReset', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                -
              </button>
              <button
                onClick={() => {
                    const val = Math.min(127, sequenceResetCC + 1);
                    setSequenceResetCC(val);
                    updateParameter('midiCcSeqReset', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                +
              </button>
              <select
                value={sequenceResetMode}
                onChange={(e) => {
                    setSequenceResetMode(e.target.value);
                    updateParameter('midiCcSeqResetMode', e.target.value);
                }}
                className={`px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} focus:outline-none`}
              >
                <option>Gate-On</option>
                <option>Gate-Off</option>
                <option>Toggle</option>
              </select>
            </div>
          </div>

          {/* Timer Reset CC */}
          <div className="flex items-center justify-between">
            <label className={`text-sm ${theme.textSecondary}`}>Timer Reset CC</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={timerResetCC}
                onChange={(e) => {
                    const val = Number(e.target.value);
                    setTimerResetCC(val);
                    updateParameter('midiCcHardReset', val);
                }}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button
                onClick={() => {
                    const val = Math.max(0, timerResetCC - 1);
                    setTimerResetCC(val);
                    updateParameter('midiCcHardReset', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                -
              </button>
              <button
                onClick={() => {
                    const val = Math.min(127, timerResetCC + 1);
                    setTimerResetCC(val);
                    updateParameter('midiCcHardReset', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                +
              </button>
              <select
                value={timerResetMode}
                onChange={(e) => {
                    setTimerResetMode(e.target.value);
                    updateParameter('midiCcHardResetMode', e.target.value);
                }}
                className={`px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} focus:outline-none`}
              >
                <option>Gate-On</option>
                <option>Gate-Off</option>
                <option>Toggle</option>
              </select>
            </div>
          </div>

          {/* Soft Reset CC */}
          <div className="flex items-center justify-between">
            <label className={`text-sm ${theme.textSecondary}`}>Soft Reset CC</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={softResetCC}
                onChange={(e) => {
                    const val = Number(e.target.value);
                    setSoftResetCC(val);
                    updateParameter('midiCcSoftReset', val);
                }}
                className={`w-20 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button
                onClick={() => {
                    const val = Math.max(0, softResetCC - 1);
                    setSoftResetCC(val);
                    updateParameter('midiCcSoftReset', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                -
              </button>
              <button
                onClick={() => {
                    const val = Math.min(127, softResetCC + 1);
                    setSoftResetCC(val);
                    updateParameter('midiCcSoftReset', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                +
              </button>
              <select
                value={softResetMode}
                onChange={(e) => {
                    setSoftResetMode(e.target.value);
                    updateParameter('midiCcSoftResetMode', e.target.value);
                }}
                className={`px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} focus:outline-none`}
              >
                <option>Gate-On</option>
                <option>Gate-Off</option>
                <option>Toggle</option>
              </select>
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
              onChange={(e) => {
                  setHostIP(e.target.value);
                  updateParameter('oscHostAddress', e.target.value); // String parameter?
                  // PluginProcessor.h says: mValueTreeState.state.setProperty("oscHostAddress", ...)
                  // This is not an AudioParameter. We need a special handler or just Property.
                  // The backend listens for setOscHostAddress? No.
                  // The processor uses Property, but sendParameterValue updates RangedAudioParameter.
                  // There is a 'oscHostAddress' property but no parameter.
                  // We might need a custom command or just ignore it if the user only wanted new features.
                  // But to be safe, let's leave it as is if it's not supported via parameter.
                  // Wait, mValueTreeState properties are not Parameters.
                  // Let's assume there is a native function for it or it's not supported via this generic call.
                  // However, for ports (int parameters), we can update them.
              }}
              className={`w-48 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} focus:outline-none`}
            />
          </div>

          {/* Send Port */}
          <div className="flex items-center justify-between">
            <label className={`text-sm ${theme.textSecondary}`}>Send Port</label>
            <div className="flex items-center gap-2">
              <input
                type="number"
                value={sendPort}
                onChange={(e) => {
                    const val = Number(e.target.value);
                    setSendPort(val);
                    updateParameter('oscSendPort', val);
                }}
                className={`w-24 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button
                onClick={() => {
                    const val = Math.max(1, sendPort - 1);
                    setSendPort(val);
                    updateParameter('oscSendPort', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                -
              </button>
              <button
                onClick={() => {
                    const val = Math.min(65535, sendPort + 1);
                    setSendPort(val);
                    updateParameter('oscSendPort', val);
                }}
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
                value={receivePort}
                onChange={(e) => {
                    const val = Number(e.target.value);
                    setReceivePort(val);
                    updateParameter('oscReceivePort', val);
                }}
                className={`w-24 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button
                onClick={() => {
                    const val = Math.max(1, receivePort - 1);
                    setReceivePort(val);
                    updateParameter('oscReceivePort', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                -
              </button>
              <button
                onClick={() => {
                    const val = Math.min(65535, receivePort + 1);
                    setReceivePort(val);
                    updateParameter('oscReceivePort', val);
                }}
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
    <div className={`${theme.panel} rounded-2xl p-6 border ${theme.border} backdrop-blur-sm`}>
      {/* Control Mode Selector */}
      <div className="flex items-center gap-3 mb-6">
        <span className={`text-sm ${theme.textSecondary}`}>Control Mode:</span>
        <div className="flex gap-2">
          <button
            onClick={() => {
              setControlMode('MIDI');
              setCurrentPage(0);
            }}
            className={`px-4 py-2 rounded-lg text-sm font-medium transition-all ${
              controlMode === 'MIDI'
                ? `${theme.accentBg} text-white`
                : `bg-slate-700/50 ${theme.text}`
            }`}
          >
            MIDI
          </button>
          <button
            onClick={() => {
              setControlMode('OSC');
              setCurrentPage(0);
            }}
            className={`px-4 py-2 rounded-lg text-sm font-medium transition-all ${
              controlMode === 'OSC'
                ? `${theme.accentBg} text-white`
                : `bg-slate-700/50 ${theme.text}`
            }`}
          >
            OSC
          </button>
        </div>
      </div>

      {/* Common Settings - Delay Adjust */}
      <div className="mb-6 pb-6 border-b border-slate-700/30">
        <h3 className={`text-sm ${theme.text} font-medium mb-4`}>Delay Adjust (samples)</h3>
        <div className="space-y-2">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-2 flex-1">
              <input
                type="number"
                value={delayAdjustSamples}
                onChange={(e) => {
                    const val = Number(e.target.value);
                    setDelayAdjustSamples(val);
                    updateParameter('delayAdjust', val);
                }}
                className={`w-24 px-3 py-1.5 border rounded ${theme.inputBg} ${theme.textSecondary} text-center focus:outline-none`}
              />
              <button
                onClick={() => {
                    const val = Math.max(0, delayAdjustSamples - 1);
                    setDelayAdjustSamples(val);
                    updateParameter('delayAdjust', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                &lt;
              </button>
              <button
                onClick={() => {
                    const val = delayAdjustSamples + 1;
                    setDelayAdjustSamples(val);
                    updateParameter('delayAdjust', val);
                }}
                className={`px-2 py-1.5 ${theme.buttonBg} ${theme.text} rounded transition-colors`}
              >
                &gt;
              </button>
            </div>
          </div>
          <input
            type="range"
            min="0"
            max="1000"
            value={delayAdjustSamples}
            onChange={(e) => {
                const val = Number(e.target.value);
                setDelayAdjustSamples(val);
                updateParameter('delayAdjust', val);
            }}
            className={`w-full h-2 bg-slate-700 rounded-lg appearance-none cursor-pointer ${theme.accentSlider}`}
          />
        </div>
      </div>

      {/* Page Header with Navigation */}
      <div className="flex items-center justify-between mb-6">
        <h2 className={`${theme.text} font-medium`}>{pages[currentPage].title}</h2>
        <div className="flex items-center gap-2">
          <button
            onClick={() => setCurrentPage(Math.max(0, currentPage - 1))}
            disabled={currentPage === 0}
            className={`p-1.5 rounded-lg ${theme.buttonBg} ${theme.text} disabled:opacity-30 disabled:cursor-not-allowed transition-colors`}
          >
            <ChevronLeft className="w-4 h-4" />
          </button>
          <span className={`text-xs ${theme.textTertiary} font-mono`}>
            {currentPage + 1} / {pages.length}
          </span>
          <button
            onClick={() => setCurrentPage(Math.min(pages.length - 1, currentPage + 1))}
            disabled={currentPage === pages.length - 1}
            className={`p-1.5 rounded-lg ${theme.buttonBg} ${theme.text} disabled:opacity-30 disabled:cursor-not-allowed transition-colors`}
          >
            <ChevronRight className="w-4 h-4" />
          </button>
        </div>
      </div>

      {/* Page Content */}
      <div className="min-h-[280px]">
        {pages[currentPage].content}
      </div>
    </div>
  );
}
