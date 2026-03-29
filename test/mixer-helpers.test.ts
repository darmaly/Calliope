import { describe, it, expect } from 'vitest'
import {
  dbToLinear,
  linearToDb,
  formatDb,
  levelToMeterHeight,
  panToString,
  clampVolume,
  clampPan,
  MAX_VOLUME_LINEAR,
} from '../app/src/renderer/utils/mixer-helpers'

describe('mixer-helpers', () => {
  describe('dbToLinear', () => {
    it('converts 0 dB to 1.0', () => {
      expect(dbToLinear(0)).toBe(1.0)
    })

    it('converts -6 dB to approximately 0.501', () => {
      expect(dbToLinear(-6)).toBeCloseTo(0.501, 2)
    })

    it('converts -Infinity to 0', () => {
      expect(dbToLinear(-Infinity)).toBe(0)
    })

    it('converts +6 dB to approximately 1.995', () => {
      expect(dbToLinear(6)).toBeCloseTo(1.995, 2)
    })
  })

  describe('linearToDb', () => {
    it('converts 1.0 to 0 dB', () => {
      expect(linearToDb(1.0)).toBe(0)
    })

    it('converts 0.5 to approximately -6.02 dB', () => {
      expect(linearToDb(0.5)).toBeCloseTo(-6.02, 1)
    })

    it('converts 0 to -Infinity', () => {
      expect(linearToDb(0)).toBe(-Infinity)
    })
  })

  describe('formatDb', () => {
    it('formats 1.0 as "0.0 dB"', () => {
      expect(formatDb(1.0)).toBe('0.0 dB')
    })

    it('formats 0 as "-inf"', () => {
      expect(formatDb(0)).toBe('-inf')
    })

    it('formats 0.5 as "-6.0 dB"', () => {
      expect(formatDb(0.5)).toBe('-6.0 dB')
    })
  })

  describe('levelToMeterHeight', () => {
    it('returns 0 for level 0', () => {
      expect(levelToMeterHeight(0, 120)).toBe(0)
    })

    it('returns maxHeight for level 1.0', () => {
      expect(levelToMeterHeight(1.0, 120)).toBe(120)
    })

    it('returns a value between 0 and maxHeight for 0.5', () => {
      const h = levelToMeterHeight(0.5, 120)
      expect(h).toBeGreaterThan(0)
      expect(h).toBeLessThan(120)
    })
  })

  describe('panToString', () => {
    it('returns "100L" for -1', () => {
      expect(panToString(-1)).toBe('100L')
    })

    it('returns "C" for 0', () => {
      expect(panToString(0)).toBe('C')
    })

    it('returns "100R" for 1', () => {
      expect(panToString(1)).toBe('100R')
    })

    it('returns "50R" for 0.5', () => {
      expect(panToString(0.5)).toBe('50R')
    })

    it('returns "25L" for -0.25', () => {
      expect(panToString(-0.25)).toBe('25L')
    })
  })

  describe('clampVolume', () => {
    it('clamps negative to 0', () => {
      expect(clampVolume(-1)).toBe(0)
    })

    it('clamps above MAX_VOLUME_LINEAR', () => {
      expect(clampVolume(10)).toBe(MAX_VOLUME_LINEAR)
    })

    it('passes through valid values', () => {
      expect(clampVolume(0.5)).toBe(0.5)
    })
  })

  describe('clampPan', () => {
    it('clamps below -1', () => {
      expect(clampPan(-2)).toBe(-1)
    })

    it('clamps above 1', () => {
      expect(clampPan(2)).toBe(1)
    })

    it('passes through valid values', () => {
      expect(clampPan(0.3)).toBe(0.3)
    })
  })
})
