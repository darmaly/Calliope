const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
const BLACK_KEY_INDICES = new Set([1, 3, 6, 8, 10])

export function pitchToNoteName(pitch: number): string {
  const octave = Math.floor(pitch / 12) - 2
  const note = NOTE_NAMES[pitch % 12]
  return `${note}${octave}`
}

export function isBlackKey(pitch: number): boolean {
  return BLACK_KEY_INDICES.has(pitch % 12)
}

export function pitchToY(pitch: number, noteRowHeight: number): number {
  return (127 - pitch) * noteRowHeight
}

export function yToPitch(y: number, noteRowHeight: number, scrollY: number): number {
  return 127 - Math.floor((y + scrollY) / noteRowHeight)
}

export function velocityToAlpha(velocity: number): number {
  return 0.2 + (velocity / 127) * 0.8
}
