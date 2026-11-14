// What the engine can do, described once for everyone who needs to know:
// the UI draws controls from it, the engine-sync layer uses its defaults, and
// the AI agent is told about it so it only asks for things that exist.
//
// Names and ranges mirror the parameter registry in engine/src/engine.cpp.
// If a parameter is added there, add it here.

export type InstrumentType = 'polysynth' | 'basssynth' | 'drumMachine'
export type EffectType = 'eq' | 'compressor' | 'reverb' | 'delay' | 'limiter'

export interface ParamSpec {
  name: string
  label: string
  min: number
  max: number
  default: number
  /** Slider step. Integer parameters use 1. */
  step: number
  kind: 'float' | 'int' | 'bool'
  unit?: string
  /** For enumerated integer parameters: the meaning of each value. */
  choices?: string[]
  description?: string
}

export interface InstrumentSpec {
  type: InstrumentType
  label: string
  description: string
  params: ParamSpec[]
}

export interface EffectSpec {
  type: EffectType
  label: string
  description: string
  params: ParamSpec[]
}

const WAVEFORMS = ['saw', 'square', 'sine', 'triangle']

const f = (
  name: string,
  label: string,
  min: number,
  max: number,
  def: number,
  extra: Partial<ParamSpec> = {},
): ParamSpec => ({
  name,
  label,
  min,
  max,
  default: def,
  step: (max - min) / 200,
  kind: 'float',
  ...extra,
})

const i = (
  name: string,
  label: string,
  min: number,
  max: number,
  def: number,
  extra: Partial<ParamSpec> = {},
): ParamSpec => ({ name, label, min, max, default: def, step: 1, kind: 'int', ...extra })

const b = (name: string, label: string, def: boolean, extra: Partial<ParamSpec> = {}): ParamSpec => ({
  name,
  label,
  min: 0,
  max: 1,
  default: def ? 1 : 0,
  step: 1,
  kind: 'bool',
  ...extra,
})

const envelope = (prefix: 'amp' | 'filter', label: string, a: number, d: number, s: number, r: number) => [
  f(`${prefix}Attack`, `${label} attack`, 0.001, 10, a, { unit: 's' }),
  f(`${prefix}Decay`, `${label} decay`, 0.001, 10, d, { unit: 's' }),
  f(`${prefix}Sustain`, `${label} sustain`, 0, 1, s),
  f(`${prefix}Release`, `${label} release`, 0.001, 10, r, { unit: 's' }),
]

export const INSTRUMENTS: Record<InstrumentType, InstrumentSpec> = {
  polysynth: {
    type: 'polysynth',
    label: 'Poly Synth',
    description:
      '16-voice subtractive synth: two oscillators into a resonant low-pass filter, with amp and filter envelopes and an LFO. Use for chords, pads, leads, plucks and arpeggios.',
    params: [
      i('osc1Waveform', 'Osc 1 wave', 0, 3, 0, { choices: WAVEFORMS }),
      i('osc2Waveform', 'Osc 2 wave', 0, 3, 2, { choices: WAVEFORMS }),
      f('oscMix', 'Osc mix', 0, 1, 0.5, { description: '0 = only osc 1, 1 = only osc 2' }),
      f('osc2Detune', 'Osc 2 detune', -100, 100, 0, { unit: 'cents' }),
      f('filterCutoff', 'Filter cutoff', 20, 20000, 5000, { unit: 'Hz' }),
      f('filterResonance', 'Filter resonance', 0, 1, 0.1),
      f('filterEnvAmount', 'Filter env amount', 0, 1, 0.3),
      ...envelope('amp', 'Amp', 0.01, 0.1, 0.8, 0.3),
      ...envelope('filter', 'Filter', 0.01, 0.2, 0.5, 0.3),
      f('lfoRate', 'LFO rate', 0.1, 20, 1, { unit: 'Hz' }),
      f('lfoDepth', 'LFO depth', 0, 1, 0),
      i('lfoTarget', 'LFO target', 0, 2, 1, { choices: ['pitch', 'filter', 'amp'] }),
      f('masterGain', 'Level', 0, 1, 0.7),
    ],
  },
  basssynth: {
    type: 'basssynth',
    label: 'Bass Synth',
    description:
      '4-voice bass synth: one oscillator plus a sine sub-oscillator one or two octaves below, into a low-pass filter. Use for bass lines; write notes roughly between MIDI 28 and 55.',
    params: [
      i('oscWaveform', 'Osc wave', 0, 3, 0, { choices: WAVEFORMS }),
      f('subOscMix', 'Sub mix', 0, 1, 0.5, { description: '0 = no sub, 1 = only sub' }),
      i('subOscOctave', 'Sub octave', -2, -1, -1),
      f('filterCutoff', 'Filter cutoff', 20, 5000, 800, { unit: 'Hz' }),
      f('filterResonance', 'Filter resonance', 0, 1, 0.2),
      f('filterEnvAmount', 'Filter env amount', 0, 1, 0.4),
      ...envelope('amp', 'Amp', 0.005, 0.15, 0.9, 0.2),
      ...envelope('filter', 'Filter', 0.005, 0.3, 0.3, 0.2),
      f('masterGain', 'Level', 0, 1, 0.8),
    ],
  },
  drumMachine: {
    type: 'drumMachine',
    label: 'Drum Machine',
    description:
      '16-pad drum machine with a built-in synthesised kit. Each pad is one MIDI note (see DRUM_PADS). Hits ring out on their own, so note length does not matter.',
    params: [f('kitVolume', 'Kit level', 0, 1, 0.8)],
  },
}

