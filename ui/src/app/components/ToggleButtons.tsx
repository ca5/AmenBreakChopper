import { useJuceBridge } from '../../hooks/useJuceBridge';

export type ThemeColor = 'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink';

interface ToggleButtonsProps {
  colorTheme: ThemeColor;
}

export function ToggleButtons({ colorTheme }: ToggleButtonsProps) {
  const { parameters, sendParameter } = useJuceBridge();

  // Theme colors
  const themeColors = {
    green: {
      panel: 'bg-green-950/30',
      border: 'border-green-900/50',
      text: 'text-green-300',
      textSecondary: 'text-green-300/80',
      buttonBg: 'bg-green-900/30',
      buttonActive: 'bg-green-600',
    },
    blue: {
      panel: 'bg-blue-950/30',
      border: 'border-blue-900/50',
      text: 'text-blue-300',
      textSecondary: 'text-blue-300/80',
      buttonBg: 'bg-blue-900/30',
      buttonActive: 'bg-blue-600',
    },
    purple: {
      panel: 'bg-purple-950/30',
      border: 'border-purple-900/50',
      text: 'text-purple-300',
      textSecondary: 'text-purple-300/80',
      buttonBg: 'bg-purple-900/30',
      buttonActive: 'bg-purple-600',
    },
    red: {
      panel: 'bg-red-950/30',
      border: 'border-red-900/50',
      text: 'text-red-300',
      textSecondary: 'text-red-300/80',
      buttonBg: 'bg-red-900/30',
      buttonActive: 'bg-red-600',
    },
    orange: {
      panel: 'bg-orange-950/30',
      border: 'border-orange-900/50',
      text: 'text-orange-300',
      textSecondary: 'text-orange-300/80',
      buttonBg: 'bg-orange-900/30',
      buttonActive: 'bg-orange-600',
    },
    cyan: {
      panel: 'bg-cyan-950/30',
      border: 'border-cyan-900/50',
      text: 'text-cyan-300',
      textSecondary: 'text-cyan-300/80',
      buttonBg: 'bg-cyan-900/30',
      buttonActive: 'bg-cyan-600',
    },
    pink: {
      panel: 'bg-pink-950/30',
      border: 'border-pink-900/50',
      text: 'text-pink-300',
      textSecondary: 'text-pink-300/80',
      buttonBg: 'bg-pink-900/30',
      buttonActive: 'bg-pink-600',
    },
  };

  const theme = themeColors[colorTheme] || themeColors.green;

  // Get current toggle states
  const toggleButton1 = (parameters['toggleButton1'] || 0) > 0.5;
  const toggleButton2 = (parameters['toggleButton2'] || 0) > 0.5;
  const toggleButton3 = (parameters['toggleButton3'] || 0) > 0.5;
  const toggleButton4 = (parameters['toggleButton4'] || 0) > 0.5;

  // Toggle button handlers
  const handleToggle = (buttonIndex: number, currentState: boolean) => {
    sendParameter(`toggleButton${buttonIndex}`, currentState ? 0 : 1);
  };

  return (
    <div className={`flex flex-col gap-2 px-4 py-2 rounded-xl border ${theme.border} ${theme.panel}`}>
      {/* Toggle Buttons */}
      <div className="flex flex-col gap-2">
        <span className={`text-xs font-bold tracking-wider ${theme.textSecondary}`}>
          CHANNEL SETTING (EXT MIDI)
        </span>
        <div className="grid grid-cols-4 gap-2">
          {/* Toggle Button 1 (CC 30) */}
          <button
            onClick={() => handleToggle(1, toggleButton1)}
            className={`h-8 rounded-lg text-sm font-bold transition-all shadow-md ${
              toggleButton1
                ? `${theme.buttonActive} text-white`
                : `${theme.buttonBg} ${theme.text}`
            }`}
          >
            1
          </button>

          {/* Toggle Button 2 (CC 31) */}
          <button
            onClick={() => handleToggle(2, toggleButton2)}
            className={`h-8 rounded-lg text-sm font-bold transition-all shadow-md ${
              toggleButton2
                ? `${theme.buttonActive} text-white`
                : `${theme.buttonBg} ${theme.text}`
            }`}
          >
            2
          </button>

          {/* Toggle Button 3 (CC 32) */}
          <button
            onClick={() => handleToggle(3, toggleButton3)}
            className={`h-8 rounded-lg text-sm font-bold transition-all shadow-md ${
              toggleButton3
                ? `${theme.buttonActive} text-white`
                : `${theme.buttonBg} ${theme.text}`
            }`}
          >
            3
          </button>

          {/* Toggle Button 4 (CC 33) */}
          <button
            onClick={() => handleToggle(4, toggleButton4)}
            className={`h-8 rounded-lg text-sm font-bold transition-all shadow-md ${
              toggleButton4
                ? `${theme.buttonActive} text-white`
                : `${theme.buttonBg} ${theme.text}`
            }`}
          >
            4
          </button>
        </div>
      </div>
    </div>
  );
}
