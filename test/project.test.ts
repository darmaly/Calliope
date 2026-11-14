// The project document and the three things built on it: engine sync,
// undo/redo, and files.

import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest'
import { useTimelineStore } from '../app/src/renderer/stores/timeline-store'
import { useMixerStore } from '../app/src/renderer/stores/mixer-store'
import { usePianoRollStore } from '../app/src/renderer/stores/piano-roll-store'
import {
  ProjectFormatError,
  applyDocument,
  emptyDocument,
  getDocument,
  parseDocument,
  sameDocument,
  serializeDocument,
  subscribeDocument,
} from '../app/src/renderer/project/document'
import { createEngineSync, planEngineSync } from '../app/src/renderer/project/engine-sync'
import type { EngineApi, EngineCall } from '../app/src/renderer/project/engine-sync'
import { COALESCE_MS, createHistory } from '../app/src/renderer/project/history'
import { addNote } from '../app/src/renderer/utils/note-operations'
import { createClip, deleteClip, moveClip } from '../app/src/renderer/utils/clip-operations'

const timeline = () => useTimelineStore.getState()
const mixer = () => useMixerStore.getState()

function resetStores() {
  usePianoRollStore.setState({ activeClipId: null, notes: {}, selectedNoteIds: new Set() })
  applyDocument(emptyDocument())
  useTimelineStore.setState({ selectedClipIds: new Set(), selectedTrackId: null })
}

/** Plan the calls for whatever `edit` changes, starting from the current document. */
function plan(edit: () => void): EngineCall[] {
  const before = getDocument()
  edit()
  return planEngineSync(before, getDocument())
}

const commands = (calls: EngineCall[]) =>
  calls.filter((c): c is Extract<EngineCall, { kind: 'command' }> => c.kind === 'command')

const parameterSets = (calls: EngineCall[]) =>
  Object.fromEntries(
    commands(calls)
      .filter((c) => c.command === 'parameter.set')
      .map((c) => [c.params.id as string, c.params.value]),
  )

beforeEach(resetStores)

