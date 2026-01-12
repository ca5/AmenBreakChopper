import { useState, useRef, useEffect, useCallback } from 'react';
import { motion, useMotionValue, useSpring, useTransform } from 'motion/react';
import { CircularSliceBlock } from './CircularSliceBlock';
import { useJuceBridge } from '../../hooks/useJuceBridge';

interface WaveformDisplayProps {
  activeSlices: Set<number>;
  isPlaying: boolean;
  colorTheme: 'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink';
  originalPlayhead: number;
  triggeredPlayhead: number | null;
}

export function WaveformDisplay({ activeSlices, isPlaying, colorTheme, originalPlayhead, triggeredPlayhead }: WaveformDisplayProps) {
  const { performSequenceReset, performSoftReset, sendParameterValue, triggerNote, addEventListener } = useJuceBridge();

  const [waveforms, setWaveforms] = useState<number[][]>(() => {
      // Initial mock state to avoid empty screen before first update
      return Array.from({ length: 16 }, (_, i) => {
        return Array.from({ length: 32 }, (_, j) => {
          const freq = Math.sin((i + 1) * 0.5) * 2 + 3;
          return Math.sin(j * freq * 0.2) * 0.1; // Quieter mock
        });
      });
  });

  useEffect(() => {
    const handleWaveformUpdate = (event: any) => {
      // console.log("[Waveform] Update Received", event);
      // Expecting { data: [ 512 floats ] }
      if (event && event.data && Array.isArray(event.data) && event.data.length === 512) {
        // Chunk into 16 slices of 32
        const raw = event.data as number[];
        const newWaveforms: number[][] = [];
        for (let i = 0; i < 16; i++) {
            newWaveforms.push(raw.slice(i * 32, (i + 1) * 32));
        }
        setWaveforms(newWaveforms);
      } else {
          console.warn("[Waveform] Invalid data format received", event);
      }
    };

    const removeListener = addEventListener('waveform', handleWaveformUpdate);
    return () => removeListener();
  }, [addEventListener]);

  const [centerButtonSwipe, setCenterButtonSwipe] = useState(false);
  const [swipeStartPos, setSwipeStartPos] = useState<{ x: number; y: number } | null>(null);
  const containerRef = useRef<HTMLDivElement>(null);

  // Motion values for elastic button effect
  const buttonOffsetX = useMotionValue(0);
  const buttonOffsetY = useMotionValue(0);
  const buttonScale = useMotionValue(1);

  // Spring animation
  const [springConfig, setSpringConfig] = useState({ stiffness: 400, damping: 25 });
  const springOffsetX = useSpring(buttonOffsetX, springConfig);
  const springOffsetY = useSpring(buttonOffsetY, springConfig);
  const springScale = useSpring(buttonScale, springConfig);

  const svgTransform = useTransform(
    [springOffsetX, springOffsetY, springScale],
    ([x, y, s]) => `translate(${x}, ${y}) scale(${s})`
  );



  const handleSliceClick = (index: number) => {
    console.log(`[Waveform] Slice clicked: ${index}`);

    // Trigger slice in JUCE
    if (triggerNote) {
      console.log(`[Waveform] Triggering Slice Note: ${index}`);
      triggerNote(index);
    } else {
      console.warn("[Waveform] triggerNote not available");
    }
  };

  const handleSyncPlayheads = () => {
    console.log("[Waveform] Center Button Clicked: Syncing Playheads");
    if (performSequenceReset) {
      performSequenceReset();
    } else {
      console.warn("[Waveform] performSequenceReset not available");
    }
  };

  const handleSoftReset = () => {
    console.log("[Waveform] Center Button Swiped: Soft Reset");
    if (performSoftReset) {
      performSoftReset();
    } else {
      console.warn("[Waveform] performSoftReset not available");
    }
  };

  // Issue #24: Slide Interaction State
  const [isSlideActive, setIsSlideActive] = useState(false);
  const lastTriggeredSlice = useRef<number | null>(null);

  // Helper to get slice index from check point
  const getSliceFromPoint = (clientX: number, clientY: number) => {
    if (!containerRef.current) return -1;
    
    // Get SVG bounds
    const svg = containerRef.current.querySelector('svg');
    if (!svg) return -1;
    
    const rect = svg.getBoundingClientRect();
    const centerX = rect.left + rect.width / 2;
    const centerY = rect.top + rect.height / 2;
    
    // Relative coordinates
    const x = clientX - centerX;
    const y = clientY - centerY;
    
    // Calculate radius to ensure we are within the ring
    // SVG is 400x400, Center 200,200. Inner R ~90, Outer R ~190.
    // We need to map screen coordinates to SVG units roughly, or just check ratio.
    const radius = Math.sqrt(x*x + y*y);
    const maxRadius = Math.min(rect.width, rect.height) / 2;
    const normRadius = radius / maxRadius; // 0 to 1
    
    // Inner radius is roughly 90/200 = 0.45, Outer is 190/200 = 0.95
    if (normRadius < 0.4 || normRadius > 0.98) return -1;

    // Calculate Angle
    // atan2 returns -PI to PI. 0 is Right (3 o'clock).
    // Our slices start at -11.25 deg (top-ish?) logic is in CircularSliceBlock
    // slicesPerCircle = 16. anglePerSlice = 22.5.
    // slice 0 is usually at top or right? 
    // In CircularSliceBlock: startAngle = index * 22.5 + (-11.25)
    // 0 deg is typically 3 o'clock in SVG.
    // If index 0 startAngle is -11.25, that means it centers at 0 deg (Right).
    // So Slice 0 is East. Slice 4 is South. Slice 8 is West. Slice 12 is North.
    
    let angleDeg = Math.atan2(y, x) * (180 / Math.PI);
    // Normalize to 0-360
    if (angleDeg < 0) angleDeg += 360;
    
    // Adjust for the offset (-11.25 start) => center of slice 0 is at 0 deg.
    // The range for slice 0 is [-11.25, 11.25]. 
    // So we add 11.25 to shift the boundary to 0.
    // Issue Fix: Add 90 degrees because visual 0 is at Top (-90), but math 0 is Right (0).
    // StartAngle -11.25 corresponds to the wedge varying around -90 deg.
    // So we need to rotate our input by +90 to align with the visual rotation.
    const shiftedAngle = (angleDeg + 90 + 11.25) % 360;
    
    const index = Math.floor(shiftedAngle / 22.5);
    return index >= 0 && index < 16 ? index : -1;
  };

  const handleSlicePointerDown = (e: React.PointerEvent) => {
    // Only engage if not touching the center button
    // Center button radius is < 90 SVG units (approx 0.45 norm radius)
    // The helper logic already filters < 0.4
    
    const index = getSliceFromPoint(e.clientX, e.clientY);
    if (index >= 0) {
       setIsSlideActive(true);
       lastTriggeredSlice.current = index;
       handleSliceClick(index);
       (e.target as Element).setPointerCapture(e.pointerId);
       e.preventDefault();
    }
  };

  const handleSlicePointerMove = (e: React.PointerEvent) => {
    if (!isSlideActive) return;
    
    const index = getSliceFromPoint(e.clientX, e.clientY);
    
    // Reverted to strict change detection to avoid message flooding
    if (index >= 0 && index !== lastTriggeredSlice.current) {
        lastTriggeredSlice.current = index;
        handleSliceClick(index);
    }
  };

  const handleSlicePointerUp = (e: React.PointerEvent) => {
    if (isSlideActive) {
        setIsSlideActive(false);
        lastTriggeredSlice.current = null;
        (e.target as Element).releasePointerCapture(e.pointerId);
    }
  };

  // Issue Fix: Sync re-triggering to playhead updates (next beat)
  // allowing hold-looping without flooding IPC
  useEffect(() => {
     if (isSlideActive && lastTriggeredSlice.current !== null) {
         handleSliceClick(lastTriggeredSlice.current);
     }
  }, [originalPlayhead, isSlideActive]);

  // Center button interaction handlers
  const handleCenterButtonPointerDown = (e: React.PointerEvent) => {
    // console.log("[Waveform] Center Button Down"); // Verbose
    setCenterButtonSwipe(true);
    setSwipeStartPos({ x: e.clientX, y: e.clientY });
    // Set extremely high stiffness for instant response during drag
    setSpringConfig({ stiffness: 100000, damping: 100000 });
    // Capture pointer to continue receiving events even outside the button
    (e.target as Element).setPointerCapture(e.pointerId);
    e.stopPropagation();
  };

  const handleCenterButtonPointerMove = (e: React.PointerEvent) => {
    if (centerButtonSwipe && swipeStartPos) {
      const dx = e.clientX - swipeStartPos.x;
      const dy = e.clientY - swipeStartPos.y;
      const distance = Math.sqrt(dx * dx + dy * dy);

      // Apply offset to button position (scaled down for SVG coordinates)
      const scale = 0.3; // Scale factor to convert screen pixels to SVG units
      buttonOffsetX.set(dx * scale);
      buttonOffsetY.set(dy * scale);

      // Maintain circular shape - uniform scale based on distance
      const scaleFactor = 1 + Math.min(distance / 150, 0.5); // Grow up to 1.5x size
      buttonScale.set(scaleFactor);
    }
  };

  const handleCenterButtonPointerUp = (e: React.PointerEvent) => {
    if (centerButtonSwipe && swipeStartPos) {
      const dx = e.clientX - swipeStartPos.x;
      const dy = e.clientY - swipeStartPos.y;
      const distance = Math.sqrt(dx * dx + dy * dy);

      // Restore spring animation for elastic bounce back
      setSpringConfig({ stiffness: 500, damping: 28 });

      // Reset button position and scale with spring animation
      buttonOffsetX.set(0);
      buttonOffsetY.set(0);
      buttonScale.set(1);

      // If dragged more than 70px, it's a swipe - trigger soft reset
      if (distance > 90) {
        handleSoftReset();
      } else {
        // Otherwise it's a click - sync playheads
        handleSyncPlayheads();
      }
    }
    setCenterButtonSwipe(false);
    setSwipeStartPos(null);
    // Release pointer capture
    (e.target as Element).releasePointerCapture(e.pointerId);
    e.stopPropagation();
  };

  const slicesPerCircle = 16;
  const anglePerSlice = 360 / slicesPerCircle;
  const offsetAngle = -anglePerSlice / 2; // Shift by half a slice counter-clockwise

  // Theme colors
  const themeColors = {
    green: { panel: 'bg-green-950/30', border: 'border-green-900/50', text: 'text-green-300', centerStroke: '#22c55e', centerFill: '#166534,#14532d' },
    blue: { panel: 'bg-blue-950/30', border: 'border-blue-900/50', text: 'text-blue-300', centerStroke: '#3b82f6', centerFill: '#1e40af,#1e3a8a' },
    purple: { panel: 'bg-purple-950/30', border: 'border-purple-900/50', text: 'text-purple-300', centerStroke: '#a855f7', centerFill: '#6b21a8,#581c87' },
    red: { panel: 'bg-red-950/30', border: 'border-red-900/50', text: 'text-red-300', centerStroke: '#ef4444', centerFill: '#991b1b,#7f1d1d' },
    orange: { panel: 'bg-orange-950/30', border: 'border-orange-900/50', text: 'text-orange-300', centerStroke: '#f97316', centerFill: '#9a3412,#7c2d12' },
    cyan: { panel: 'bg-cyan-950/30', border: 'border-cyan-900/50', text: 'text-cyan-300', centerStroke: '#06b6d4', centerFill: '#155e75,#164e63' },
    pink: { panel: 'bg-pink-950/30', border: 'border-pink-900/50', text: 'text-pink-300', centerStroke: '#ec4899', centerFill: '#9d174d,#831843' },
  };

  const theme = themeColors[colorTheme];
  const [centerFill1, centerFill2] = theme.centerFill.split(',');

  return (
    <div className={`flex-1 ${theme.panel} rounded-2xl p-4 border ${theme.border} backdrop-blur-sm`}>
      <div className="h-full flex flex-col gap-3">
        {/* Title */}
        <div className="flex items-center justify-between">
          <h2 className={`${theme.text} font-medium`}></h2>
          <div className="text-xs text-green-400/60 font-mono flex gap-3">
            {isPlaying && (
              <>
                <span className="text-cyan-400">
                  orig: {originalPlayhead + 1}/16
                </span>
                {triggeredPlayhead !== null && (
                  <span className="text-pink-400">
                    trig: {triggeredPlayhead + 1}/16
                  </span>
                )}
              </>
            )}
          </div>
        </div>

        {/* Circular Waveform Grid */}
        <div
          ref={containerRef}
          className="flex-1 flex items-center justify-center select-none"
          style={{ touchAction: 'none' }}
        >
          <div className="relative w-full h-full max-w-2xl max-h-2xl aspect-square">
            <svg
              viewBox="0 0 400 400"
              className="w-full h-full outline-none"
              onPointerDown={handleSlicePointerDown}
              onPointerMove={handleSlicePointerMove}
              onPointerUp={handleSlicePointerUp}
              onPointerLeave={handleSlicePointerUp}
            >
              {/* Center circle decoration */}
              <circle
                cx="200"
                cy="200"
                r="80"
                fill="none"
                stroke="#14532d"
                strokeWidth="1"
                opacity="0.3"
              />

              {/* Center sync button */}
              <motion.g
                onPointerDown={handleCenterButtonPointerDown}
                onPointerMove={handleCenterButtonPointerMove}
                onPointerUp={handleCenterButtonPointerUp}
                className="cursor-pointer transition-all hover:opacity-80"
                style={{
                  transform: svgTransform,
                  transformOrigin: 'center',
                }}
              >
                {/* Button background - Issue #25: Plain circle, no icons */}
                <motion.circle
                  cx="200"
                  cy="200"
                  r="60"
                  fill={centerButtonSwipe ? theme.centerStroke : "url(#centerButtonGradient)"} 
                  stroke={theme.centerStroke}
                  strokeWidth="2"
                  className="transition-all"
                  style={{
                    scaleX: springScale,
                    scaleY: springScale,
                    transformOrigin: '200px 200px',
                  }}
                />
              </motion.g>

              {/* Render each slice as a circular segment */}
              {Array.from({ length: slicesPerCircle }, (_, index) => {
                const startAngle = index * anglePerSlice + offsetAngle;
                const isOriginalPlaying = isPlaying && originalPlayhead === index && activeSlices.has(index);
                const isTriggeredPlaying = isPlaying && triggeredPlayhead === index && activeSlices.has(index);

                return (
                  <CircularSliceBlock
                    key={index}
                    index={index}
                    waveform={waveforms[index]}
                    startAngle={startAngle}
                    angleSpan={anglePerSlice}
                    innerRadius={90}
                    outerRadius={190}
                    centerX={200}
                    centerY={200}
                    isActive={activeSlices.has(index)}
                    isOriginalPlaying={isOriginalPlaying}
                    isTriggeredPlaying={isTriggeredPlaying}
                    isSwiping={false} // swipedIndices.has(index) - disabled
                    colorTheme={colorTheme}
                    onClick={() => handleSliceClick(index)}
                    onPointerDown={() => { }} // handlePointerDown(index) - disabled
                    onPointerEnter={() => { }} // handlePointerEnter(index) - disabled
                  />
                );
              })}

              {/* Gradients */}
              <defs>
                <radialGradient id="centerButtonGradient">
                  <stop offset="0%" stopColor={centerFill1} stopOpacity="0.8" />
                  <stop offset="100%" stopColor={centerFill2} stopOpacity="0.9" />
                </radialGradient>
              </defs>
            </svg>
          </div>
        </div>

        {/* Instructions */}
        <div className="text-xs text-green-400/50 text-center">
          Tap slice to trigger • Click center to sync • Drag center to reset both to position 1
        </div>
      </div>
    </div>
  );
}