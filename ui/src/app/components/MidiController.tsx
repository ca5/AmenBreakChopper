import { useJuceBridge } from '../../hooks/useJuceBridge';
import { useState, useRef } from 'react';

export type ThemeColor = 'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink';

interface MidiControllerProps {
  colorTheme: ThemeColor;
  onSwitchToTiming?: () => void;
}

export function MidiController({ colorTheme, onSwitchToTiming }: MidiControllerProps) {
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
  const radioButtonState = Math.round(parameters['radioButtonState'] || 0);
  const sliderValue = Math.round((parameters['sliderValue'] || 0) * 127);

  // Debug state
  const [debugInfo, setDebugInfo] = useState<string>('');

  // Radio button handlers - Unified logic for both Tap and Slide
  const handleRadioButtonInteraction = (clientX: number, clientY: number, source: string) => {
    let debugLog = `Source: ${source}\nPos: ${Math.round(clientX)}, ${Math.round(clientY)}\n`;

    // 1. Try elementFromPoint (High Precision)
    const element = document.elementFromPoint(clientX, clientY);
    debugLog += `ElFromPoint: ${element ? element.tagName : 'null'}\n`;
    
    if (element) {
      const button = element.closest('[data-button-index]');
      debugLog += `ClosestBtn: ${button ? parseInt(button.getAttribute('data-button-index')!) + 1 : 'null'}\n`;
      
      if (button) {
        const indexStr = button.getAttribute('data-button-index');
        if (indexStr !== null) {
          const index = parseInt(indexStr);
          if (index >= 0 && index < 8) {
            setActiveButton(index);
            sendParameter('radioButtonState', index + 1);
            setDebugInfo(debugLog + `Action: Select Button ${index + 1} (Element)`);
            return;
          }
        }
      }
    }

    // 2. Fallback: Check Nearest Button Center (Robust against gaps/offsets)
    // This solves issues where the user taps slightly between buttons or outside the strict rect
    if (!radioGroupRef.current) {
        setDebugInfo(debugLog + "Error: No Ref");
        return;
    }
    
    const buttons = radioGroupRef.current.querySelectorAll('[data-button-index]');
    
    let minDistance = Infinity;
    let nearestIndex = -1;

    for (let i = 0; i < buttons.length; i++) {
      const button = buttons[i] as HTMLElement;
      const rect = button.getBoundingClientRect();
      
      // Check if Y is within reasonable vertical range (+- 30px buffer to allows inaccurate vertical taps)
      if (clientY >= rect.top - 30 && clientY <= rect.bottom + 30) {
          const centerX = rect.left + rect.width / 2;
          const distance = Math.abs(clientX - centerX);
          
          if (distance < minDistance) {
              minDistance = distance;
              const indexStr = button.getAttribute('data-button-index');
              if (indexStr !== null) {
                nearestIndex = parseInt(indexStr);
              }
          }
      }
    }
    
    // Determine cutoff distance (approx half button width + gap)
    // If buttons are ~40px wide with 8px gap, max dist should be around 24-25px
    const cutoffDistance = 30; // Generous allowance

    if (nearestIndex >= 0 && minDistance < cutoffDistance) {
         setActiveButton(nearestIndex);
         sendParameter('radioButtonState', nearestIndex + 1);
         setDebugInfo(debugLog + `Action: Select Button ${nearestIndex + 1} (Nearest: ${Math.round(minDistance)}px)`);
         return;
    }
    
    setDebugInfo(debugLog + "No hit detected");
  };

  const handleRadioButtonStart = (e: React.MouseEvent | React.TouchEvent) => {
    // Prevent default behavior to stop scrolling/selection
    if (e.cancelable && 'touches' in e) {
      e.preventDefault();
    }

    const clientX = 'touches' in e ? e.touches[0].clientX : e.clientX;
    const clientY = 'touches' in e ? e.touches[0].clientY : e.clientY;
    
    // Use the exact same logic as Move (Interaction)
    handleRadioButtonInteraction(clientX, clientY, 'Start');
  };

  const handleRadioButtonMove = (e: React.MouseEvent | React.TouchEvent) => {
    // Prevent scrolling while dragging
    if (e.cancelable && 'touches' in e) {
      e.preventDefault();
    }
    
    const clientX = 'touches' in e ? e.touches[0].clientX : e.clientX;
    const clientY = 'touches' in e ? e.touches[0].clientY : e.clientY;
    handleRadioButtonInteraction(clientX, clientY, 'Move');
  };

  const handleRadioButtonEnd = () => {
    setActiveButton(null);
    // Send 0 when button is released (per controller.md spec)
    sendParameter('radioButtonState', 0);
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
    sendParameter('sliderValue', value / 127);
  };

  return (
    <>
      {/* Radio Button Group */}
      <div className="flex items-center gap-2 w-full justify-between">
        <button
          onClick={onSwitchToTiming}
          className={`px-3 py-1 rounded-md text-xs font-bold transition-all ${theme.panel} ${theme.text} border ${theme.border} ${onSwitchToTiming ? 'hover:opacity-80' : ''}`}
        >
         RETRIGGER (EXT MIDI) 
        </button>
      </div>
      
      <div className="w-full">
        <div 
          ref={radioGroupRef}
          className="grid grid-cols-8 gap-2 w-full"
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
              data-button-index={index}
              className={`py-3 rounded-lg text-sm font-bold transition-all text-center ${
                radioButtonState !== 0 && radioButtonState === index + 1
                  ? `${theme.buttonActive} text-white shadow-lg`
                  : `${theme.buttonBg} ${theme.text}`
              }`}
            >
              {index + 1}
            </div>
          ))}
        </div>
        {/* Debug Overlay */}
        <div className="absolute top-0 right-0 bg-black/80 text-green-400 text-[10px] p-1 pointer-events-none z-50 whitespace-pre font-mono max-w-[150px] opacity-70">
          {debugInfo}
        </div>
      </div>

      {/* Push Button & Slider */}
      <div className="flex items-center gap-3 w-full">
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
            max="127"
            step="1"
            value={sliderValue}
            onChange={(e) => handleSliderChange(Number(e.target.value))}
            className={`w-full h-2 bg-slate-700 rounded-lg appearance-none cursor-pointer ${theme.accentSlider}`}
          />
        </div>
      </div>
    </>
  );
}
