import { VerticalFader, ThemeColor } from './VerticalFader';
import { HorizontalToggle } from './HorizontalToggle';

interface FaderUnit2Props {
  fader1Value: number;
  fader2Value: number;
  toggleState: boolean;
  onFader1Change: (value: number) => void;
  onFader2Change: (value: number) => void;
  onToggle: () => void;
  ccFader1: number;
  ccFader2: number;
  ccToggle: number;
  colorTheme: ThemeColor;
}

export function FaderUnit2({
  fader1Value,
  fader2Value,
  toggleState,
  onFader1Change,
  onFader2Change,
  onToggle,
  ccFader1,
  ccFader2,
  ccToggle,
  colorTheme,
}: FaderUnit2Props) {
  return (
    <div className="flex flex-col gap-2">
      {/* Two Faders */}
      <div className="grid grid-cols-2 gap-2">
        <VerticalFader
          value={fader1Value}
          onChange={onFader1Change}
          ccNumber={ccFader1}
          label="Fader 4"
          colorTheme={colorTheme}
        />
        <VerticalFader
          value={fader2Value}
          onChange={onFader2Change}
          ccNumber={ccFader2}
          label="Fader 5"
          colorTheme={colorTheme}
        />
      </div>

      {/* Toggle Button */}
      <HorizontalToggle
        isOn={toggleState}
        onToggle={onToggle}
        ccNumber={ccToggle}
        label="Toggle 2"
        colorTheme={colorTheme}
      />
    </div>
  );
}
