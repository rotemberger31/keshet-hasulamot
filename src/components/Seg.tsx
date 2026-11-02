import ui from './ui.module.css';

interface SegProps<T> {
  label: string;
  value: T;
  options: { value: T; label: string }[];
  onChange: (value: T) => void;
}

export function Seg<T extends string | number>({ label, value, options, onChange }: SegProps<T>) {
  return (
    <div className={ui.seg} role="group" aria-label={label}>
      {options.map((option) => (
        <button key={option.value} type="button" aria-pressed={option.value === value} onClick={() => onChange(option.value)}>
          {option.label}
        </button>
      ))}
    </div>
  );
}
