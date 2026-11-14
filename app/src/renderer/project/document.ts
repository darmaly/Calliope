// The project document: everything that makes up a piece of music.
//
// The Zustand stores hold it; this module gives it one shape so that the
// things which need "the whole project" all agree on what that is:
//   - engine-sync projects it onto the audio engine,
//   - history snapshots it for undo/redo,
//   - save/load serialises it,
//   - the AI agent reads it and edits it.
//
// View state (scroll, zoom, selection, meters, which panel is open) is not
// part of the document.

import { useTimelineStore } from '../stores/timeline-store'
import { useMixerStore } from '../stores/mixer-store'
import type { Clip, LoopRegion, Track } from '../types/timeline'
import type { EffectSlotInfo } from '../types/mixer'
import type { MidiNote } from '../types/piano-roll'
import {
  EFFECTS,
  INSTRUMENTS,
  clampParam,
  findParam,
  isEffectType,
  isInstrumentType,
} from '../../shared/catalog'

export const DOCUMENT_FORMAT_VERSION = 2
export const DOCUMENT_APP_ID = 'calliope'

export interface ProjectDocument {
  tracks: Track[]
  clips: Record<string, Clip>
  mixer: {
    trackVolumes: Record<string, number>
    trackPans: Record<string, number>
    trackEffects: Record<string, EffectSlotInfo[]>
    masterVolume: number
    masterEffects: EffectSlotInfo[]
  }
  transport: {
    bpm: number
    timeSignature: { numerator: number; denominator: number }
    loopEnabled: boolean
    loopRegion: LoopRegion | null
    metronomeEnabled: boolean
  }
}

export function emptyDocument(): ProjectDocument {
  return {
    tracks: [],
    clips: {},
    mixer: {
      trackVolumes: {},
      trackPans: {},
      trackEffects: {},
      masterVolume: 1,
      masterEffects: [],
    },
    transport: {
      bpm: 120,
      timeSignature: { numerator: 4, denominator: 4 },
      loopEnabled: false,
      loopRegion: null,
      metronomeEnabled: false,
    },
  }
}

/**
 * The current document. Parts are returned by reference, not copied: the
 * stores never mutate in place, so a part that has not changed is the same
 * object as last time. Everything that diffs documents relies on that.
 */
export function getDocument(): ProjectDocument {
  const t = useTimelineStore.getState()
  const m = useMixerStore.getState()
  return {
    tracks: t.tracks,
    clips: t.clips,
    mixer: {
      trackVolumes: m.trackVolumes,
      trackPans: m.trackPans,
      trackEffects: m.trackEffects,
      masterVolume: m.masterVolume,
      masterEffects: m.masterEffects,
    },
    transport: {
      bpm: t.bpm,
      timeSignature: t.timeSignature,
      loopEnabled: t.loopEnabled,
      loopRegion: t.loopRegion,
      metronomeEnabled: t.metronomeEnabled,
    },
  }
}

/** True when two documents are the same by identity of their parts. */
export function sameDocument(a: ProjectDocument, b: ProjectDocument): boolean {
  return (
    a.tracks === b.tracks &&
    a.clips === b.clips &&
    a.mixer.trackVolumes === b.mixer.trackVolumes &&
    a.mixer.trackPans === b.mixer.trackPans &&
    a.mixer.trackEffects === b.mixer.trackEffects &&
    a.mixer.masterVolume === b.mixer.masterVolume &&
    a.mixer.masterEffects === b.mixer.masterEffects &&
    a.transport.bpm === b.transport.bpm &&
    a.transport.timeSignature === b.transport.timeSignature &&
    a.transport.loopEnabled === b.transport.loopEnabled &&
    a.transport.loopRegion === b.transport.loopRegion &&
    a.transport.metronomeEnabled === b.transport.metronomeEnabled
  )
}