// ---------------------------------------------------------------------------
describe('project document', () => {
  it('is the same document until something in it changes', () => {
    const a = getDocument()
    expect(sameDocument(a, getDocument())).toBe(true)

    // View state is not part of the document.
    timeline().setScrollX(100)
    timeline().setPlaybackPosition(true, 3.5)
    mixer().setTrackLevel('x', { peakL: 0.5, peakR: 0.5 })
    expect(sameDocument(a, getDocument())).toBe(true)

    timeline().addTrack()
    expect(sameDocument(a, getDocument())).toBe(false)
  })

  it('notifies subscribers once per change and not for view state', () => {
    const listener = vi.fn()
    const unsubscribe = subscribeDocument(listener)

    timeline().setScrollX(40)
    expect(listener).not.toHaveBeenCalled()

    timeline().setBpm(98)
    expect(listener).toHaveBeenCalledTimes(1)

    unsubscribe()
    timeline().setBpm(99)
    expect(listener).toHaveBeenCalledTimes(1)
  })

  it('round-trips through a file', () => {
    const lead = timeline().addTrack('Lead', 2, 'polysynth')
    const drums = timeline().addTrack('Drums', 3, 'drumMachine')
    timeline().setTrackParam(lead, 'filterCutoff', 900)
    timeline().toggleMute(drums)
    const clipId = createClip(lead, 4, 8, 'midi')
    timeline().updateClipNotes(clipId, {
      n1: { id: 'n1', pitch: 60, startBeat: 0, lengthBeats: 1, velocity: 90 },
      n2: { id: 'n2', pitch: 67, startBeat: 1.5, lengthBeats: 0.5, velocity: 64 },
    })
    mixer().setTrackVolume(lead, 0.6)
    mixer().setTrackPan(drums, -0.3)
    mixer().addTrackEffect(lead, 'delay')
    mixer().setEffectParam(lead, 0, 'feedback', 0.6)
    mixer().addMasterEffect('limiter')
    mixer().bypassMasterEffect(0, true)
    timeline().setBpm(97)
    timeline().setTimeSignature(3, 4)
    timeline().setLoopRegion({ startBeat: 4, endBeat: 12 })
    timeline().setMetronomeEnabled(true)

    const saved = getDocument()
    const loaded = parseDocument(serializeDocument(saved))

    expect(loaded).toEqual(saved)
  })

  it('rejects files that are not Calliope projects', () => {
    expect(() => parseDocument('not json')).toThrow(ProjectFormatError)
    expect(() => parseDocument('{"hello":"world"}')).toThrow(ProjectFormatError)
    expect(() => parseDocument('[]')).toThrow(ProjectFormatError)
    expect(() =>
      parseDocument(JSON.stringify({ app: 'calliope', formatVersion: 999, document: {} })),
    ).toThrow(/newer version/)
  })

  it('repairs what it can in a damaged project instead of failing', () => {
    const doc = parseDocument(
      JSON.stringify({
        app: 'calliope',
        formatVersion: 2,
        document: {
          tracks: [
            { id: 't1', name: 'Lead', instrument: 'theremin', params: { filterCutoff: 1e9, bogus: 1 } },
            { id: 't1', name: 'Duplicate ID' },
            { name: 'No ID' },
          ],
          clips: {
            good: { id: 'good', trackId: 't1', type: 'midi', startBeat: -4, lengthBeats: 4, notes: [
              { pitch: 300, startBeat: 0, lengthBeats: 1, velocity: 500 },
              { pitch: 60, startBeat: 0, lengthBeats: 0 },
            ] },
            orphan: { id: 'orphan', trackId: 'missing', type: 'midi', startBeat: 0, lengthBeats: 4 },
            empty: { id: 'empty', trackId: 't1', type: 'midi', startBeat: 0, lengthBeats: 0 },
          },
          mixer: {
            trackVolumes: { t1: 50, missing: 1 },
            masterVolume: 'loud',
            masterEffects: [{ effectType: 'reverb' }, { effectType: 'flanger' }],
          },
          transport: { bpm: 100000, loopEnabled: true, loopRegion: { startBeat: 8, endBeat: 2 } },
        },
      }),
    )

    expect(doc.tracks).toHaveLength(1)
    expect(doc.tracks[0].instrument).toBe('polysynth')
    expect(doc.tracks[0].params).toEqual({ filterCutoff: 20000 })

    expect(Object.keys(doc.clips)).toEqual(['good'])
    expect(doc.clips.good.startBeat).toBe(0)
    const notes = Object.values(doc.clips.good.notes!)
    expect(notes).toHaveLength(1)
    expect(notes[0].pitch).toBe(127)
    expect(notes[0].velocity).toBe(127)

    expect(doc.mixer.trackVolumes).toEqual({ t1: 2 })
    expect(doc.mixer.masterVolume).toBe(1)
    expect(doc.mixer.masterEffects.map((e) => e.effectType)).toEqual(['reverb'])
    expect(doc.transport.bpm).toBe(300)
    // A backwards loop region is dropped, and looping with it.
    expect(doc.transport.loopRegion).toBeNull()
    expect(doc.transport.loopEnabled).toBe(false)
  })
})

