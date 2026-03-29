/**
 * Convert dB value to linear gain (0.0 - ~1.259 for -inf to +6dB).
 * -Infinity maps to 0.
 */
export function dbToLinear(db: number): number {
  if (db === -Infinity) return 0
  return Math.pow(10, db / 20)
}

/**
 * Convert linear gain to dB.
 * 0 maps to -Infinity.
 */
export function linearToDb(linear: number): number {
  if (linear <= 0) return -Infinity
  return 20 * Math.log10(linear)
}

/**
 * Format a linear gain value as a dB string for display.
 * e.g., 1.0 -> "0.0 dB", 0 -> "-inf", 0.5 -> "-6.0 dB"
 */
export function formatDb(linear: number): string {
  if (linear <= 0) return '-inf'
  const db = linearToDb(linear)
  return `${db.toFixed(1)} dB`
}

/**
 * Convert a linear level (0-1) to a meter pixel height.
 * Uses a quasi-logarithmic scale for natural meter appearance.
 */
export function levelToMeterHeight(level: number, maxHeight: number): number {
  if (level <= 0) return 0
  if (level >= 1) return maxHeight
  // Use a power curve for more natural meter behavior
  // Maps 0->0, 1->maxHeight, with more resolution at lower levels
  const db = linearToDb(level)
  // Map -60dB to 0dB onto 0 to maxHeight
  const minDb = -60
  const normalized = Math.max(0, (db - minDb) / (0 - minDb))
  return normalized * maxHeight
}

/**
 * Convert a pan value (-1 to 1) to a display string.
 * -1 -> "100L", 0 -> "C", 1 -> "100R"
 */
export function panToString(pan: number): string {
  if (pan === 0) return 'C'
  const pct = Math.round(Math.abs(pan) * 100)
  return pan < 0 ? `${pct}L` : `${pct}R`
}

/** Maximum linear gain value corresponding to +6dB */
export const MAX_VOLUME_LINEAR = dbToLinear(6) // ~1.9953

/** Clamp volume to valid range [0, MAX_VOLUME_LINEAR] */
export function clampVolume(v: number): number {
  return Math.max(0, Math.min(MAX_VOLUME_LINEAR, v))
}

/** Clamp pan to valid range [-1, 1] */
export function clampPan(p: number): number {
  return Math.max(-1, Math.min(1, p))
}

/**
 * Convert fader position (0-1) to linear gain.
 * Position 0 = silence (gain 0), position ~0.75 = 0dB (gain 1.0), position 1 = +6dB.
 * Uses a logarithmic mapping for natural feel.
 */
export function faderToGain(position: number): number {
  if (position <= 0) return 0
  if (position >= 1) return MAX_VOLUME_LINEAR
  // Map 0-1 to -60dB to +6dB, then convert to linear
  const minDb = -60
  const maxDb = 6
  const db = minDb + position * (maxDb - minDb)
  return dbToLinear(db)
}

/**
 * Convert linear gain to fader position (0-1).
 * Inverse of faderToGain.
 */
export function gainToFader(gain: number): number {
  if (gain <= 0) return 0
  if (gain >= MAX_VOLUME_LINEAR) return 1
  const db = linearToDb(gain)
  const minDb = -60
  const maxDb = 6
  return (db - minDb) / (maxDb - minDb)
}
