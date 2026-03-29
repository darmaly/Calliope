import { useEffect, useRef } from 'react'
import { createPortal } from 'react-dom'

interface ParamDef {
  name: string
  label: string
  min: number
  max: number
  step: number
  default: number
}

const EFFECT_PARAMS: Record<string, ParamDef[]> = {
  eq: [
    { name: 'lowGain', label: 'Low', min: -12, max: 12, step: 0.5, default: 0 },
    { name: 'lowMidGain', label: 'Low Mid', min: -12, max: 12, step: 0.5, default: 0 },
    { name: 'hiMidGain', label: 'Hi Mid', min: -12, max: 12, step: 0.5, default: 0 },
    { name: 'highGain', label: 'High', min: -12, max: 12, step: 0.5, default: 0 },
  ],
  compressor: [
    { name: 'threshold', label: 'Threshold', min: -60, max: 0, step: 1, default: -24 },
    { name: 'ratio', label: 'Ratio', min: 1, max: 20, step: 0.5, default: 4 },
    { name: 'attack', label: 'Attack (ms)', min: 0.1, max: 100, step: 0.1, default: 10 },
    { name: 'release', label: 'Release (ms)', min: 10, max: 1000, step: 10, default: 100 },
    { name: 'makeupGain', label: 'Makeup', min: 0, max: 24, step: 0.5, default: 0 },
  ],
  reverb: [
    { name: 'roomSize', label: 'Room Size', min: 0, max: 1, step: 0.01, default: 0.5 },
    { name: 'damping', label: 'Damping', min: 0, max: 1, step: 0.01, default: 0.5 },
    { name: 'wetDry', label: 'Wet/Dry', min: 0, max: 1, step: 0.01, default: 0.3 },
    { name: 'preDelay', label: 'Pre-Delay (ms)', min: 0, max: 100, step: 1, default: 20 },
  ],
  delay: [
    { name: 'delayTime', label: 'Time (ms)', min: 10, max: 2000, step: 10, default: 250 },
    { name: 'feedback', label: 'Feedback', min: 0, max: 0.95, step: 0.01, default: 0.4 },
    { name: 'wetDry', label: 'Wet/Dry', min: 0, max: 1, step: 0.01, default: 0.3 },
  ],
  limiter: [
    { name: 'threshold', label: 'Threshold', min: -24, max: 0, step: 0.5, default: -1 },
    { name: 'release', label: 'Release (ms)', min: 10, max: 500, step: 10, default: 100 },
  ],
}

interface EffectParamPopoverProps {
  trackId: string
  slotIndex: number
  effectType: string
  position: { x: number; y: number }
  onClose: () => void
}

export function EffectParamPopover({ trackId, slotIndex, effectType, position, onClose }: EffectParamPopoverProps) {
  const popoverRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    const handleClickOutside = (e: PointerEvent) => {
      if (popoverRef.current && !popoverRef.current.contains(e.target as Node)) {
        onClose()
      }
    }
    requestAnimationFrame(() => {
      document.addEventListener('pointerdown', handleClickOutside)
    })
    return () => {
      document.removeEventListener('pointerdown', handleClickOutside)
    }
  }, [onClose])

  const params = EFFECT_PARAMS[effectType] ?? []

  const handleChange = (paramName: string, value: number) => {
    window.calliope
      .dispatchCommand({
        command: 'parameter.set',
        params: {
          id: `effects.${trackId}.${slotIndex}.${paramName}`,
          value,
        },
      })
      .catch(() => {
        /* engine not ready */
      })
  }

  return createPortal(
    <div
      ref={popoverRef}
      style={{
        position: 'fixed',
        left: position.x,
        top: position.y,
        width: 240,
        backgroundColor: '#252542',
        border: '1px solid #3a3a5a',
        borderRadius: 6,
        boxShadow: '0 8px 24px rgba(0,0,0,0.5)',
        padding: 12,
        zIndex: 50,
      }}
    >
      <div style={{ fontSize: 12, color: '#eeeeee', fontWeight: 500, marginBottom: 8 }}>
        {effectType.charAt(0).toUpperCase() + effectType.slice(1)} Parameters
      </div>
      {params.map((param) => (
        <div key={param.name} style={{ marginBottom: 6 }}>
          <div
            style={{
              display: 'flex',
              justifyContent: 'space-between',
              alignItems: 'center',
              marginBottom: 2,
            }}
          >
            <span style={{ fontSize: 10, color: '#999999' }}>{param.label}</span>
            <span style={{ fontSize: 10, color: '#999999' }} id={`val-${param.name}`}>
              {param.default}
            </span>
          </div>
          <input
            type="range"
            min={param.min}
            max={param.max}
            step={param.step}
            defaultValue={param.default}
            style={{
              width: '100%',
              accentColor: '#6c63ff',
              cursor: 'pointer',
            }}
            onChange={(e) => {
              const val = parseFloat(e.target.value)
              handleChange(param.name, val)
              const display = document.getElementById(`val-${param.name}`)
              if (display) display.textContent = String(val)
            }}
          />
        </div>
      ))}
      {params.length === 0 && (
        <div style={{ fontSize: 11, color: '#666666' }}>No parameters available</div>
      )}
    </div>,
    document.body,
  )
}