// ---------------------------------------------------------------------------
describe('engine sync plan', () => {
  it('builds the whole project into a freshly reset engine', () => {
    const lead = timeline().addTrack('Lead', 0, 'polysynth')
    const clipId = createClip(lead, 0, 4, 'midi')
    timeline().updateClipNotes(clipId, {
      n1: { id: 'n1', pitch: 60, startBeat: 0, lengthBeats: 1, velocity: 100 },
    })
    timeline().setBpm(90)

    const calls = planEngineSync(null, getDocument())

    expect(calls[0]).toEqual({ kind: 'addTrack', trackId: lead, instrument: 'polysynth' })
    expect(calls).toContainEqual({
      kind: 'setClip',
      clip: {
        clipId,
        trackId: lead,
        startBeat: 0,
        lengthBeats: 4,
        notes: [{ pitch: 60, startBeat: 0, lengthBeats: 1, velocity: 100 }],
      },
    })
    expect(calls).toContainEqual({ kind: 'setBpm', bpm: 90 })
    expect(calls).toContainEqual({ kind: 'setMetronome', enabled: false })
    // Nothing is sent for settings that are still at the engine's defaults.
    expect(parameterSets(calls)).toEqual({})
  })

  it('plans nothing when nothing changed', () => {
    timeline().addTrack()
    expect(plan(() => {})).toEqual([])
    expect(plan(() => timeline().setScrollX(50))).toEqual([])
  })

  it('creates the track before anything that refers to it', () => {
    const calls = plan(() => {
      const id = timeline().addTrack('Bass', 0, 'basssynth')
      mixer().setTrackVolume(id, 0.5)
      timeline().setTrackParam(id, 'subOscMix', 0.9)
      createClip(id, 0, 4, 'midi')
    })

    const kinds = calls.map((c) => c.kind)
    expect(kinds[0]).toBe('addTrack')
    expect(kinds.indexOf('setClip')).toBeGreaterThan(0)
    const id = timeline().tracks[0].id
    expect(parameterSets(calls)).toEqual({
      [`track.${id}.volume`]: 0.5,
      [`track.${id}.subOscMix`]: 0.9,
    })
  })

  it('sends only what changed for an existing track', () => {
    const id = timeline().addTrack()
    const clipA = createClip(id, 0, 4, 'midi')
    createClip(id, 4, 4, 'midi')

    expect(plan(() => mixer().setTrackPan(id, -0.5))).toEqual([
      { kind: 'command', command: 'parameter.set', params: { id: `track.${id}.pan`, value: -0.5 } },
    ])
    expect(plan(() => timeline().toggleMute(id))).toEqual([
      { kind: 'command', command: 'parameter.set', params: { id: `track.${id}.mute`, value: true } },
    ])
    expect(plan(() => timeline().toggleSolo(id))).toEqual([
      { kind: 'command', command: 'parameter.set', params: { id: `track.${id}.solo`, value: true } },
    ])

    // Moving one clip resends that clip and leaves the other alone.
    const moved = plan(() => moveClip(clipA, 8))
    expect(moved).toHaveLength(1)
    expect(moved[0]).toMatchObject({ kind: 'setClip', clip: { clipId: clipA, startBeat: 8 } })

    expect(plan(() => deleteClip(clipA))).toEqual([{ kind: 'removeClip', clipId: clipA }])
  })

  it('sends a note drawn in the piano roll to the engine straight away', () => {
    const id = timeline().addTrack()
    const clipId = createClip(id, 0, 4, 'midi')
    usePianoRollStore.getState().setActiveClip(clipId)

    const calls = plan(() => addNote(64, 1, 0.5, 80))

    expect(calls).toHaveLength(1)
    expect(calls[0]).toMatchObject({
      kind: 'setClip',
      clip: { clipId, notes: [{ pitch: 64, startBeat: 1, lengthBeats: 0.5, velocity: 80 }] },
    })
  })

  it('removes a deleted track and does not separately remove its clips', () => {
    const keep = timeline().addTrack()
    const drop = timeline().addTrack()
    createClip(drop, 0, 4, 'midi')
    createClip(keep, 0, 4, 'midi')

    const calls = plan(() => timeline().removeTrack(drop))

    expect(calls).toEqual([{ kind: 'removeTrack', trackId: drop }])
  })

  it('rebuilds a track whose instrument changed, with its clips and settings', () => {
    const id = timeline().addTrack('Part', 0, 'polysynth')
    createClip(id, 0, 4, 'midi')
    mixer().setTrackVolume(id, 0.4)

    const calls = plan(() =>
      useTimelineStore.setState({
        tracks: timeline().tracks.map((t) => (t.id === id ? { ...t, instrument: 'basssynth', params: {} } : t)),
      }),
    )

    expect(calls.map((c) => c.kind).slice(0, 2)).toEqual(['removeTrack', 'addTrack'])
    expect(calls[1]).toMatchObject({ instrument: 'basssynth' })
    expect(parameterSets(calls)).toEqual({ [`track.${id}.volume`]: 0.4 })
    expect(calls.some((c) => c.kind === 'setClip')).toBe(true)
  })

  it('inserts an effect and then sets only its non-default parameters', () => {
    const id = timeline().addTrack()

    const inserted = plan(() => mixer().addTrackEffect(id, 'reverb'))
    expect(inserted).toEqual([
      { kind: 'command', command: 'effect.insert', params: { trackId: id, effectType: 'reverb', position: -1 } },
    ])

    const tweaked = plan(() => mixer().setEffectParam(id, 0, 'roomSize', 0.9))
    expect(tweaked).toEqual([
      { kind: 'command', command: 'parameter.set', params: { id: `effects.${id}.0.roomSize`, value: 0.9 } },
    ])

    const bypassed = plan(() => mixer().bypassTrackEffect(id, 0, true))
    expect(bypassed).toEqual([
      { kind: 'command', command: 'effect.bypass', params: { trackId: id, position: 0, bypassed: true } },
    ])
  })

  it('sends boolean effect parameters as booleans', () => {
    mixer().addMasterEffect('delay')
    const calls = plan(() => mixer().setEffectParam('master', 0, 'pingPongEnabled', 1))
    expect(parameterSets(calls)).toEqual({ 'effects.master.0.pingPongEnabled': true })
  })

  it('rebuilds a reordered chain so slot numbers and settings stay aligned', () => {
    mixer().addMasterEffect('eq')
    mixer().addMasterEffect('limiter')
    mixer().setEffectParam('master', 1, 'threshold', -6)
    mixer().bypassMasterEffect(0, true)

    const calls = plan(() => mixer().reorderMasterEffect(0, 1))

    expect(commands(calls).map((c) => [c.command, c.params.position ?? c.params.id])).toEqual([
      // Removed from the end backwards...
      ['effect.remove', 1],
      ['effect.remove', 0],
      // ...then rebuilt in the new order, each with its own settings.
      ['effect.insert', -1],
      ['parameter.set', 'effects.master.0.threshold'],
      ['effect.insert', -1],
      ['effect.bypass', 1],
    ])
  })

  it('turns looping off in the engine when the region is cleared or disabled', () => {
    expect(plan(() => timeline().setLoopRegion({ startBeat: 4, endBeat: 8 }))).toEqual([
      { kind: 'setLoop', startBeat: 4, endBeat: 8, enabled: true },
    ])
    expect(plan(() => timeline().setLoopEnabled(false))).toEqual([
      { kind: 'setLoop', startBeat: 0, endBeat: 0, enabled: false },
    ])
    expect(plan(() => timeline().setLoopEnabled(true))).toEqual([
      { kind: 'setLoop', startBeat: 4, endBeat: 8, enabled: true },
    ])
  })

  it('does not send audio clips, which the engine cannot play yet', () => {
    const id = timeline().addTrack()
    expect(plan(() => createClip(id, 0, 4, 'audio'))).toEqual([])
  })
})

