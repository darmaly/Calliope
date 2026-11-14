// Keeps the audio engine in step with the project document.
//
// The document (in the stores) is the truth; the engine is a projection of
// it. Whenever the document changes, planEngineSync works out the smallest
// set of engine calls that takes the engine from the last document it was
// given to the new one. Nothing else in the renderer tells the engine what
// the project contains, so the UI, undo/redo, opening a file and the AI agent
// cannot drift apart: they all just change the document.

import type { ProjectDocument } from './document'
import { getDocument, subscribeDocument } from './document'
import type { Clip, Track } from '../types/timeline'
import type { EffectSlotInfo } from '../types/mixer'
import { MASTER_CHAIN } from '../types/mixer'
import { EFFECTS, INSTRUMENTS, isEffectType } from '../../shared/catalog'
import type { ParamSpec } from '../../shared/catalog'

export interface EngineClip {
  clipId: string
  trackId: string
  startBeat: number
  lengthBeats: number
  notes: Array<{ pitch: number; startBeat: number; lengthBeats: number; velocity: number }>
}

export type EngineCall =
  | { kind: 'addTrack'; trackId: string; instrument: string }
  | { kind: 'removeTrack'; trackId: string }
  | { kind: 'setClip'; clip: EngineClip }
  | { kind: 'removeClip'; clipId: string }
  | { kind: 'command'; command: string; params: Record<string, unknown> }
  | { kind: 'setBpm'; bpm: number }
  | { kind: 'setTimeSignature'; numerator: number; denominator: number }
  | { kind: 'setLoop'; startBeat: number; endBeat: number; enabled: boolean }
  | { kind: 'setMetronome'; enabled: boolean }

const setParameter = (id: string, value: number | boolean): EngineCall => ({
  kind: 'command',
  command: 'parameter.set',
  params: { id, value },
})

const engineValue = (spec: ParamSpec, value: number): number | boolean =>
  spec.kind === 'bool' ? value >= 0.5 : value

function toEngineClip(clip: Clip): EngineClip {
  return {
    clipId: clip.id,
    trackId: clip.trackId,
    startBeat: clip.startBeat,
    lengthBeats: clip.lengthBeats,
    notes: Object.values(clip.notes ?? {}).map((n) => ({
      pitch: n.pitch,
      startBeat: n.startBeat,
      lengthBeats: n.lengthBeats,
      velocity: n.velocity,
    })),
  }
}

const validEffects = (chain: EffectSlotInfo[] | undefined): EffectSlotInfo[] =>
  (chain ?? []).filter((slot) => isEffectType(slot.effectType))

/**
 * Engine calls that bring an effect chain from `before` to `after`.
 * Parameter IDs in the engine are positional (effects.{chain}.{slot}.{name}),
 * so when the line-up changes the chain is rebuilt from the end backwards
 * rather than patched, which keeps slot numbers and parameters aligned.
 */
function planChain(chainId: string, before: EffectSlotInfo[], after: EffectSlotInfo[]): EngineCall[] {
  const calls: EngineCall[] = []
  const sameLineUp =
    before.length === after.length &&
    before.every((slot, i) => slot.id === after[i].id && slot.effectType === after[i].effectType)

  if (!sameLineUp) {
    for (let position = before.length - 1; position >= 0; position--) {
      calls.push({ kind: 'command', command: 'effect.remove', params: { trackId: chainId, position } })
    }
  }

  after.forEach((slot, position) => {
    if (!isEffectType(slot.effectType)) return
    const previous = sameLineUp ? before[position] : undefined

    if (!previous) {
      calls.push({
        kind: 'command',
        command: 'effect.insert',
        params: { trackId: chainId, effectType: slot.effectType, position: -1 },
      })
    }
    if (slot.bypassed !== (previous?.bypassed ?? false)) {
      calls.push({
        kind: 'command',
        command: 'effect.bypass',
        params: { trackId: chainId, position, bypassed: slot.bypassed },
      })
    }
    for (const spec of EFFECTS[slot.effectType].params) {
      const value = slot.params[spec.name] ?? spec.default
      const was = previous ? (previous.params[spec.name] ?? spec.default) : spec.default
      if (value !== was) {
        calls.push(
          setParameter(`effects.${chainId}.${position}.${spec.name}`, engineValue(spec, value)),
        )
      }
    }
  })

  return calls
}