/** Replace the whole document. Selection that no longer points at anything is dropped. */
export function applyDocument(doc: ProjectDocument): void {
  const timeline = useTimelineStore.getState()
  const selectedClipIds = new Set([...timeline.selectedClipIds].filter((id) => id in doc.clips))
  const selectedTrackId =
    timeline.selectedTrackId && doc.tracks.some((t) => t.id === timeline.selectedTrackId)
      ? timeline.selectedTrackId
      : null

  useTimelineStore.setState({
    tracks: doc.tracks,
    clips: doc.clips,
    bpm: doc.transport.bpm,
    timeSignature: doc.transport.timeSignature,
    loopEnabled: doc.transport.loopEnabled,
    loopRegion: doc.transport.loopRegion,
    metronomeEnabled: doc.transport.metronomeEnabled,
    selectedClipIds,
    selectedTrackId,
  })

  useMixerStore.setState({
    trackVolumes: doc.mixer.trackVolumes,
    trackPans: doc.mixer.trackPans,
    trackEffects: doc.mixer.trackEffects,
    masterVolume: doc.mixer.masterVolume,
    masterEffects: doc.mixer.masterEffects,
  })
}

/** Calls `listener` whenever the document changes. Returns an unsubscribe function. */
export function subscribeDocument(listener: (doc: ProjectDocument) => void): () => void {
  let last = getDocument()
  const check = () => {
    const current = getDocument()
    if (!sameDocument(last, current)) {
      last = current
      listener(current)
    }
  }
  const unsubscribeTimeline = useTimelineStore.subscribe(check)
  const unsubscribeMixer = useMixerStore.subscribe(check)
  return () => {
    unsubscribeTimeline()
    unsubscribeMixer()
  }
}

// ---------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------

export function serializeDocument(doc: ProjectDocument): string {
  // Waveform peaks are display data derived from audio; they are not saved.
  const clips: Record<string, Clip> = {}
  for (const [id, clip] of Object.entries(doc.clips)) {
    const { peakData: _peaks, ...rest } = clip
    clips[id] = rest
  }
  return JSON.stringify(
    {
      app: DOCUMENT_APP_ID,
      formatVersion: DOCUMENT_FORMAT_VERSION,
      savedAt: new Date().toISOString(),
      document: { ...doc, clips },
    },
    null,
    2,
  )
}

export class ProjectFormatError extends Error {}

const isObject = (v: unknown): v is Record<string, unknown> =>
  typeof v === 'object' && v !== null && !Array.isArray(v)

const num = (v: unknown, fallback: number, min = -Infinity, max = Infinity): number =>
  typeof v === 'number' && Number.isFinite(v) ? Math.min(max, Math.max(min, v)) : fallback

const str = (v: unknown, fallback: string): string => (typeof v === 'string' ? v : fallback)

function parseParams(
  raw: unknown,
  specs: Array<{ name: string }>,
  clamp: (name: string, value: number) => number | undefined,
): Record<string, number> {
  const out: Record<string, number> = {}
  if (!isObject(raw)) return out
  for (const spec of specs) {
    const v = raw[spec.name]
    if (typeof v === 'number' && Number.isFinite(v)) {
      const clamped = clamp(spec.name, v)
      if (clamped !== undefined) out[spec.name] = clamped
    }
  }
  return out
}

function parseEffects(raw: unknown): EffectSlotInfo[] {
  if (!Array.isArray(raw)) return []
  const out: EffectSlotInfo[] = []
  for (const item of raw) {
    if (!isObject(item) || !isEffectType(item.effectType)) continue
    const specs = EFFECTS[item.effectType].params
    out.push({
      id: str(item.id, '') || crypto.randomUUID(),
      effectType: item.effectType,
      bypassed: item.bypassed === true,
      params: parseParams(item.params, specs, (name, value) => {
        const spec = findParam(specs, name)
        return spec ? clampParam(spec, value) : undefined
      }),
    })
  }
  return out
}

function parseNotes(raw: unknown): Record<string, MidiNote> {
  const out: Record<string, MidiNote> = {}
  const items = Array.isArray(raw) ? raw : isObject(raw) ? Object.values(raw) : []
  for (const item of items) {
    if (!isObject(item)) continue
    const lengthBeats = num(item.lengthBeats, 0, 0)
    if (lengthBeats <= 0) continue
    const id = str(item.id, '') || crypto.randomUUID()
    out[id] = {
      id,
      pitch: Math.round(num(item.pitch, 60, 0, 127)),
      startBeat: num(item.startBeat, 0, 0),
      lengthBeats,
      velocity: Math.round(num(item.velocity, 100, 1, 127)),
    }
  }
  return out
}

/**
 * Parse a saved project. Throws ProjectFormatError when the text is not a
 * Calliope project; anything recoverable inside one (an unknown parameter, an
 * out-of-range value, a clip on a missing track) is repaired or dropped.
 */