// ---------------------------------------------------------------------------
describe('engine sync runner', () => {
  function fakeEngine() {
    const log: string[] = []
    const record =
      (name: string) =>
      async (...args: unknown[]) => {
        log.push(`${name} ${JSON.stringify(args)}`)
        return true
      }
    const api: EngineApi = {
      resetProject: record('resetProject'),
      addTrack: record('addTrack'),
      removeTrack: record('removeTrack'),
      clipUpdate: record('clipUpdate'),
      clipRemove: record('clipRemove'),
      dispatchCommand: record('dispatchCommand'),
      setBpm: record('setBpm'),
      setTimeSignature: record('setTimeSignature'),
      setLoopRegion: record('setLoopRegion'),
      setMetronomeEnabled: record('setMetronomeEnabled'),
    }
    return { api, log }
  }

  it('resets the engine and rebuilds it from the document', async () => {
    const id = timeline().addTrack('Lead', 0, 'polysynth')
    const { api, log } = fakeEngine()
    const sync = createEngineSync(api)

    await sync.rebuild()

    expect(log[0]).toBe('resetProject []')
    expect(log[1]).toBe(`addTrack [{"trackId":"${id}","instrument":"polysynth"}]`)
    sync.dispose()
  })

  it('follows document changes, coalescing edits made in the same tick', async () => {
    const { api, log } = fakeEngine()
    const sync = createEngineSync(api)
    await sync.rebuild()
    log.length = 0

    const id = timeline().addTrack()
    mixer().setTrackVolume(id, 0.1)
    mixer().setTrackVolume(id, 0.2)
    mixer().setTrackVolume(id, 0.3)
    await sync.settled()

    expect(log).toEqual([
      `addTrack [{"trackId":"${id}","instrument":"polysynth"}]`,
      `dispatchCommand [{"command":"parameter.set","params":{"id":"track.${id}.volume","value":0.3}}]`,
    ])
    sync.dispose()
  })

  it('reports calls the engine refuses', async () => {
    const { api } = fakeEngine()
    api.addTrack = async () => false
    const errors: string[] = []
    const sync = createEngineSync(api, (m) => errors.push(m))
    await sync.rebuild()

    timeline().addTrack()
    await sync.settled()

    expect(errors).toHaveLength(1)
    expect(errors[0]).toMatch(/refused/)
    sync.dispose()
  })

  it('stops following after dispose', async () => {
    const { api, log } = fakeEngine()
    const sync = createEngineSync(api)
    await sync.rebuild()
    sync.dispose()
    log.length = 0

    timeline().addTrack()
    await Promise.resolve()

    expect(log).toEqual([])
  })
})

