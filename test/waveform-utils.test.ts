import { describe, it, expect } from 'vitest'
import { peaksToVertices, generateMockPeaks } from '../app/src/renderer/utils/waveform'

describe('waveform utilities', () => {
  describe('peaksToVertices', () => {
    it('returns a flat array of x,y coordinate pairs', () => {
      const peaks = new Float32Array([0.5, -0.3, 0.8])
      const result = peaksToVertices(peaks, 100, 60)
      // Should be a flat number array
      expect(Array.isArray(result)).toBe(true)
      expect(result.length).toBeGreaterThan(0)
      // All values should be numbers
      for (const val of result) {
        expect(typeof val).toBe('number')
      }
    })

    it('output length equals peaks length * 2 (top + bottom) * 2 (x,y per vertex)', () => {
      const peaks = new Float32Array([0.5, -0.3, 0.8])
      const result = peaksToVertices(peaks, 100, 60)
      // Top line: 3 points, bottom line: 3 points = 6 vertices, each with x,y = 12 values
      expect(result.length).toBe(peaks.length * 2 * 2)
    })

    it('computes correct x positions spread across width', () => {
      const peaks = new Float32Array([0.5, 0.5, 0.5])
      const result = peaksToVertices(peaks, 90, 60)
      // First top vertex x
      expect(result[0]).toBe(0) // x of first top point
      // Second top vertex x
      expect(result[2]).toBe(30) // x = (1/3) * 90
      // Third top vertex x
      expect(result[4]).toBe(60) // x = (2/3) * 90
    })

    it('computes correct y positions based on peak values', () => {
      const peaks = new Float32Array([1.0])
      const result = peaksToVertices(peaks, 100, 60)
      // top_y = height/2 - peak * height/2 = 30 - 1.0 * 30 = 0
      expect(result[1]).toBe(0) // y of top point for peak=1.0
      // bottom_y = height/2 + peak * height/2 = 30 + 1.0 * 30 = 60
      expect(result[3]).toBe(60) // y of bottom point (mirrored)
    })

    it('handles empty peaks array', () => {
      const peaks = new Float32Array([])
      const result = peaksToVertices(peaks, 100, 60)
      expect(result).toEqual([])
    })
  })

  describe('generateMockPeaks', () => {
    it('returns Float32Array of requested length', () => {
      const peaks = generateMockPeaks(100)
      expect(peaks).toBeInstanceOf(Float32Array)
      expect(peaks.length).toBe(100)
    })

    it('all values are between -1 and 1', () => {
      const peaks = generateMockPeaks(500)
      for (let i = 0; i < peaks.length; i++) {
        expect(peaks[i]).toBeGreaterThanOrEqual(-1)
        expect(peaks[i]).toBeLessThanOrEqual(1)
      }
    })

    it('generates varying values (not all zeros)', () => {
      const peaks = generateMockPeaks(100)
      const uniqueValues = new Set(peaks)
      expect(uniqueValues.size).toBeGreaterThan(1)
    })
  })
})
