import { useJuceBridge } from '../../hooks/useJuceBridge';
import { useState, useRef } from 'react';

export type ThemeColor = 'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink';

interface MidiControllerProps {
  colorTheme: ThemeColor;
}

export function MidiController({ colorTheme }: MidiControllerProps) {
  const { parameters, sendParameter } = useJuceBridge();
  const [activeButton, setActiveButton] = useState<number | null>(null);
  const radioGroupRef = useRef<HTMLDivElement>(null);

  // Theme colors
  const themeColors = {
    green: {
      panel: 'bg-green-950/30',
      border: 'border-green-900/50',
      text: 'text-green-300',
      textSecondary: 'text-green-300/80',
      buttonBg: 'bg-green-900/30 hover:bg-green-800/50',
      buttonActive: 'bg-green-600',
      accentSlider: 'accent-green-600',
    },
    blue: {
      panel: 'bg-blue-950/30',
      border: 'border-blue-900/50',
      text: 'text-blue-300',
      textSecondary: 'text-blue-300/80',
      buttonBg: 'bg-blue-900/30 hover:bg-blue-800/50',
      buttonActive: 'bg-blue-600',
      accentSlider: 'accent-blue-600',
    },
    purple: {
      panel: 'bg-purple-950/30',
      border: 'border-purple-900/50',
      text: 'text-purple-300',
      textSecondary: 'text-purple-300/80',
      buttonBg: 'bg-purple-900/30 hover:bg-purple-800/50',
      buttonActive: 'bg-purple-600',
      accentSlider: 'accent-purple-600',
    },
    red: {
      panel: 'bg-red-950/30',
      border: 'border-red-900/50',
      text: 'text-red-300',
      textSecondary: 'text-red-300/80',
      buttonBg: 'bg-red-900/30 hover:bg-red-800/50',
      buttonActive: 'bg-red-600',
      accentSlider: 'accent-red-600',
    },
    orange: {
      panel: 'bg-orange-950/30',
      border: 'border-orange-900/50',
      text: 'text-orange-300',
      textSecondary: 'text-orange-300/80',
      buttonBg: 'bg-orange-900/30 hover:bg-orange-800/50',
      buttonActive: 'bg-orange-600',
      accentSlider: 'accent-orange-600',
    },
    cyan: {
      panel: 'bg-cyan-950/30',
      border: 'border-cyan-900/50',
      text: 'text-cyan-300',
      textSecondary: 'text-cyan-300/80',
      buttonBg: 'bg-cyan-900/30 hover:bg-cyan-800/50',
      buttonActive: 'bg-cyan-600',
      accentSlider: 'accent-cyan-600',
    },
    pink: {
      panel: 'bg-pink-950/30',
      border: 'border-pink-900/50',
      text: 'text-pink-300',
      textSecondary: 'text-pink-300/80',
      buttonBg: 'bg-pink-900/30 hover:bg-pink-800/50',
      buttonActive: 'bg-pink-600',
      accentSlider: 'accent-pink-600',
    },
  };

  const theme = themeColors[colorTheme] || themeColors.green;

  // Get current values from parameters
  const sliderValue = parameters['sliderValue'] || 0;

  // Radio button handlers with slide tracking
  const handleRadioButtonInteraction = (clientX: number) => {
    if (!radioGroupRef.current) return;

    const rect = radioGroupRef.current.getBoundingClientRect();
    const relativeX = clientX - rect.left;
    const buttonWidth = rect.width / 8;
    const index = Math.floor(relativeX / buttonWidth);

    if (index >= 0 && index <= 7) {
      setActiveButton(index);
      sendParameter('radioGroupSelection', index);
    }
  };

  const handleRadioButtonStart = (e: React.MouseEvent | React.TouchEvent) => {
    const clientX = 'touches' in e ? e.touches[0].clientX : e.clientX;
    handleRadioButtonInteraction(clientX);
  };

  const handleRadioButtonMove = (e: React.MouseEvent | React.TouchEvent) => {
    if (activeButton === null) return;
    const clientX = 'touches' in e ? e.touches[0].clientX : e.clientX;
    handleRadioButtonInteraction(clientX);
  };

  const handleRadioButtonEnd = () => {
    // Send 0 when finger is released
    sendParameter('radioGroupSelection', -1); // Use -1 to indicate no selection
    setActiveButton(null);
  };

  // Push button handlers
  const handlePushButtonDown = () => {
    sendParameter('pushButtonState', 1);
  };

  const handlePushButtonUp = () => {
    sendParameter('pushButtonState', 0);
  };

  // Slider handler
  const handleSliderChange = (value: number) => {
    sendParameter('sliderValue', value);
  };

  return (
    <div className={`flex flex-col gap-4 px-4 py-3 rounded-xl border ${theme.border} ${theme.panel}`}>
      {/* Radio Button Group */}
      <div className="flex flex-col gap-2">
        <span className={`text-xs font-bold tracking-wider ${theme.textSecondary}`}>
         RETRIGGER (EXT MIDI) 
        </span>
        <div 
          ref={radioGroupRef}
          className="flex gap-2"
          onMouseDown={handleRadioButtonStart}
          onMouseMove={handleRadioButtonMove}
          onMouseUp={handleRadioButtonEnd}
          onMouseLeave={handleRadioButtonEnd}
          onTouchStart={handleRadioButtonStart}
          onTouchMove={handleRadioButtonMove}
          onTouchEnd={handleRadioButtonEnd}
        >
          {[0, 1, 2, 3, 4, 5, 6, 7].map((index) => (
            <div
              key={index}
              className={`flex-1 py-3 rounded-lg text-sm font-bold transition-all text-center ${
                activeButton === index
                  ? `${theme.buttonActive} text-white shadow-lg`
                  : `${theme.buttonBg} ${theme.text}`
              }`}
            >
              {index + 1}
            </div>
          ))}
        </div>
      </div>

      {/* Push Button & Slider */}
      <div className="flex items-center gap-3">
        <span className={`text-xs font-bold tracking-wider ${theme.textSecondary}`}>
          SLOW (EXT MIDI)
        </span>
        
        {/* Push Button (Dot) */}
        <button
          onMouseDown={handlePushButtonDown}
          onMouseUp={handlePushButtonUp}
          onMouseLeave={handlePushButtonUp}
          onTouchStart={handlePushButtonDown}
          onTouchEnd={handlePushButtonUp}
          className={`w-12 h-12 rounded-full ${theme.buttonActive} hover:opacity-80 active:scale-95 transition-all shadow-lg`}
        >
          <span className="text-white text-xl font-bold">●</span>
        </button>

        {/* Slider (Bar) */}
        <div className="flex-1">
          <input
            type="range"
            min="0"
            max="1"
            step="0.01"
            value={sliderValue}
            onChange={(e) => handleSliderChange(Number(e.target.value))}
            className={`w-full h-2 bg-slate-700 rounded-lg appearance-none cursor-pointer ${theme.accentSlider}`}
          />
        </div>
      </div>
    </div>
  );
}
