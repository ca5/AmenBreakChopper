interface CircularSliceBlockProps {
  index: number;
  waveform: number[];
  startAngle: number;
  angleSpan: number;
  innerRadius: number;
  outerRadius: number;
  centerX: number;
  centerY: number;
  isActive: boolean;
  isOriginalPlaying: boolean;
  isTriggeredPlaying: boolean;
  isSwiping: boolean;
  colorTheme: 'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink';
  onClick: () => void;
  onPointerDown: () => void;
  onPointerEnter: () => void;
}

export function CircularSliceBlock({
  index,
  waveform,
  startAngle,
  angleSpan,
  innerRadius,
  outerRadius,
  centerX,
  centerY,
  isActive,
  isOriginalPlaying,
  isTriggeredPlaying,
  isSwiping,
  colorTheme,
  onClick,
  onPointerDown,
  onPointerEnter,
}: CircularSliceBlockProps) {
  const gap = 2; // Gap between slices in degrees
  const actualStartAngle = startAngle + gap / 2;
  const actualAngleSpan = angleSpan - gap;

  // Convert polar to cartesian coordinates
  const polarToCartesian = (angle: number, radius: number) => {
    const angleInRadians = ((angle - 90) * Math.PI) / 180.0;
    return {
      x: centerX + radius * Math.cos(angleInRadians),
      y: centerY + radius * Math.sin(angleInRadians),
    };
  };

  // Create the slice segment path
  const endAngle = actualStartAngle + actualAngleSpan;

  const outerStart = polarToCartesian(actualStartAngle, outerRadius);
  const outerEnd = polarToCartesian(endAngle, outerRadius);
  const innerStart = polarToCartesian(actualStartAngle, innerRadius);
  const innerEnd = polarToCartesian(endAngle, innerRadius);

  const largeArcFlag = actualAngleSpan <= 180 ? '0' : '1';

  // Create proper fan/wedge shape path
  const slicePath = `
    M ${outerStart.x} ${outerStart.y}
    A ${outerRadius} ${outerRadius} 0 ${largeArcFlag} 1 ${outerEnd.x} ${outerEnd.y}
    L ${innerEnd.x} ${innerEnd.y}
    A ${innerRadius} ${innerRadius} 0 ${largeArcFlag} 0 ${innerStart.x} ${innerStart.y}
    Z
  `;

  // Create curved waveform path along the arc
  const createWaveformPath = () => {
    const points = waveform.length;
    const waveformRadius = (innerRadius + outerRadius) / 2;
    const amplitude = (outerRadius - innerRadius) * 0.35;

    let path = '';

    waveform.forEach((value, i) => {
      const angle = actualStartAngle + (i / (points - 1)) * actualAngleSpan;
      const radius = waveformRadius + value * amplitude;
      const point = polarToCartesian(angle, radius);

      if (i === 0) {
        path += `M ${point.x} ${point.y}`;
      } else {
        path += ` L ${point.x} ${point.y}`;
      }
    });

    return path;
  };

  // Calculate label position (middle of the slice)
  const labelAngle = actualStartAngle + actualAngleSpan / 2;
  const labelRadius = (innerRadius + outerRadius) / 2;
  const labelPos = polarToCartesian(labelAngle, labelRadius);

  // Determine colors based on even/odd index - Green theme
  const isEven = index % 2 === 0;

  // Theme-specific colors
  const themeColors = {
    green: {
      active: { stroke: isEven ? '#22c55e' : '#16a34a', waveform: isEven ? '#4ade80' : '#22c55e', label: isEven ? '#86efac' : '#4ade80' },
      inactive: { stroke: isEven ? '#475569' : '#3f3f46', waveform: isEven ? '#64748b' : '#52525b', label: isEven ? '#64748b' : '#52525b' },
    },
    blue: {
      active: { stroke: isEven ? '#3b82f6' : '#2563eb', waveform: isEven ? '#60a5fa' : '#3b82f6', label: isEven ? '#93c5fd' : '#60a5fa' },
      inactive: { stroke: isEven ? '#475569' : '#3f3f46', waveform: isEven ? '#64748b' : '#52525b', label: isEven ? '#64748b' : '#52525b' },
    },
    purple: {
      active: { stroke: isEven ? '#a855f7' : '#9333ea', waveform: isEven ? '#c084fc' : '#a855f7', label: isEven ? '#d8b4fe' : '#c084fc' },
      inactive: { stroke: isEven ? '#475569' : '#3f3f46', waveform: isEven ? '#64748b' : '#52525b', label: isEven ? '#64748b' : '#52525b' },
    },
    red: {
      active: { stroke: isEven ? '#ef4444' : '#dc2626', waveform: isEven ? '#f87171' : '#ef4444', label: isEven ? '#fca5a5' : '#f87171' },
      inactive: { stroke: isEven ? '#475569' : '#3f3f46', waveform: isEven ? '#64748b' : '#52525b', label: isEven ? '#64748b' : '#52525b' },
    },
    orange: {
      active: { stroke: isEven ? '#f97316' : '#ea580c', waveform: isEven ? '#fb923c' : '#f97316', label: isEven ? '#fdba74' : '#fb923c' },
      inactive: { stroke: isEven ? '#475569' : '#3f3f46', waveform: isEven ? '#64748b' : '#52525b', label: isEven ? '#64748b' : '#52525b' },
    },
    cyan: {
      active: { stroke: isEven ? '#06b6d4' : '#0891b2', waveform: isEven ? '#22d3ee' : '#06b6d4', label: isEven ? '#67e8f9' : '#22d3ee' },
      inactive: { stroke: isEven ? '#475569' : '#3f3f46', waveform: isEven ? '#64748b' : '#52525b', label: isEven ? '#64748b' : '#52525b' },
    },
    pink: {
      active: { stroke: isEven ? '#ec4899' : '#db2777', waveform: isEven ? '#f472b6' : '#ec4899', label: isEven ? '#f9a8d4' : '#f472b6' },
      inactive: { stroke: isEven ? '#475569' : '#3f3f46', waveform: isEven ? '#64748b' : '#52525b', label: isEven ? '#64748b' : '#52525b' },
    },
  };

  const colors = themeColors[colorTheme];
  const strokeColor = isActive ? colors.active.stroke : colors.inactive.stroke;
  const waveformColor = isActive ? colors.active.waveform : colors.inactive.waveform;
  const labelColor = isActive ? colors.active.label : colors.inactive.label;

  const fillGradientId = isActive
    ? (isEven ? `activeGradientEven_${colorTheme}` : `activeGradientOdd_${colorTheme}`)
    : (isEven ? 'inactiveGradientEven' : 'inactiveGradientOdd');

  return (
    <g
      className="transition-all"
      style={{ touchAction: 'none' }}
    >
      {/* Slice background */}
      <path
        d={slicePath}
        fill={`url(#${fillGradientId})`}
        stroke={strokeColor}
        strokeWidth={isActive ? '2' : '1'}
        className="transition-all pointer-events-none"
        opacity={isSwiping ? 0.7 : 1}
      />

      {/* Pattern 1: Original playing indicator (Cyan - subtle) */}
      {isOriginalPlaying && (
        <path
          d={slicePath}
          fill="#06b6d4"
          opacity="0.25"
          className="animate-pulse pointer-events-none"
          style={{ animationDuration: '1s' }}
        />
      )}

      {/* Pattern 2: Triggered playing indicator (Pink/Magenta - prominent) */}
      {isTriggeredPlaying && (
        <>
          <path
            d={slicePath}
            fill="#ec4899"
            opacity="0.4"
            className="animate-pulse pointer-events-none"
            style={{ animationDuration: '0.6s' }}
          />
          {/* Extra glow for pattern 2 */}
          <path
            d={slicePath}
            fill="none"
            stroke="#ec4899"
            strokeWidth="3"
            opacity="0.8"
            className="pointer-events-none"
          />
        </>
      )}

      {/* Waveform */}
      <path
        d={createWaveformPath()}
        fill="none"
        stroke={waveformColor}
        strokeWidth="1.5"
        strokeLinecap="round"
        strokeLinejoin="round"
        opacity="0.8"
        className="pointer-events-none"
      />

      {/* Slice label */}
      {/* Slice number hidden as per user request
      <text
        x={labelX}
        y={labelY}
        fill={labelColor}
        fontSize="10"
        fontWeight="600"
        textAnchor="middle"
        dominantBaseline="middle"
        className="pointer-events-none select-none"
      >
        {index + 1}
      </text>
      */}

      {/* Invisible interaction layer - covers the entire slice area */}
      <path
        d={slicePath}
        fill="transparent"
        stroke="none"
        className="cursor-pointer"
        style={{ pointerEvents: 'all' }}
        onClick={onClick}
        onPointerDown={onPointerDown}
        onPointerEnter={onPointerEnter}
      />

      {/* Gradients */}
      <defs>
        {/* Active gradients - Theme-specific */}
        <radialGradient id={`activeGradientEven_${colorTheme}`}>
          <stop offset="0%" stopColor={colors.active.stroke} stopOpacity="0.3" />
          <stop offset="100%" stopColor={colors.active.stroke} stopOpacity="0.5" />
        </radialGradient>
        <radialGradient id={`activeGradientOdd_${colorTheme}`}>
          <stop offset="0%" stopColor={colors.active.stroke} stopOpacity="0.25" />
          <stop offset="100%" stopColor={colors.active.stroke} stopOpacity="0.45" />
        </radialGradient>

        {/* Inactive gradients */}
        <radialGradient id="inactiveGradientEven">
          <stop offset="0%" stopColor="#334155" stopOpacity="0.3" />
          <stop offset="100%" stopColor="#1e293b" stopOpacity="0.5" />
        </radialGradient>
        <radialGradient id="inactiveGradientOdd">
          <stop offset="0%" stopColor="#27272a" stopOpacity="0.3" />
          <stop offset="100%" stopColor="#18181b" stopOpacity="0.5" />
        </radialGradient>
      </defs>
    </g>
  );
}