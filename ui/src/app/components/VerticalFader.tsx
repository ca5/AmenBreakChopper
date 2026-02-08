import { useState } from 'react';

export type ThemeColor = 'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink';

interface VerticalFaderProps {
  value: number; // 0-127
  onChange: (value: number) => void;
  ccNumber: number;
  label: string;
  colorTheme: ThemeColor;
}

export function VerticalFader({ value, onChange, ccNumber, label, colorTheme }: VerticalFaderProps) {
  const [showValue, setShowValue] = useState(false);

  // Theme colors
  const themeColors = {
    green: {
      track: 'bg-green-900/30',
      thumb: 'accent-green-600',
      text: 'text-green-300',
      textSecondary: 'text-green-300/70',
      valueBg: 'bg-green-600',
    },
    blue: {
      track: 'bg-blue-900/30',
      thumb: 'accent-blue-600',
      text: 'text-blue-300',
      textSecondary: 'text-blue-300/70',
      valueBg: 'bg-blue-600',
    },
    purple: {
      track: 'bg-purple-900/30',
      thumb: 'accent-purple-600',
      text: 'text-purple-300',
      textSecondary: 'text-purple-300/70',
      valueBg: 'bg-purple-600',
    },
    red: {
      track: 'bg-red-900/30',
      thumb: 'accent-red-600',
      text: 'text-red-300',
      textSecondary: 'text-red-300/70',
      valueBg: 'bg-red-600',
    },
    orange: {
      track: 'bg-orange-900/30',
      thumb: 'accent-orange-600',
      text: 'text-orange-300',
      textSecondary: 'text-orange-300/70',
      valueBg: 'bg-orange-600',
    },
    cyan: {
      track: 'bg-cyan-900/30',
      thumb: 'accent-cyan-600',
      text: 'text-cyan-300',
      textSecondary: 'text-cyan-300/70',
      valueBg: 'bg-cyan-600',
    },
    pink: {
      track: 'bg-pink-900/30',
      thumb: 'accent-pink-600',
      text: 'text-pink-300',
      textSecondary: 'text-pink-300/70',
      valueBg: 'bg-pink-600',
    },
  };

  const theme = themeColors[colorTheme] || themeColors.green;

  return (
    <div className="flex flex-col items-center gap-1 relative">
      {/* Label */}
      <span className={`text-xs font-bold ${theme.textSecondary}`}>{label}</span>
      
      {/* Fader Container */}
      <div className="relative flex items-center justify-center" style={{ height: '140px', width: '50px' }}>
        {/* Vertical Range Input */}
        <input
          type="range"
          min="0"
          max="127"
          value={value}
          onChange={(e) => onChange(Number(e.target.value))}
          onMouseDown={() => setShowValue(true)}
          onMouseUp={() => setShowValue(false)}
          onTouchStart={() => setShowValue(true)}
          onTouchEnd={() => setShowValue(false)}
          className={`appearance-none cursor-pointer ${theme.thumb}`}
          style={{
            width: '140px',
            height: '8px',
            transform: 'rotate(-90deg)',
            transformOrigin: 'center',
            background: `linear-gradient(to right, ${theme.track.includes('green') ? '#166534' : theme.track.includes('blue') ? '#1e3a8a' : theme.track.includes('purple') ? '#581c87' : theme.track.includes('red') ? '#7f1d1d' : theme.track.includes('orange') ? '#7c2d12' : theme.track.includes('cyan') ? '#164e63' : '#831843'} 0%, ${theme.track.includes('green') ? '#166534' : theme.track.includes('blue') ? '#1e3a8a' : theme.track.includes('purple') ? '#581c87' : theme.track.includes('red') ? '#7f1d1d' : theme.track.includes('orange') ? '#7c2d12' : theme.track.includes('cyan') ? '#164e63' : '#831843'} 100%)`,
            borderRadius: '4px',
          }}
        />
        
        {/* Value Display (Popup) */}
        {showValue && (
          <div className={`absolute top-0 right-0 px-2 py-1 rounded ${theme.valueBg} text-white text-xs font-bold shadow-lg z-10`}>
            {value}
          </div>
        )}
      </div>

      {/* CC Number */}
      <span className={`text-[10px] ${theme.textSecondary}`}>CC {ccNumber}</span>
      
      {/* Current Value (Always Visible) */}
      <span className={`text-xs font-mono ${theme.text}`}>{value}</span>
    </div>
  );
}