/** The built-in kit: General MIDI percussion layout from note 36. */
export const DRUM_PADS: Array<{ note: number; name: string }> = [
  { note: 36, name: 'Kick' },
  { note: 37, name: 'Rim' },
  { note: 38, name: 'Snare' },
  { note: 39, name: 'Clap' },
  { note: 40, name: 'Snare Tight' },
  { note: 41, name: 'Floor Tom' },
  { note: 42, name: 'Closed Hat' },
  { note: 43, name: 'Low Tom' },
  { note: 44, name: 'Pedal Hat' },
  { note: 45, name: 'Mid Tom' },
  { note: 46, name: 'Open Hat' },
  { note: 47, name: 'High-Mid Tom' },
  { note: 48, name: 'High Tom' },
  { note: 49, name: 'Crash' },
  { note: 50, name: 'Top Tom' },
  { note: 51, name: 'Ride' },
]

const eqBand = (index: number, label: string, frequency: number): ParamSpec[] => [
  f(`band${index}.frequency`, `${label} freq`, 20, 20000, frequency, { unit: 'Hz' }),
  f(`band${index}.gainDb`, `${label} gain`, -24, 24, 0, { unit: 'dB' }),
  f(`band${index}.q`, `${label} Q`, 0.1, 20, 0.71),
  b(`band${index}.enabled`, `${label} on`, true),
]

export const EFFECTS: Record<EffectType, EffectSpec> = {
  eq: {
    type: 'eq',
    label: 'Parametric EQ',
    description: 'Four bands: low shelf, two peaks, high shelf. Boost or cut by setting a band gain.',
    params: [
      ...eqBand(0, 'Low shelf', 100),
      ...eqBand(1, 'Low-mid', 500),
      ...eqBand(2, 'High-mid', 2000),
      ...eqBand(3, 'High shelf', 8000),
    ],
  },
  compressor: {
    type: 'compressor',
    label: 'Compressor',
    description: 'Evens out level. Lower threshold and higher ratio compress harder.',
    params: [
      f('threshold', 'Threshold', -60, 0, -10, { unit: 'dB' }),
      f('ratio', 'Ratio', 1, 20, 4),
      f('attack', 'Attack', 0.1, 200, 10, { unit: 'ms' }),
      f('release', 'Release', 10, 1000, 100, { unit: 'ms' }),
      f('makeupGain', 'Makeup gain', 0, 30, 0, { unit: 'dB' }),
    ],
  },
  reverb: {
    type: 'reverb',
    label: 'Reverb',
    description: 'Room reverb. Raise wetLevel and roomSize for a bigger space.',
    params: [
      f('roomSize', 'Room size', 0, 1, 0.5),
      f('damping', 'Damping', 0, 1, 0.5),
      f('wetLevel', 'Wet level', 0, 1, 0.33),
      f('dryLevel', 'Dry level', 0, 1, 0.67),
      f('preDelay', 'Pre-delay', 0, 250, 0, { unit: 'ms' }),
    ],
  },
  delay: {
    type: 'delay',
    label: 'Delay',
    description: 'Tempo-synced echo. syncNoteValue is the delay time in beats (1 = quarter note, 0.5 = eighth).',
    params: [
      f('syncNoteValue', 'Time', 0.125, 4, 1, { unit: 'beats' }),
      f('feedback', 'Feedback', 0, 0.95, 0.3),
      f('wetDry', 'Mix', 0, 1, 0.5),
      b('pingPongEnabled', 'Ping-pong', false),
    ],
  },
  limiter: {
    type: 'limiter',
    label: 'Limiter',
    description: 'Stops peaks going over the threshold. Put it last on the master.',
    params: [
      f('threshold', 'Threshold', -30, 0, -1, { unit: 'dB' }),
      f('release', 'Release', 10, 1000, 100, { unit: 'ms' }),
    ],
  },
}

export const INSTRUMENT_TYPES = Object.keys(INSTRUMENTS) as InstrumentType[]
export const EFFECT_TYPE_LIST = Object.keys(EFFECTS) as EffectType[]

export function isInstrumentType(value: unknown): value is InstrumentType {
  return typeof value === 'string' && value in INSTRUMENTS
}

export function isEffectType(value: unknown): value is EffectType {
  return typeof value === 'string' && value in EFFECTS
}

export function findParam(specs: ParamSpec[], name: string): ParamSpec | undefined {
  return specs.find((p) => p.name === name)
}

/** Clamp (and for integers and booleans, round) a value to what the parameter accepts. */
export function clampParam(spec: ParamSpec, value: number): number {
  const v = Number.isFinite(value) ? value : spec.default
  const clamped = Math.min(spec.max, Math.max(spec.min, v))
  return spec.kind === 'float' ? clamped : Math.round(clamped)
}
