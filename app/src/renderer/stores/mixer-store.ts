import { create } from 'zustand'
import type { MixerState, MixerActions, LevelData, EffectSlotInfo, ChainId } from '../types/mixer'
import { MASTER_CHAIN } from '../types/mixer'
import { EFFECTS, clampParam, findParam, isEffectType } from '../../shared/catalog'
import { clampVolume, clampPan } from '../utils/mixer-helpers'

const DEFAULT_LEVEL: LevelData = { peakL: 0, peakR: 0 }

const newSlot = (effectType: string): EffectSlotInfo => ({
  id: crypto.randomUUID(),
  effectType,
  bypassed: false,
  params: {},
})

export const useMixerStore = create<MixerState & MixerActions>((set, get) => ({
  // Default state
  mixerVisible: false,
  mixerHeight: 300,
  trackVolumes: {},
  trackPans: {},
  trackLevels: {},
  trackEffects: {},
  masterVolume: 1.0,
  masterPan: 0.0,
  masterLevel: { ...DEFAULT_LEVEL },
  masterEffects: [],

  // Visibility
  toggleMixerVisible: () => set((state) => ({ mixerVisible: !state.mixerVisible })),
  setMixerVisible: (visible: boolean) => set({ mixerVisible: visible }),

  // Per-track volume/pan
  setTrackVolume: (trackId: string, volume: number) =>
    set((state) => ({
      trackVolumes: { ...state.trackVolumes, [trackId]: clampVolume(volume) },
    })),

  setTrackPan: (trackId: string, pan: number) =>
    set((state) => ({
      trackPans: { ...state.trackPans, [trackId]: clampPan(pan) },
    })),

  getTrackVolume: (trackId: string): number => {
    return get().trackVolumes[trackId] ?? 1.0
  },

  getTrackPan: (trackId: string): number => {
    return get().trackPans[trackId] ?? 0.0
  },

  // Level meters
  setTrackLevel: (trackId: string, level: LevelData) =>
    set((state) => ({
      trackLevels: { ...state.trackLevels, [trackId]: level },
    })),

  setMasterLevel: (level: LevelData) => set({ masterLevel: level }),

  // Master
  setMasterVolume: (volume: number) => set({ masterVolume: clampVolume(volume) }),
  setMasterPan: (pan: number) => set({ masterPan: clampPan(pan) }),

  // Track effects
  getTrackEffects: (trackId: string): EffectSlotInfo[] => {
    return get().trackEffects[trackId] ?? []
  },

  addTrackEffect: (trackId: string, effectType: string) =>
    set((state) => {
      const current = state.trackEffects[trackId] ?? []
      return {
        trackEffects: {
          ...state.trackEffects,
          [trackId]: [...current, newSlot(effectType)],
        },
      }
    }),

  removeTrackEffect: (trackId: string, index: number) =>
    set((state) => {
      const current = state.trackEffects[trackId] ?? []
      if (index < 0 || index >= current.length) return state
      const updated = [...current]
      updated.splice(index, 1)
      return {
        trackEffects: { ...state.trackEffects, [trackId]: updated },
      }
    }),

  reorderTrackEffect: (trackId: string, fromIndex: number, toIndex: number) =>
    set((state) => {
      const current = state.trackEffects[trackId] ?? []
      if (
        fromIndex < 0 || fromIndex >= current.length ||
        toIndex < 0 || toIndex >= current.length
      ) return state
      const updated = [...current]
      const [moved] = updated.splice(fromIndex, 1)
      updated.splice(toIndex, 0, moved)
      return {
        trackEffects: { ...state.trackEffects, [trackId]: updated },
      }
    }),

  bypassTrackEffect: (trackId: string, index: number, bypassed: boolean) =>
    set((state) => {
      const current = state.trackEffects[trackId] ?? []
      if (index < 0 || index >= current.length) return state
      const updated = current.map((slot, i) =>
        i === index ? { ...slot, bypassed } : slot,
      )
      return {
        trackEffects: { ...state.trackEffects, [trackId]: updated },
      }
    }),

  // Master effects
  addMasterEffect: (effectType: string) =>
    set((state) => ({
      masterEffects: [...state.masterEffects, newSlot(effectType)],
    })),

  removeMasterEffect: (index: number) =>
    set((state) => {
      if (index < 0 || index >= state.masterEffects.length) return state
      const updated = [...state.masterEffects]
      updated.splice(index, 1)
      return { masterEffects: updated }
    }),

  reorderMasterEffect: (fromIndex: number, toIndex: number) =>
    set((state) => {
      const { masterEffects } = state
      if (
        fromIndex < 0 || fromIndex >= masterEffects.length ||
        toIndex < 0 || toIndex >= masterEffects.length
      ) return state
      const updated = [...masterEffects]
      const [moved] = updated.splice(fromIndex, 1)
      updated.splice(toIndex, 0, moved)
      return { masterEffects: updated }
    }),

  bypassMasterEffect: (index: number, bypassed: boolean) =>
    set((state) => {
      if (index < 0 || index >= state.masterEffects.length) return state
      const updated = state.masterEffects.map((slot, i) =>
        i === index ? { ...slot, bypassed } : slot,
      )
      return { masterEffects: updated }
    }),

  // Effect settings
  getChain: (chainId: ChainId): EffectSlotInfo[] =>
    chainId === MASTER_CHAIN ? get().masterEffects : (get().trackEffects[chainId] ?? []),

  setEffectParam: (chainId: ChainId, index: number, name: string, value: number) =>
    set((state) => {
      const chain =
        chainId === MASTER_CHAIN ? state.masterEffects : (state.trackEffects[chainId] ?? [])
      const slot = chain[index]
      if (!slot || !isEffectType(slot.effectType)) return state
      const spec = findParam(EFFECTS[slot.effectType].params, name)
      if (!spec) return state
      const clamped = clampParam(spec, value)
      if (slot.params[name] === clamped) return state

      const updated = chain.map((s, i) =>
        i === index ? { ...s, params: { ...s.params, [name]: clamped } } : s,
      )
      return chainId === MASTER_CHAIN
        ? { masterEffects: updated }
        : { trackEffects: { ...state.trackEffects, [chainId]: updated } }
    }),

  // Panel height
  setMixerHeight: (height: number) => set({ mixerHeight: height }),

  // Cleanup
  removeTrackData: (trackId: string) =>
    set((state) => {
      const { [trackId]: _v, ...trackVolumes } = state.trackVolumes
      const { [trackId]: _p, ...trackPans } = state.trackPans
      const { [trackId]: _l, ...trackLevels } = state.trackLevels
      const { [trackId]: _e, ...trackEffects } = state.trackEffects
      return { trackVolumes, trackPans, trackLevels, trackEffects }
    }),
}))