// ---------------------------------------------------------------------------
describe('undo and redo', () => {
  beforeEach(() => vi.useFakeTimers())
  afterEach(() => vi.useRealTimers())

  const settle = () => vi.advanceTimersByTime(COALESCE_MS + 1)

  it('undoes and redoes a change', () => {
    const history = createHistory()
    timeline().addTrack('Lead')
    settle()

    expect(history.canUndo()).toBe(true)
    expect(history.undo()).toBe(true)
    expect(timeline().tracks).toHaveLength(0)
    expect(history.canRedo()).toBe(true)

    expect(history.redo()).toBe(true)
    expect(timeline().tracks.map((t) => t.name)).toEqual(['Lead'])
    history.dispose()
  })

  it('has nothing to undo in a fresh project', () => {
    const history = createHistory()
    expect(history.canUndo()).toBe(false)
    expect(history.undo()).toBe(false)
    expect(history.redo()).toBe(false)
    history.dispose()
  })

  it('treats a burst of changes as one step and separate changes as separate steps', () => {
    const id = timeline().addTrack()
    const history = createHistory()

    // A fader drag: many values in quick succession.
    for (const v of [0.9, 0.8, 0.7, 0.6]) {
      mixer().setTrackVolume(id, v)
      vi.advanceTimersByTime(20)
    }
    settle()
    mixer().setTrackPan(id, 0.5)
    settle()

    history.undo()
    expect(mixer().getTrackPan(id)).toBe(0)
    expect(mixer().getTrackVolume(id)).toBe(0.6)

    history.undo()
    expect(mixer().getTrackVolume(id)).toBe(1)
    expect(history.canUndo()).toBe(false)
    history.dispose()
  })

  it('undoes a change that has not been grouped yet', () => {
    const history = createHistory()
    timeline().addTrack()
    // No timer advance: undo immediately.
    expect(history.undo()).toBe(true)
    expect(timeline().tracks).toHaveLength(0)
    history.dispose()
  })

  it('covers notes, effects and tempo with the same mechanism', () => {
    const id = timeline().addTrack()
    const clipId = createClip(id, 0, 4, 'midi')
    usePianoRollStore.getState().setActiveClip(clipId)
    const history = createHistory()

    addNote(60, 0, 1)
    settle()
    mixer().addTrackEffect(id, 'reverb')
    settle()
    timeline().setBpm(140)
    settle()

    history.undo()
    expect(timeline().bpm).toBe(120)
    history.undo()
    expect(mixer().getTrackEffects(id)).toEqual([])
    history.undo()
    expect(Object.keys(timeline().clips[clipId].notes ?? {})).toHaveLength(0)
    // The open piano roll shows the undone state too.
    expect(Object.keys(usePianoRollStore.getState().notes)).toHaveLength(0)
    history.dispose()
  })

  it('drops the redo stack when a new change is made', () => {
    const history = createHistory()
    timeline().addTrack('A')
    settle()
    history.undo()
    timeline().addTrack('B')
    settle()

    expect(history.canRedo()).toBe(false)
    expect(history.redo()).toBe(false)
    expect(timeline().tracks.map((t) => t.name)).toEqual(['B'])
    history.dispose()
  })

  it('records a transaction as exactly one step however long it takes', async () => {
    const history = createHistory()

    await history.transaction(async () => {
      const id = timeline().addTrack('Drums', 0, 'drumMachine')
      vi.advanceTimersByTime(COALESCE_MS * 5)
      createClip(id, 0, 4, 'midi')
      vi.advanceTimersByTime(COALESCE_MS * 5)
      timeline().setBpm(128)
    })

    expect(history.undo()).toBe(true)
    expect(timeline().tracks).toHaveLength(0)
    expect(timeline().clips).toEqual({})
    expect(timeline().bpm).toBe(120)
    expect(history.canUndo()).toBe(false)
    history.dispose()
  })

  it('forgets everything on clear', () => {
    const history = createHistory()
    timeline().addTrack()
    settle()
    history.clear()
    expect(history.canUndo()).toBe(false)
    expect(history.undo()).toBe(false)
    expect(timeline().tracks).toHaveLength(1)
    history.dispose()
  })
})

