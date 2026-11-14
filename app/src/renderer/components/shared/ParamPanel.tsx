import { useEffect, useRef } from 'react'
import { createPortal } from 'react-dom'
import type { ParamSpec } from '../../../shared/catalog'

interface ParamPanelProps {
  title: string
  specs: ParamSpec[]
  /** Current values by parameter name; a missing entry means the default. */
  values: Record<string, number>
  onChange: (name: string, value: number) => void
  position: { x: number; y: number }
  onClose: () => void
}

function formatValue(spec: ParamSpec, value: number): string {
  if (spec.kind === 'bool') return value >= 0.5 ? 'On' : 'Off'
  if (spec.choices) return spec.choices[Math.round(value) - spec.min] ?? String(value)
  const range = spec.max - spec.min
  const digits = spec.kind === 'int' ? 0 : range >= 1000 ? 0 : range >= 20 ? 1 : 2
  return `${value.toFixed(digits)}${spec.unit ? ` ${spec.unit}` : ''}`
}

/**
 * A floating panel of controls generated from parameter specs. Used for
 * instrument settings and effect settings alike, so both always show the
 * parameters the engine really has, at their real ranges and current values.
 */
export function ParamPanel({ title, specs, values, onChange, position, onClose }: ParamPanelProps) {
  const panelRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    const handlePointerDown = (e: PointerEvent) => {
      if (panelRef.current && !panelRef.current.contains(e.target as Node)) onClose()
    }
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key === 'Escape') onClose()
    }
    // Registered a frame later so the click that opened the panel does not close it.
    const frame = requestAnimationFrame(() => {
      document.addEventListener('pointerdown', handlePointerDown)
    })
    document.addEventListener('keydown', handleKeyDown)
    return () => {
      cancelAnimationFrame(frame)
      document.removeEventListener('pointerdown', handlePointerDown)
      document.removeEventListener('keydown', handleKeyDown)
    }
  }, [onClose])

  // Keep the panel on screen.
  const width = 264
  const maxHeight = Math.min(460, window.innerHeight - 24)
  const left = Math.max(8, Math.min(position.x, window.innerWidth - width - 8))
  const top = Math.max(8, Math.min(position.y, window.innerHeight - maxHeight - 8))

  return createPortal(
    <div
      ref={panelRef}
      className="fixed z-50 flex flex-col rounded-md border border-[#3a3a5a] bg-[#252542] shadow-[0_8px_24px_rgba(0,0,0,0.5)]"
      style={{ left, top, width, maxHeight }}
      onPointerDown={(e) => e.stopPropagation()}
    >
      <div className="flex items-center justify-between px-3 py-2 border-b border-[#3a3a5a]">
        <span className="text-[12px] font-medium text-[#eeeeee]">{title}</span>
        <button
          onClick={onClose}
          className="text-[11px] text-[#999999] hover:text-[#eeeeee]"
          aria-label="Close"
        >
          Close
        </button>
      </div>
      <div className="overflow-y-auto px-3 py-2">
        {specs.map((spec) => {
          const value = values[spec.name] ?? spec.default
          return (
            <label key={spec.name} className="block mb-2" title={spec.description}>
              <div className="flex justify-between items-center mb-0.5">
                <span className="text-[10px] text-[#aaaacc]">{spec.label}</span>
                <span className="text-[10px] text-[#999999] tabular-nums">{formatValue(spec, value)}</span>
              </div>
              {spec.kind === 'bool' ? (
                <input
                  type="checkbox"
                  checked={value >= 0.5}
                  onChange={(e) => onChange(spec.name, e.target.checked ? 1 : 0)}
                  className="accent-[#6c63ff] cursor-pointer"
                />
              ) : spec.choices ? (
                <select
                  value={Math.round(value)}
                  onChange={(e) => onChange(spec.name, Number(e.target.value))}
                  className="w-full px-1 py-0.5 rounded bg-[#1a1a2e] border border-[#3a3a5a] text-[11px] text-[#eeeeee] outline-none"
                >
                  {spec.choices.map((choice, index) => (
                    <option key={choice} value={spec.min + index}>
                      {choice}
                    </option>
                  ))}
                </select>
              ) : (
                <input
                  type="range"
                  min={spec.min}
                  max={spec.max}
                  step={spec.step}
                  value={value}
                  onChange={(e) => onChange(spec.name, parseFloat(e.target.value))}
                  className="w-full accent-[#6c63ff] cursor-pointer"
                />
              )}
            </label>
          )
        })}
        {specs.length === 0 && <div className="text-[11px] text-[#666666]">No settings.</div>}
      </div>
    </div>,
    document.body,
  )
}