/**
 * The engine calls that take the engine from `prev` to `next`.
 * `prev` is null when the engine has just been reset and holds nothing.
 */
export function planEngineSync(prev: ProjectDocument | null, next: ProjectDocument): EngineCall[] {
  const calls: EngineCall[] = []
  const prevTracks = new Map<string, Track>((prev?.tracks ?? []).map((t) => [t.id, t]))
  const nextTracks = new Map<string, Track>(next.tracks.map((t) => [t.id, t]))

  // Tracks that are gone, or whose instrument was swapped, are removed first.
  // The engine drops their clips, effects and parameters with them.
  for (const [id, track] of prevTracks) {
    const now = nextTracks.get(id)
    if (!now || now.instrument !== track.instrument) calls.push({ kind: 'removeTrack', trackId: id })
  }

  // Tracks created in this step start from engine defaults.
  const fresh = new Set<string>()
  const ordered = [...next.tracks].sort((a, b) => a.order - b.order)

  for (const track of ordered) {
    const previous = prevTracks.get(track.id)
    const before = previous && previous.instrument === track.instrument ? previous : undefined
    if (!before) {
      calls.push({ kind: 'addTrack', trackId: track.id, instrument: track.instrument })
      fresh.add(track.id)
    }
    const id = track.id

    const volume = next.mixer.trackVolumes[id] ?? 1
    const pan = next.mixer.trackPans[id] ?? 0
    const volumeWas = before ? (prev!.mixer.trackVolumes[id] ?? 1) : 1
    const panWas = before ? (prev!.mixer.trackPans[id] ?? 0) : 0
    if (volume !== volumeWas) calls.push(setParameter(`track.${id}.volume`, volume))
    if (pan !== panWas) calls.push(setParameter(`track.${id}.pan`, pan))
    if (track.muted !== (before?.muted ?? false)) calls.push(setParameter(`track.${id}.mute`, track.muted))
    if (track.solo !== (before?.solo ?? false)) calls.push(setParameter(`track.${id}.solo`, track.solo))

    for (const spec of INSTRUMENTS[track.instrument].params) {
      const value = track.params[spec.name] ?? spec.default
      const was = before ? (before.params[spec.name] ?? spec.default) : spec.default
      if (value !== was) calls.push(setParameter(`track.${id}.${spec.name}`, engineValue(spec, value)))
    }

    calls.push(
      ...planChain(
        id,
        before ? validEffects(prev!.mixer.trackEffects[id]) : [],
        validEffects(next.mixer.trackEffects[id]),
      ),
    )
  }

  calls.push(
    ...planChain(
      MASTER_CHAIN,
      validEffects(prev?.mixer.masterEffects),
      validEffects(next.mixer.masterEffects),
    ),
  )

  // Clips. Only MIDI clips play; the engine has no audio-clip playback yet.
  const playable = (clip: Clip | undefined): clip is Clip =>
    clip !== undefined && clip.type === 'midi' && nextTracks.has(clip.trackId)

  for (const [clipId, clip] of Object.entries(prev?.clips ?? {})) {
    if (clip.type !== 'midi') continue
    if (playable(next.clips[clipId])) continue
    // If its track went away the engine has already dropped the clip.
    if (nextTracks.has(clip.trackId) && !fresh.has(clip.trackId)) calls.push({ kind: 'removeClip', clipId })
  }
  for (const [clipId, clip] of Object.entries(next.clips)) {
    if (!playable(clip)) continue
    const was = prev?.clips[clipId]
    if (was !== clip || fresh.has(clip.trackId)) {
      calls.push({ kind: 'setClip', clip: toEngineClip(clip) })
    }
  }

  // Transport and master
  const t = next.transport
  const p = prev?.transport
  if (!p || p.bpm !== t.bpm) calls.push({ kind: 'setBpm', bpm: t.bpm })
  if (!p || p.timeSignature !== t.timeSignature) {
    calls.push({
      kind: 'setTimeSignature',
      numerator: t.timeSignature.numerator,
      denominator: t.timeSignature.denominator,
    })
  }
  if (!p || p.loopEnabled !== t.loopEnabled || p.loopRegion !== t.loopRegion) {
    const looping = t.loopEnabled && t.loopRegion !== null
    calls.push({
      kind: 'setLoop',
      startBeat: looping ? t.loopRegion!.startBeat : 0,
      endBeat: looping ? t.loopRegion!.endBeat : 0,
      enabled: looping,
    })
  }
  if (!p || p.metronomeEnabled !== t.metronomeEnabled) {
    calls.push({ kind: 'setMetronome', enabled: t.metronomeEnabled })
  }
  if ((prev?.mixer.masterVolume ?? 1) !== next.mixer.masterVolume) {
    calls.push({
      kind: 'command',
      command: 'master.setVolume',
      params: { volume: next.mixer.masterVolume },
    })
  }

  return calls
}

