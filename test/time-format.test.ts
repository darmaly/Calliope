import { describe, it, expect } from 'vitest'
import {
  beatToTimeMs,
  formatTimeMs,
  beatToBarsBeats,
  formatBarsBeats,
} from '../app/src/renderer/utils/time-format'

describe('time-format utilities', () => {
  describe('beatToTimeMs', () => {
    it('returns 0 for beat 0 at any BPM', () => {
      expect(beatToTimeMs(0, 120)).toBe(0)
    })

    it('converts 120 beats at 120 BPM to 60000ms (1 minute)', () => {
      expect(beatToTimeMs(120, 120)).toBe(60000)
    })

    it('converts 1 beat at 60 BPM to 1000ms', () => {
      expect(beatToTimeMs(1, 60)).toBe(1000)
    })
  })

  describe('formatTimeMs', () => {
    it('formats 0ms as 00:00.000', () => {
      expect(formatTimeMs(0)).toBe('00:00.000')
    })

    it('formats 61500ms as 01:01.500', () => {
      expect(formatTimeMs(61500)).toBe('01:01.500')
    })

    it('formats 3723456ms as 62:03.456 (minutes exceed 59)', () => {
      expect(formatTimeMs(3723456)).toBe('62:03.456')
    })
  })

  describe('beatToBarsBeats', () => {
    it('returns bar 1 beat 1 for beat 0 in 4/4', () => {
      expect(beatToBarsBeats(0, 4)).toEqual({ bar: 1, beatInBar: 1 })
    })

    it('returns bar 2 beat 1 for beat 4 in 4/4', () => {
      expect(beatToBarsBeats(4, 4)).toEqual({ bar: 2, beatInBar: 1 })
    })

    it('returns bar 2 beat 4 for beat 7 in 4/4', () => {
      expect(beatToBarsBeats(7, 4)).toEqual({ bar: 2, beatInBar: 4 })
    })

    it('returns bar 1 beat 1 for beat 0 in 3/4', () => {
      expect(beatToBarsBeats(0, 3)).toEqual({ bar: 1, beatInBar: 1 })
    })
  })

  describe('formatBarsBeats', () => {
    it('formats bar 1 beat 1 as 001:01', () => {
      expect(formatBarsBeats(1, 1)).toBe('001:01')
    })

    it('formats bar 12 beat 3 as 012:03', () => {
      expect(formatBarsBeats(12, 3)).toBe('012:03')
    })
  })
})
