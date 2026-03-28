import { describe, it, expect } from 'vitest'
import { beatToPixel, pixelToBeat, snapToBeat } from '../app/src/renderer/utils/beat-math'
import { TRACK_COLORS } from '../app/src/renderer/utils/colors'

describe('beatToPixel', () => {
  it('converts beat to pixel position', () => {
    expect(beatToPixel(4, 24, 0)).toBe(96)
  })

  it('accounts for scroll offset', () => {
    expect(beatToPixel(4, 24, 48)).toBe(48)
  })
})

describe('pixelToBeat', () => {
  it('converts pixel to beat position', () => {
    expect(pixelToBeat(96, 24, 0)).toBe(4)
  })

  it('accounts for scroll offset', () => {
    expect(pixelToBeat(96, 24, 48)).toBe(6)
  })
})

describe('snapToBeat', () => {
  it('snaps down to nearest grid line', () => {
    expect(snapToBeat(3.3, 0.25)).toBe(3.25)
  })

  it('snaps up to nearest grid line', () => {
    expect(snapToBeat(3.4, 0.25)).toBe(3.5)
  })

  it('snaps to eighth-note grid', () => {
    expect(snapToBeat(3.13, 0.125)).toBe(3.125)
  })

  it('snaps zero correctly', () => {
    expect(snapToBeat(0, 0.25)).toBe(0)
  })
})

describe('TRACK_COLORS', () => {
  it('has exactly 8 entries', () => {
    expect(TRACK_COLORS).toHaveLength(8)
  })

  it('first color is blue', () => {
    expect(TRACK_COLORS[0]).toBe('#4a9eff')
  })
})