// ---------------------------------------------------------------------------
// Running the plan
// ---------------------------------------------------------------------------

/** The part of the preload API the sync layer uses. */
export interface EngineApi {
  resetProject(): Promise<boolean>
  addTrack(track: { trackId: string; instrument: string }): Promise<boolean>
  removeTrack(trackId: string): Promise<boolean>
  clipUpdate(clip: EngineClip): Promise<boolean>
  clipRemove(clipId: string): Promise<boolean>
  dispatchCommand(cmd: { command: string; params: Record<string, unknown> }): Promise<unknown>
  setBpm(bpm: number): Promise<unknown>
  setTimeSignature(numerator: number, denominator: number): Promise<unknown>
  setLoopRegion(startBeat: number, endBeat: number, enabled: boolean): Promise<unknown>
  setMetronomeEnabled(enabled: boolean): Promise<unknown>
}

function execute(api: EngineApi, call: EngineCall): Promise<unknown> {
  switch (call.kind) {
    case 'addTrack':
      return api.addTrack({ trackId: call.trackId, instrument: call.instrument })
    case 'removeTrack':
      return api.removeTrack(call.trackId)
    case 'setClip':
      return api.clipUpdate(call.clip)
    case 'removeClip':
      return api.clipRemove(call.clipId)
    case 'command':
      return api.dispatchCommand({ command: call.command, params: call.params })
    case 'setBpm':
      return api.setBpm(call.bpm)
    case 'setTimeSignature':
      return api.setTimeSignature(call.numerator, call.denominator)
    case 'setLoop':
      return api.setLoopRegion(call.startBeat, call.endBeat, call.enabled)
    case 'setMetronome':
      return api.setMetronomeEnabled(call.enabled)
  }
}

export interface EngineSync {
  /** Reset the engine and rebuild it from the current document. */
  rebuild(): Promise<void>
  /** Resolves once every change made so far has reached the engine. */
  settled(): Promise<void>
  /** Stop following the document. */
  dispose(): void
}

export function createEngineSync(
  api: EngineApi,
  onError: (message: string) => void = (m) => console.warn(m),
): EngineSync {
  let synced: ProjectDocument | null = null
  let scheduled = false
  let inFlight: Promise<void> = Promise.resolve()

  const run = (calls: EngineCall[]): Promise<void> => {
    // Calls are issued in order without waiting on each other: IPC and the
    // engine both preserve call order, and a burst of edits (a drag) should
    // not queue up behind round trips.
    const results = calls.map((call) =>
      execute(api, call).then(
        (ok) => {
          if (ok === false) onError(`Engine refused ${describe(call)}`)
        },
        (err) => onError(`Engine call failed (${describe(call)}): ${String(err)}`),
      ),
    )
    return Promise.all(results).then(() => undefined)
  }

  const flush = () => {
    scheduled = false
    const next = getDocument()
    const calls = planEngineSync(synced, next)
    synced = next
    if (calls.length > 0) inFlight = Promise.all([inFlight, run(calls)]).then(() => undefined)
  }

  const unsubscribe = subscribeDocument(() => {
    if (scheduled) return
    scheduled = true
    // Coalesce every store update made in the same tick into one plan.
    queueMicrotask(flush)
  })

  return {
    async rebuild() {
      await inFlight
      await api.resetProject()
      synced = null
      flush()
      await inFlight
    },
    async settled() {
      if (scheduled) flush()
      await inFlight
    },
    dispose: unsubscribe,
  }
}

function describe(call: EngineCall): string {
  switch (call.kind) {
    case 'command':
      return `${call.command} ${JSON.stringify(call.params)}`
    case 'setClip':
      return `setClip ${call.clip.clipId}`
    default:
      return JSON.stringify(call)
  }
}
