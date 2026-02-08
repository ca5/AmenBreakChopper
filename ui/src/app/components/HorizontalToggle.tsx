export type ThemeColor = 'green' | 'blue' | 'purple' | 'red' | 'orange' | 'cyan' | 'pink';

interface HorizontalToggleProps {
  isOn: boolean;
  onToggle: () => void;
  ccNumber: number;
  label: string;
  colorTheme: ThemeColor;
}

export function HorizontalToggle({ isOn, onToggle, ccNumber, label, colorTheme }: HorizontalToggleProps) {
  // Theme colors
  const themeColors = {
    green: {
      buttonOff: 'bg-green-900/30',
      buttonOn: 'bg-green-600',
      text: 'text-green-300',
      textSecondary: 'text-green-300/70',
    },
    blue: {
      buttonOff: 'bg-blue-900/30',
      buttonOn: 'bg-blue-600',
      text: 'text-blue-300',
      textSecondary: 'text-blue-300/70',
    },
    purple: {
      buttonOff: 'bg-purple-900/30',
      buttonOn: 'bg-purple-600',
      text: 'text-purple-300',
      textSecondary: 'text-purple-300/70',
    },
    red: {
      buttonOff: 'bg-red-900/30',
      buttonOn: 'bg-red-600',
      text: 'text-red-300',
      textSecondary: 'text-red-300/70',
    },
    orange: {
      buttonOff: 'bg-orange-900/30',
      buttonOn: 'bg-orange-600',
      text: 'text-orange-300',
      textSecondary: 'text-orange-300/70',
    },
    cyan: {
      buttonOff: 'bg-cyan-900/30',
      buttonOn: 'bg-cyan-600',
      text: 'text-cyan-300',
      textSecondary: 'text-cyan-300/70',
    },
    pink: {
      buttonOff: 'bg-pink-900/30',
      buttonOn: 'bg-pink-600',
      text: 'text-pink-300',
      textSecondary: 'text-pink-300/70',
    },
  };

  const theme = themeColors[colorTheme] || themeColors.green;

  return (
    <div className="flex flex-col gap-2">
      {/* Label and CC Number */}
      <div className="flex items-center justify-between px-2">
        <span className={`text-xs font-bold ${theme.textSecondary}`}>{label}</span>
        <span className={`text-[10px] ${theme.textSecondary}`}>CC {ccNumber}</span>
      </div>
      
      {/* Toggle Button */}
      <button
        onClick={onToggle}
        className={`w-full py-3 rounded-lg text-sm font-bold transition-all shadow-md ${
          isOn
            ? `${theme.buttonOn} text-white`
            : `${theme.buttonOff} ${theme.text}`
        }`}
      >
        {isOn ? 'ON (127)' : 'OFF (0)'}
      </button>
    </div>
  );
}