export function parseDocument(text: string): ProjectDocument {
  let root: unknown
  try {
    root = JSON.parse(text)
  } catch {
    throw new ProjectFormatError('This file is not a Calliope project (it is not valid JSON).')
  }
  if (!isObject(root) || root.app !== DOCUMENT_APP_ID || !isObject(root.document)) {
    throw new ProjectFormatError('This file is not a Calliope project.')
  }
  if (typeof root.formatVersion === 'number' && root.formatVersion > DOCUMENT_FORMAT_VERSION) {
    throw new ProjectFormatError(
      'This project was saved by a newer version of Calliope and cannot be opened by this one.',
    )
  }

  const raw = root.document
  const doc = emptyDocument()

  const trackIds = new Set<string>()
  if (Array.isArray(raw.tracks)) {
    for (const item of raw.tracks) {
      if (!isObject(item) || typeof item.id !== 'string' || !item.id || trackIds.has(item.id)) continue
      const instrument = isInstrumentType(item.instrument) ? item.instrument : 'polysynth'
      const specs = INSTRUMENTS[instrument].params
      trackIds.add(item.id)
      doc.tracks.push({
        id: item.id,
        name: str(item.name, `Track ${doc.tracks.length + 1}`),
        instrument,
        params: parseParams(item.params, specs, (name, value) => {
          const spec = findParam(specs, name)
          return spec ? clampParam(spec, value) : undefined
        }),
        colorIndex: Math.round(num(item.colorIndex, doc.tracks.length % 8, 0, 7)),
        muted: item.muted === true,
        solo: item.solo === true,
        armed: false,
        order: num(item.order, doc.tracks.length),
      })
    }
  }

  if (isObject(raw.clips)) {
    for (const item of Object.values(raw.clips)) {
      if (!isObject(item) || typeof item.id !== 'string' || !item.id) continue
      if (typeof item.trackId !== 'string' || !trackIds.has(item.trackId)) continue
      const lengthBeats = num(item.lengthBeats, 0, 0)
      if (lengthBeats <= 0) continue
      const type = item.type === 'audio' ? 'audio' : 'midi'
      doc.clips[item.id] = {
        id: item.id,
        trackId: item.trackId,
        type,
        startBeat: num(item.startBeat, 0, 0),
        lengthBeats,
        name: str(item.name, type === 'midi' ? 'MIDI Clip' : 'Audio Clip'),
        ...(type === 'midi' ? { notes: parseNotes(item.notes) } : {}),
      }
    }
  }

  if (isObject(raw.mixer)) {
    const mixer = raw.mixer
    if (isObject(mixer.trackVolumes)) {
      for (const [id, v] of Object.entries(mixer.trackVolumes))
        if (trackIds.has(id)) doc.mixer.trackVolumes[id] = num(v, 1, 0, 2)
    }
    if (isObject(mixer.trackPans)) {
      for (const [id, v] of Object.entries(mixer.trackPans))
        if (trackIds.has(id)) doc.mixer.trackPans[id] = num(v, 0, -1, 1)
    }
    if (isObject(mixer.trackEffects)) {
      for (const [id, v] of Object.entries(mixer.trackEffects))
        if (trackIds.has(id)) doc.mixer.trackEffects[id] = parseEffects(v)
    }
    doc.mixer.masterVolume = num(mixer.masterVolume, 1, 0, 2)
    doc.mixer.masterEffects = parseEffects(mixer.masterEffects)
  }

  if (isObject(raw.transport)) {
    const transport = raw.transport
    doc.transport.bpm = num(transport.bpm, 120, 20, 300)
    if (isObject(transport.timeSignature)) {
      const numerator = Math.round(num(transport.timeSignature.numerator, 4, 1, 32))
      const denominator = num(transport.timeSignature.denominator, 4)
      doc.transport.timeSignature = {
        numerator,
        denominator: [1, 2, 4, 8, 16, 32].includes(denominator) ? denominator : 4,
      }
    }
    if (isObject(transport.loopRegion)) {
      const startBeat = num(transport.loopRegion.startBeat, 0, 0)
      const endBeat = num(transport.loopRegion.endBeat, 0, 0)
      if (endBeat > startBeat) doc.transport.loopRegion = { startBeat, endBeat }
    }
    doc.transport.loopEnabled = transport.loopEnabled === true && doc.transport.loopRegion !== null
    doc.transport.metronomeEnabled = transport.metronomeEnabled === true
  }

  return doc
}
