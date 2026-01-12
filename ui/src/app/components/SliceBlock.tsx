interface SliceBlockProps {
  index: number;
  waveform: number[];
  isActive: boolean;
  isPlaying: boolean;
  isSwiping: boolean;
  onClick: () => void;
  onPointerDown: () => void;
  onPointerEnter: () => void;
}

export function SliceBlock({
  index,
  waveform,
  isActive,
  isPlaying,
  isSwiping,
  onClick,
  onPointerDown,
  onPointerEnter,
}: SliceBlockProps) {
  return (
    <button
      className={`
        relative rounded-lg overflow-hidden transition-all cursor-pointer
        ${isActive 
          ? 'bg-gradient-to-br from-blue-600/30 to-blue-800/30 border-2 border-blue-500/70' 
          : 'bg-slate-700/30 border-2 border-slate-600/30'
        }
        ${isPlaying ? 'ring-4 ring-blue-400/50 scale-105' : ''}
        ${isSwiping ? 'scale-95' : 'hover:scale-105'}
      `}
      onClick={onClick}
      onPointerDown={onPointerDown}
      onPointerEnter={onPointerEnter}
      style={{ touchAction: 'none' }}
    >
      {/* Slice Number */}
      <div className="absolute top-1 left-1.5 z-10">
        <span className={`
          text-[10px] font-mono font-semibold
          ${isActive ? 'text-blue-300' : 'text-slate-500'}
        `}>
          {index + 1}
        </span>
      </div>

      {/* Waveform Canvas */}
      <div className="w-full h-full flex items-center justify-center p-2 pt-5">
        <svg
          width="100%"
          height="100%"
          viewBox="0 0 100 60"
          preserveAspectRatio="none"
          className="opacity-70"
        >
          <path
            d={generateWaveformPath(waveform)}
            fill="none"
            stroke={isActive ? '#60a5fa' : '#64748b'}
            strokeWidth="2"
            vectorEffect="non-scaling-stroke"
          />
        </svg>
      </div>

      {/* Playing Indicator */}
      {isPlaying && (
        <div className="absolute inset-0 bg-blue-400/20 animate-pulse" />
      )}

      {/* Active Glow */}
      {isActive && (
        <div className="absolute inset-0 bg-gradient-to-t from-blue-500/10 to-transparent pointer-events-none" />
      )}
    </button>
  );
}

function generateWaveformPath(waveform: number[]): string {
  const points = waveform.length;
  const width = 100;
  const height = 60;
  const centerY = height / 2;

  let path = '';

  waveform.forEach((value, i) => {
    const x = (i / (points - 1)) * width;
    const y = centerY + value * (height * 0.4);
    
    if (i === 0) {
      path += `M ${x} ${y}`;
    } else {
      path += ` L ${x} ${y}`;
    }
  });

  return path;
}
