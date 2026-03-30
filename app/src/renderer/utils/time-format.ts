/**
 * Convert a beat position to milliseconds given BPM.
 * Formula: (beat / bpm) * 60000
 */
export function beatToTimeMs(beat: number, bpm: number): number {
  return (beat / bpm) * 60000
}

/**
 * Format milliseconds as "MM:SS.ms" where minutes can exceed 59.
 * Examples: "00:00.000", "01:01.500", "62:03.456"
 */
export function formatTimeMs(ms: number): string {
  const totalSeconds = Math.floor(ms / 1000)
  const minutes = Math.floor(totalSeconds / 60)
  const seconds = totalSeconds % 60
  const millis = Math.round(ms % 1000)

  const mm = String(minutes).padStart(2, '0')
  const ss = String(seconds).padStart(2, '0')
  const mmm = String(millis).padStart(3, '0')

  return `${mm}:${ss}.${mmm}`
}

/**
 * Convert a beat position to 1-indexed bar and beat-in-bar.
 * Bar 1, beat 1 corresponds to beat position 0.
 */
export function beatToBarsBeats(
  beat: number,
  beatsPerBar: number,
): { bar: number; beatInBar: number } {
  const bar = Math.floor(beat / beatsPerBar) + 1
  const beatInBar = Math.floor(beat % beatsPerBar) + 1
  return { bar, beatInBar }
}

/**
 * Format bar and beat as "NNN:NN" zero-padded.
 * Examples: "001:01", "012:03"
 */
export function formatBarsBeats(bar: number, beat: number): string {
  return `${String(bar).padStart(3, '0')}:${String(beat).padStart(2, '0')}`
}
