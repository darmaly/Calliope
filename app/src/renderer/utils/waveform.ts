/**
 * Convert peak amplitude data to a flat array of [x, y] vertex coordinates
 * for PixiJS polygon/mesh rendering.
 *
 * Produces a closed polygon: top line left-to-right, then bottom line right-to-left (mirrored).
 * Output is a flat array: [x0, y0, x1, y1, ...].
 */
export function peaksToVertices(peaks: Float32Array, width: number, height: number): number[] {
  const len = peaks.length
  if (len === 0) return []

  const halfHeight = height / 2
  const result: number[] = new Array(len * 4)

  for (let i = 0; i < len; i++) {
    const x = (i / len) * width
    const peak = Math.abs(peaks[i])

    // Top vertex (above center)
    const topIdx = i * 2
    result[topIdx] = x
    result[topIdx + 1] = halfHeight - peak * halfHeight

    // Bottom vertex (below center) — stored in reverse order at end
    const bottomIdx = (len * 2) + (len - 1 - i) * 2
    result[bottomIdx] = x
    result[bottomIdx + 1] = halfHeight + peak * halfHeight
  }

  return result
}

/**
 * Generate synthetic waveform peak data for testing/placeholder display.
 * Uses sine wave with noise, clamped to [-1, 1].
 */
export function generateMockPeaks(length: number): Float32Array {
  const peaks = new Float32Array(length)
  for (let i = 0; i < length; i++) {
    const value = Math.sin(i * 0.1) * 0.5 + (Math.random() - 0.5) * 0.3
    peaks[i] = Math.max(-1, Math.min(1, value))
  }
  return peaks
}
