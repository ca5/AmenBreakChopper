import { useState, useRef, useEffect, useCallback } from 'react';

export type ThemeColor = 'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink';

interface VerticalFaderProps {
  value: number; // 0-127
  onChange: (value: number) => void;
  ccNumber: number;
  label: string;
  colorTheme: ThemeColor;
}

export function VerticalFader({ value, onChange, ccNumber, label, colorTheme }: VerticalFaderProps) {
  const [isDragging, setIsDragging] = useState(false);
  const trackRef = useRef<HTMLDivElement>(null);
  
  // Refs for relative dragging to avoid closure staleness and re-renders
  const dragStartY = useRef<number>(0);
  const dragStartValue = useRef<number>(0);

  // Theme colors
  const themeColors = {
    green: {
      track: 'bg-green-900/30',
      fill: 'bg-green-600',
      thumb: 'bg-green-400',
      text: 'text-green-300',
      textSecondary: 'text-green-300/70',
      border: 'border-green-500/50',
    },
    blue: {
      track: 'bg-blue-900/30',
      fill: 'bg-blue-600',
      thumb: 'bg-blue-400',
      text: 'text-blue-300',
      textSecondary: 'text-blue-300/70',
      border: 'border-blue-500/50',
    },
    purple: {
      track: 'bg-purple-900/30',
      fill: 'bg-purple-600',
      thumb: 'bg-purple-400',
      text: 'text-purple-300',
      textSecondary: 'text-purple-300/70',
      border: 'border-purple-500/50',
    },
    red: {
      track: 'bg-red-900/30',
      fill: 'bg-red-600',
      thumb: 'bg-red-400',
      text: 'text-red-300',
      textSecondary: 'text-red-300/70',
      border: 'border-red-500/50',
    },
    orange: {
      track: 'bg-orange-900/30',
      fill: 'bg-orange-600',
      thumb: 'bg-orange-400',
      text: 'text-orange-300',
      textSecondary: 'text-orange-300/70',
      border: 'border-orange-500/50',
    },
    cyan: {
      track: 'bg-cyan-900/30',
      fill: 'bg-cyan-600',
      thumb: 'bg-cyan-400',
      text: 'text-cyan-300',
      textSecondary: 'text-cyan-300/70',
      border: 'border-cyan-500/50',
    },
    pink: {
      track: 'bg-pink-900/30',
      fill: 'bg-pink-600',
      thumb: 'bg-pink-400',
      text: 'text-pink-300',
      textSecondary: 'text-pink-300/70',
      border: 'border-pink-500/50',
    },
  };

  const theme = themeColors[colorTheme] || themeColors.green;

  // Calculate percentage for display
  const percentage = (value / 127) * 100;

  const handlePointerDown = (e: React.PointerEvent) => {
    e.preventDefault(); // Prevent default browser actions
    e.currentTarget.setPointerCapture(e.pointerId);
    
    setIsDragging(true);
    
    // Record start state for relative dragging
    dragStartY.current = e.clientY;
    dragStartValue.current = value;
  };

  const handlePointerMove = (e: React.PointerEvent) => {
    if (isDragging && trackRef.current) {
        e.preventDefault();
        
        const rect = trackRef.current.getBoundingClientRect();
        const trackHeight = rect.height;
        
        // Calculate delta Y (Up is negative in screen coords, but we want positive change)
        // Dragging UP (smaller Y) should INCREASE value.
        // DeltaY = StartY - CurrentY
        // If StartY = 100, CurrentY = 90 (moved up 10px), Delta = 10
        const deltaY = dragStartY.current - e.clientY;
        
        // Calculate value change scale
        // Full height = 127 value steps (approx 1px = 1 step if height is ~120px)
        const deltaValue = (deltaY / trackHeight) * 127;
        
        // Apply relative change
        const newValue = Math.min(127, Math.max(0, Math.round(dragStartValue.current + deltaValue)));
        
        if (newValue !== value) {
            onChange(newValue);
        }
    }
  };

  const handlePointerUp = (e: React.PointerEvent) => {
    e.preventDefault();
    setIsDragging(false);
    e.currentTarget.releasePointerCapture(e.pointerId);
  };

  return (
    <div className="flex flex-col items-center gap-1 relative select-none touch-none">
      {/* Label */}
      <span className={`text-xs font-bold ${theme.textSecondary}`}>{label}</span>
      
      {/* Fader Track Area */}
      <div 
        ref={trackRef}
        className={`relative w-12 h-[120px] rounded-lg overflow-hidden cursor-pointer touch-none ${theme.track} border ${theme.border}`}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onPointerCancel={handlePointerUp}
      >
        {/* Fill Level (Background of the active area) */}
        <div 
          className={`absolute bottom-0 left-0 right-0 ${theme.fill} opacity-30 pointer-events-none transition-all duration-75 ease-out`}
          style={{ height: `${percentage}%` }}
        />

        {/* Thumb / Handle */}
        <div 
          className={`absolute left-0 right-0 h-8 rounded-sm mx-1 shadow-lg pointer-events-none transition-all duration-75 ease-out flex items-center justify-center border border-white/20 ${theme.thumb}`}
          style={{ 
            bottom: `calc(${percentage}% - 16px)`, // Center thumb on value
            // Clamp thumb visual position to keep inside track
            // Actually, standard fader behavior allows thumb center to hit 0 and 100%
            // But visually we might want to constrain it slightly or let it overlap.
            // Let's keep it simple: center of thumb represents value.
          }}
        >
            {/* Grip lines */}
            <div className="w-6 h-[1px] bg-black/30 mb-[2px]"></div>
            <div className="w-6 h-[1px] bg-black/30 mt-[2px]"></div>
        </div>

        {/* Value Overlay (Always Visible while dragging or large enough to read) */}
        <div className={`absolute inset-0 flex items-center justify-center pointer-events-none`}>
            <span className={`text-xs font-bold font-mono drop-shadow-md ${isDragging ? 'text-white scale-125' : 'text-white/50'} transition-all`}>
                {value}
            </span>
        </div>
      </div>

      {/* CC Number */}
      <span className={`text-[10px] ${theme.textSecondary}`}>CC {ccNumber}</span>
    </div>
  );
}