// ---------------------------------------------------------------------------
describe('piano roll and timeline stay in step', () => {
  it('writes piano roll edits through to the clip', () => {
    const id = timeline().addTrack()
    const clipId = createClip(id, 0, 4, 'midi')
    usePianoRollStore.getState().setActiveClip(clipId)

    const noteId = addNote(72, 2, 1, 110)

    expect(timeline().clips[clipId].notes?.[noteId]).toMatchObject({ pitch: 72, startBeat: 2 })
  })

  it('closes the piano roll when its clip is deleted', () => {
    const id = timeline().addTrack()
    const clipId = createClip(id, 0, 4, 'midi')
    usePianoRollStore.getState().setActiveClip(clipId)

    deleteClip(clipId)

    expect(usePianoRollStore.getState().activeClipId).toBeNull()
  })

  it('shows notes written to the open clip from outside the piano roll', () => {
    const id = timeline().addTrack()
    const clipId = createClip(id, 0, 4, 'midi')
    usePianoRollStore.getState().setActiveClip(clipId)

    timeline().updateClipNotes(clipId, {
      x: { id: 'x', pitch: 50, startBeat: 0, lengthBeats: 1, velocity: 100 },
    })

    expect(usePianoRollStore.getState().notes).toHaveProperty('x')
  })
})
