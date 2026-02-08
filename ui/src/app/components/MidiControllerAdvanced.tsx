import { useState } from 'react';
import { useJuceBridge } from '../../hooks/useJuceBridge';
import { FaderUnit3 } from './FaderUnit3';
import { FaderUnit2 } from './FaderUnit2';
import { ThemeColor } from './VerticalFader';

interface MidiControllerAdvancedProps {
  colorTheme: ThemeColor;
}

export function MidiControllerAdvanced({ colorTheme }: MidiControllerAdvancedProps) {
  const { parameters, sendParameter } = useJuceBridge();

  // Get current values from parameters (0.0-1.0, convert to 0-127)
  const fader1Value = Math.round((parameters['faderAdvanced1'] || 0) * 127);
  const fader2Value = Math.round((parameters['faderAdvanced2'] || 0) * 127);
  const fader3Value = Math.round((parameters['faderAdvanced3'] || 0) * 127);
  const fader4Value = Math.round((parameters['faderAdvanced4'] || 0) * 127);
  const fader5Value = Math.round((parameters['faderAdvanced5'] || 0) * 127);

  const toggle1State = (parameters['toggleAdvanced1'] || 0) > 0.5;
  const toggle2State = (parameters['toggleAdvanced2'] || 0) > 0.5;

  // Get CC numbers and MIDI channel from parameters (default values)
  const midiChannel = Math.round(parameters['midiChannelAdvanced'] || 1);
  const ccFader1 = Math.round(parameters['ccFaderAdvanced1'] || 1);
  const ccFader2 = Math.round(parameters['ccFaderAdvanced2'] || 2);
  const ccFader3 = Math.round(parameters['ccFaderAdvanced3'] || 3);
  const ccFader4 = Math.round(parameters['ccFaderAdvanced4'] || 13);
  const ccFader5 = Math.round(parameters['ccFaderAdvanced5'] || 14);
  const ccToggle1 = Math.round(parameters['ccToggleAdvanced1'] || 0);
  const ccToggle2 = Math.round(parameters['ccToggleAdvanced2'] || 12);

  // Handlers - use sendMidiCC directly
  const handleFader1Change = (value: number) => {
    console.log('[MIDI Advanced UI] Fader 1 changed:', value);
    sendParameter('faderAdvanced1', value / 127);
  };

  const handleFader2Change = (value: number) => {
    console.log('[MIDI Advanced UI] Fader 2 changed:', value);
    sendParameter('faderAdvanced2', value / 127);
  };

  const handleFader3Change = (value: number) => {
    console.log('[MIDI Advanced UI] Fader 3 changed:', value);
    sendParameter('faderAdvanced3', value / 127);
  };

  const handleFader4Change = (value: number) => {
    console.log('[MIDI Advanced UI] Fader 4 changed:', value);
    sendParameter('faderAdvanced4', value / 127);
  };

  const handleFader5Change = (value: number) => {
    console.log('[MIDI Advanced UI] Fader 5 changed:', value);
    sendParameter('faderAdvanced5', value / 127);
  };

  const handleToggle1 = () => {
    console.log('[MIDI Advanced UI] Toggle 1 clicked, current state:', toggle1State);
    const newState = toggle1State ? 0 : 1;
    sendParameter('toggleAdvanced1', newState);
  };

  const handleToggle2 = () => {
    console.log('[MIDI Advanced UI] Toggle 2 clicked, current state:', toggle2State);
    const newState = toggle2State ? 0 : 1;
    sendParameter('toggleAdvanced2', newState);
  };

  // Theme colors for panel
  const themeColors = {
    green: {
      panel: 'bg-green-950/30',
      border: 'border-green-900/50',
      text: 'text-green-300',
    },
    blue: {
      panel: 'bg-blue-950/30',
      border: 'border-blue-900/50',
      text: 'text-blue-300',
    },
    purple: {
      panel: 'bg-purple-950/30',
      border: 'border-purple-900/50',
      text: 'text-purple-300',
    },
    red: {
      panel: 'bg-red-950/30',
      border: 'border-red-900/50',
      text: 'text-red-300',
    },
    orange: {
      panel: 'bg-orange-950/30',
      border: 'border-orange-900/50',
      text: 'text-orange-300',
    },
    cyan: {
      panel: 'bg-cyan-950/30',
      border: 'border-cyan-900/50',
      text: 'text-cyan-300',
    },
    pink: {
      panel: 'bg-pink-950/30',
      border: 'border-pink-900/50',
      text: 'text-pink-300',
    },
  };

  const theme = themeColors[colorTheme] || themeColors.green;

  // Active unit state (1 = 3-fader unit, 2 = 2-fader unit)
  const [activeUnit, setActiveUnit] = useState<1 | 2>(1);

  return (
    <div className={`flex flex-col gap-2 px-3 py-1.5 rounded-xl border ${theme.border} ${theme.panel}`}>
      {/* Title */}
      <h2 className={`text-xs font-bold ${theme.text} text-center`}> </h2>

      {/* Unit Tabs */}
      <div className="flex gap-2 mt-8">
        <button
          onClick={() => setActiveUnit(1)}
          className={`flex-1 px-2 py-0.5 rounded-md text-xs font-bold transition-all ${
            activeUnit === 1
              ? `${theme.panel.replace('/30', '/50')} ${theme.text} border ${theme.border}`
              : `bg-slate-900/30 text-slate-400 border border-slate-700/30`
          }`}
        >
          Unit 1 (3 Faders)
        </button>
        <button
          onClick={() => setActiveUnit(2)}
          className={`flex-1 px-2 py-0.5 rounded-md text-xs font-bold transition-all ${
            activeUnit === 2
              ? `${theme.panel.replace('/30', '/50')} ${theme.text} border ${theme.border}`
              : `bg-slate-900/30 text-slate-400 border border-slate-700/30`
          }`}
        >
          Unit 2 (2 Faders)
        </button>
      </div>

      {/* 3-Fader Unit */}
      {activeUnit === 1 && (
        <FaderUnit3
          fader1Value={fader1Value}
          fader2Value={fader2Value}
          fader3Value={fader3Value}
          toggleState={toggle1State}
          onFader1Change={handleFader1Change}
          onFader2Change={handleFader2Change}
          onFader3Change={handleFader3Change}
          onToggle={handleToggle1}
          ccFader1={ccFader1}
          ccFader2={ccFader2}
          ccFader3={ccFader3}
          ccToggle={ccToggle1}
          colorTheme={colorTheme}
        />
      )}

      {/* 2-Fader Unit */}
      {activeUnit === 2 && (
        <FaderUnit2
          fader1Value={fader4Value}
          fader2Value={fader5Value}
          toggleState={toggle2State}
          onFader1Change={handleFader4Change}
          onFader2Change={handleFader5Change}
          onToggle={handleToggle2}
          ccFader1={ccFader4}
          ccFader2={ccFader5}
          ccToggle={ccToggle2}
          colorTheme={colorTheme}
        />
      )}
    </div>
  );
}
