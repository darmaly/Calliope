import { describe, it, expect, beforeEach } from 'vitest'
import { useMixerStore } from '../app/src/renderer/stores/mixer-store'
import { MAX_VOLUME_LINEAR } from '../app/src/renderer/utils/mixer-helpers'

describe('mixer store', () => {
  beforeEach(() => {
    useMixerStore.setState({
      mixerVisible: false,
      trackVolumes: {},
      trackPans: {},
      trackLevels: {},
      trackEffects: {},
      masterVolume: 1.0,
      masterPan: 0.0,
      masterLevel: { peakL: 0, peakR: 0 },
      masterEffects: [],
    })
  })

  describe('track volume', () => {
    it('defaults to 1.0 for unknown track', () => {
      expect(useMixerStore.getState().getTrackVolume('t1')).toBe(1.0)
    })

    it('sets and gets volume', () => {
      useMixerStore.getState().setTrackVolume('t1', 0.8)
      expect(useMixerStore.getState().getTrackVolume('t1')).toBe(0.8)
    })

    it('clamps volume above MAX_VOLUME_LINEAR', () => {
      useMixerStore.getState().setTrackVolume('t1', 2.0)
      expect(useMixerStore.getState().getTrackVolume('t1')).toBe(MAX_VOLUME_LINEAR)
    })

    it('clamps volume below 0', () => {
      useMixerStore.getState().setTrackVolume('t1', -1)
      expect(useMixerStore.getState().getTrackVolume('t1')).toBe(0)
    })
  })

  describe('track pan', () => {
    it('defaults to 0.0 for unknown track', () => {
      expect(useMixerStore.getState().getTrackPan('t1')).toBe(0.0)
    })

    it('sets and gets pan', () => {
      useMixerStore.getState().setTrackPan('t1', -0.5)
      expect(useMixerStore.getState().getTrackPan('t1')).toBe(-0.5)
    })

    it('clamps pan above 1', () => {
      useMixerStore.getState().setTrackPan('t1', 2.0)
      expect(useMixerStore.getState().getTrackPan('t1')).toBe(1.0)
    })

    it('clamps pan below -1', () => {
      useMixerStore.getState().setTrackPan('t1', -2.0)
      expect(useMixerStore.getState().getTrackPan('t1')).toBe(-1.0)
    })
  })

  describe('master volume', () => {
    it('defaults to 1.0', () => {
      expect(useMixerStore.getState().masterVolume).toBe(1.0)
    })

    it('sets master volume', () => {
      useMixerStore.getState().setMasterVolume(0.7)
      expect(useMixerStore.getState().masterVolume).toBe(0.7)
    })

    it('clamps master volume', () => {
      useMixerStore.getState().setMasterVolume(5.0)
      expect(useMixerStore.getState().masterVolume).toBe(MAX_VOLUME_LINEAR)
    })
  })

  describe('master pan', () => {
    it('defaults to 0.0', () => {
      expect(useMixerStore.getState().masterPan).toBe(0.0)
    })

    it('sets master pan', () => {
      useMixerStore.getState().setMasterPan(0.3)
      expect(useMixerStore.getState().masterPan).toBe(0.3)
    })
  })

  describe('level meters', () => {
    it('defaults to zero levels for unknown track', () => {
      // No explicit default accessor needed - trackLevels empty means zeros
      expect(useMixerStore.getState().trackLevels['t1']).toBeUndefined()
    })

    it('sets track level', () => {
      useMixerStore.getState().setTrackLevel('t1', { peakL: 0.8, peakR: 0.6 })
      expect(useMixerStore.getState().trackLevels['t1']).toEqual({
        peakL: 0.8,
        peakR: 0.6,
      })
    })

    it('sets master level', () => {
      useMixerStore.getState().setMasterLevel({ peakL: 0.9, peakR: 0.85 })
      expect(useMixerStore.getState().masterLevel).toEqual({
        peakL: 0.9,
        peakR: 0.85,
      })
    })
  })

  describe('effect slots', () => {
    it('returns empty array for track with no effects', () => {
      expect(useMixerStore.getState().getTrackEffects('t1')).toEqual([])
    })

    it('adds an effect to a track', () => {
      useMixerStore.getState().addTrackEffect('t1', 'eq')
      expect(useMixerStore.getState().getTrackEffects('t1')).toEqual([
        { effectType: 'eq', bypassed: false },
      ])
    })

    it('adds multiple effects', () => {
      useMixerStore.getState().addTrackEffect('t1', 'eq')
      useMixerStore.getState().addTrackEffect('t1', 'compressor')
      const effects = useMixerStore.getState().getTrackEffects('t1')
      expect(effects).toHaveLength(2)
      expect(effects[0].effectType).toBe('eq')
      expect(effects[1].effectType).toBe('compressor')
    })

    it('removes an effect by index', () => {
      useMixerStore.getState().addTrackEffect('t1', 'eq')
      useMixerStore.getState().addTrackEffect('t1', 'compressor')
      useMixerStore.getState().removeTrackEffect('t1', 0)
      const effects = useMixerStore.getState().getTrackEffects('t1')
      expect(effects).toHaveLength(1)
      expect(effects[0].effectType).toBe('compressor')
    })

    it('reorders effects', () => {
      useMixerStore.getState().addTrackEffect('t1', 'eq')
      useMixerStore.getState().addTrackEffect('t1', 'compressor')
      useMixerStore.getState().addTrackEffect('t1', 'reverb')
      useMixerStore.getState().reorderTrackEffect('t1', 2, 0)
      const effects = useMixerStore.getState().getTrackEffects('t1')
      expect(effects[0].effectType).toBe('reverb')
      expect(effects[1].effectType).toBe('eq')
      expect(effects[2].effectType).toBe('compressor')
    })

    it('bypasses an effect', () => {
      useMixerStore.getState().addTrackEffect('t1', 'eq')
      useMixerStore.getState().bypassTrackEffect('t1', 0, true)
      expect(useMixerStore.getState().getTrackEffects('t1')[0].bypassed).toBe(true)
    })
  })

  describe('master effects', () => {
    it('starts with empty master effects', () => {
      expect(useMixerStore.getState().masterEffects).toEqual([])
    })

    it('adds a master effect', () => {
      useMixerStore.getState().addMasterEffect('limiter')
      expect(useMixerStore.getState().masterEffects).toEqual([
        { effectType: 'limiter', bypassed: false },
      ])
    })

    it('removes a master effect', () => {
      useMixerStore.getState().addMasterEffect('limiter')
      useMixerStore.getState().addMasterEffect('eq')
      useMixerStore.getState().removeMasterEffect(0)
      expect(useMixerStore.getState().masterEffects).toHaveLength(1)
      expect(useMixerStore.getState().masterEffects[0].effectType).toBe('eq')
    })

    it('reorders master effects', () => {
      useMixerStore.getState().addMasterEffect('eq')
      useMixerStore.getState().addMasterEffect('compressor')
      useMixerStore.getState().reorderMasterEffect(1, 0)
      expect(useMixerStore.getState().masterEffects[0].effectType).toBe('compressor')
      expect(useMixerStore.getState().masterEffects[1].effectType).toBe('eq')
    })

    it('bypasses a master effect', () => {
      useMixerStore.getState().addMasterEffect('limiter')
      useMixerStore.getState().bypassMasterEffect(0, true)
      expect(useMixerStore.getState().masterEffects[0].bypassed).toBe(true)
    })
  })

  describe('mixer visibility', () => {
    it('defaults to false', () => {
      expect(useMixerStore.getState().mixerVisible).toBe(false)
    })

    it('toggles visibility', () => {
      useMixerStore.getState().toggleMixerVisible()
      expect(useMixerStore.getState().mixerVisible).toBe(true)
      useMixerStore.getState().toggleMixerVisible()
      expect(useMixerStore.getState().mixerVisible).toBe(false)
    })

    it('sets visibility directly', () => {
      useMixerStore.getState().setMixerVisible(true)
      expect(useMixerStore.getState().mixerVisible).toBe(true)
    })
  })

  describe('track data cleanup', () => {
    it('removes all data for a track', () => {
      useMixerStore.getState().setTrackVolume('t1', 0.5)
      useMixerStore.getState().setTrackPan('t1', -0.3)
      useMixerStore.getState().setTrackLevel('t1', { peakL: 0.5, peakR: 0.5 })
      useMixerStore.getState().addTrackEffect('t1', 'eq')

      useMixerStore.getState().removeTrackData('t1')

      expect(useMixerStore.getState().trackVolumes['t1']).toBeUndefined()
      expect(useMixerStore.getState().trackPans['t1']).toBeUndefined()
      expect(useMixerStore.getState().trackLevels['t1']).toBeUndefined()
      expect(useMixerStore.getState().trackEffects['t1']).toBeUndefined()
    })
  })
})
