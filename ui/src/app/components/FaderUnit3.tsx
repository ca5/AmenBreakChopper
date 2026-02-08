import { VerticalFader, ThemeColor } from './VerticalFader';
import { HorizontalToggle } from './HorizontalToggle';

interface FaderUnit3Props {
  fader1Value: number;
  fader2Value: number;
  fader3Value: number;
  toggleState: boolean;
  onFader1Change: (value: number) => void;
  onFader2Change: (value: number) => void;
  onFader3Change: (value: number) => void;
  onToggle: () => void;
  ccFader1: number;
  ccFader2: number;
  ccFader3: number;
  ccToggle: number;
  colorTheme: ThemeColor;
}

export function FaderUnit3({
  fader1Value,
  fader2Value,
  fader3Value,
  toggleState,
  onFader1Change,
  onFader2Change,
  onFader3Change,
  onToggle,
  ccFader1,
  ccFader2,
  ccFader3,
  ccToggle,
  colorTheme,
}: FaderUnit3Props) {
  return (
    <div className="flex flex-col gap-2">
      {/* Three Faders */}
      <div className="grid grid-cols-3 gap-2">
        <VerticalFader
          value={fader1Value}
          onChange={onFader1Change}
          ccNumber={ccFader1}
          label="Fader 1"
          colorTheme={colorTheme}
        />
        <VerticalFader
          value={fader2Value}
          onChange={onFader2Change}
          ccNumber={ccFader2}
          label="Fader 2"
          colorTheme={colorTheme}
        />
        <VerticalFader
          value={fader3Value}
          onChange={onFader3Change}
          ccNumber={ccFader3}
          label="Fader 3"
          colorTheme={colorTheme}
        />
      </div>

      {/* Toggle Button */}
      <HorizontalToggle
        isOn={toggleState}
        onToggle={onToggle}
        ccNumber={ccToggle}
        label="Toggle 1"
        colorTheme={colorTheme}
      />
    </div>
  );
}
