import { describe, it, expect } from 'vitest'
import {
  pitchToNoteName,
  isBlackKey,
  pitchToY,
  yToPitch,
  velocityToAlpha,
} from '../app/src/renderer/utils/piano-helpers'

describe('pitchToNoteName', () => {
  it('converts pitch 60 to C3 (MIDI standard: note 0 = C-2)', () => {
    expect(pitchToNoteName(60)).toBe('C3')
  })

  it('converts pitch 0 to C-2', () => {
    expect(pitchToNoteName(0)).toBe('C-2')
  })

  it('converts pitch 127 to G8', () => {
    expect(pitchToNoteName(127)).toBe('G8')
  })

  it('converts pitch 69 to A3 (MIDI standard: note 0 = C-2)', () => {
    expect(pitchToNoteName(69)).toBe('A3')
  })
})

describe('isBlackKey', () => {
  it('C#4 (61) is black', () => {
    expect(isBlackKey(61)).toBe(true)
  })

  it('C4 (60) is white', () => {
    expect(isBlackKey(60)).toBe(false)
  })

  it('F#4 (66) is black', () => {
    expect(isBlackKey(66)).toBe(true)
  })

  it('E4 (64) is white', () => {
    expect(isBlackKey(64)).toBe(false)
  })
})

describe('pitchToY', () => {
  it('pitch 127 maps to y=0', () => {
    expect(pitchToY(127, 16)).toBe(0)
  })

  it('pitch 0 maps to y=127*16', () => {
    expect(pitchToY(0, 16)).toBe(127 * 16)
  })

  it('pitch 60 maps to y=67*16', () => {
    expect(pitchToY(60, 16)).toBe(67 * 16)
  })
})

describe('yToPitch', () => {
  it('y=0 maps to pitch 127', () => {
    expect(yToPitch(0, 16, 0)).toBe(127)
  })

  it('y=127*16 maps to pitch 0', () => {
    expect(yToPitch(127 * 16, 16, 0)).toBe(0)
  })

  it('accounts for scrollY', () => {
    // y=0 with scrollY=16 => (0+16)/16=1 => 127-1=126
    expect(yToPitch(0, 16, 16)).toBe(126)
  })
})

describe('velocityToAlpha', () => {
  it('velocity 127 is approximately 1.0', () => {
    expect(velocityToAlpha(127)).toBeCloseTo(1.0, 1)
  })

  it('velocity 1 is approximately 0.206', () => {
    expect(velocityToAlpha(1)).toBeCloseTo(0.206, 2)
  })

  it('velocity 64 is approximately 0.603', () => {
    expect(velocityToAlpha(64)).toBeCloseTo(0.603, 2)
  })
})
