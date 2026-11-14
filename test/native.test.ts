// End-to-end test of the native addon through its JavaScript API: the same
// calls the Electron main process makes. Needs the addon to be built
// (`pnpm run build:native`) and an audio output device; skipped otherwise.

import { describe, it, expect, beforeAll, afterAll } from 'vitest'
import { createRequire } from 'module'
import { existsSync, mkdtempSync, readFileSync, rmSync } from 'fs'
import { tmpdir } from 'os'
import path from 'path'

const addonPath = path.resolve(__dirname, '..', 'build', 'Release', 'calliope_addon.node')
const addonBuilt = existsSync(addonPath)

// eslint-disable-next-line @typescript-eslint/no-explicit-any
let native: any
let engineReady = false
let workDir = ''

/** Largest absolute sample value in a 16-bit PCM WAV file, as a fraction of full scale. */
function wavPeak(file: string): { peak: number; seconds: number; sampleRate: number } {
  const buf = readFileSync(file)
  expect(buf.toString('ascii', 0, 4)).toBe('RIFF')
  expect(buf.toString('ascii', 8, 12)).toBe('WAVE')

  let offset = 12
  let sampleRate = 0
  let channels = 0
  let bits = 0
  while (offset + 8 <= buf.length) {
    const id = buf.toString('ascii', offset, offset + 4)
    const size = buf.readUInt32LE(offset + 4)
    const body = offset + 8
    if (id === 'fmt ') {
      channels = buf.readUInt16LE(body + 2)
      sampleRate = buf.readUInt32LE(body + 4)
      bits = buf.readUInt16LE(body + 14)
    } else if (id === 'data') {
      expect(bits).toBe(16)
      let peak = 0
      const end = Math.min(body + size, buf.length)
      for (let i = body; i + 1 < end; i += 2) {
        const v = Math.abs(buf.readInt16LE(i))
        if (v > peak) peak = v
      }
      const frames = (end - body) / (2 * channels)
      return { peak: peak / 32768, seconds: frames / sampleRate, sampleRate }
    }
    offset = body + size + (size % 2)
  }
  throw new Error('no data chunk in ' + file)
}

describe.skipIf(!addonBuilt)('native addon', () => {
  beforeAll(async () => {
    native = createRequire(import.meta.url)(addonPath)
    workDir = mkdtempSync(path.join(tmpdir(), 'calliope-native-'))
    engineReady = await native.initialiseEngine(44100, 512)
  })

  afterAll(async () => {
    if (native) await native.shutdownEngine()
    if (workDir) rmSync(workDir, { recursive: true, force: true })
  })

  it('reports the JUCE version and audio devices', async () => {
    const info = await native.getEngineInfo()
    expect(info.juceVersion).toMatch(/JUCE v\d+\.\d+/)
    expect(Array.isArray(info.audioDevices)).toBe(true)
  })

  it('refuses work on tracks that do not exist', async (ctx) => {
    if (!engineReady) return ctx.skip()
    expect(await native.removeTrack('nope')).toBe(false)
    expect(
      await native.dispatchCommand({
        command: 'effect.insert',
        params: { trackId: 'nope', effectType: 'reverb' },
      }),
    ).toBe(false)
    expect(
      await native.dispatchCommand({
        command: 'parameter.set',
        params: { id: 'track.nope.volume', value: 0.5 },
      }),
    ).toBe(false)
  })

  it('plays a clip on a track created at runtime and exports it', async (ctx) => {
    if (!engineReady) return ctx.skip()
    await native.resetProject()

    // Track IDs are opaque to the engine; the UI uses UUIDs.
    const trackId = '3f0c1c1e-8a53-4c3e-9d57-1f6f1b0b2f10'
    expect(await native.addTrack({ trackId, instrument: 'polysynth' })).toBe(true)
    expect(await native.addTrack({ trackId, instrument: 'polysynth' })).toBe(false)

    const ids: string[] = await native.getParameterIds()
    expect(ids).toContain(`track.${trackId}.volume`)
    expect(ids).toContain(`track.${trackId}.filterCutoff`)

    expect(
      await native.addClip({
        clipId: 'clip-1',
        trackId,
        startBeat: 0,
        lengthBeats: 4,
        // Velocity as the piano roll stores it: MIDI 1-127.
        notes: [
          { pitch: 60, startBeat: 0, lengthBeats: 1, velocity: 100 },
          { pitch: 64, startBeat: 1, lengthBeats: 1, velocity: 100 },
        ],
      }),
    ).toBe(true)

    const outputPath = path.join(workDir, 'mix.wav')
    const progress: number[] = []
    const ok = await native.exportAudio({ outputPath, format: 'wav16' }, (p: number) =>
      progress.push(p),
    )
    expect(ok).toBe(true)

    const { peak, seconds } = wavPeak(outputPath)
    expect(peak).toBeGreaterThan(0.01)
    // 4 beats at 120 BPM, plus the 2 second tail.
    expect(seconds).toBeCloseTo(4, 1)

    // Progress callbacks are delivered asynchronously; let them drain.
    await new Promise((r) => setTimeout(r, 50))
    expect(progress.length).toBeGreaterThan(0)
    expect(Math.max(...progress)).toBeLessThanOrEqual(1)
  })

  it('applies a muted track to the exported audio', async (ctx) => {
    if (!engineReady) return ctx.skip()
    await native.resetProject()
    expect(await native.addTrack({ trackId: 't', instrument: 'basssynth' })).toBe(true)
    await native.addClip({
      clipId: 'c',
      trackId: 't',
      startBeat: 0,
      lengthBeats: 2,
      notes: [{ pitch: 40, startBeat: 0, lengthBeats: 2, velocity: 110 }],
    })
    expect(
      await native.dispatchCommand({
        command: 'parameter.set',
        params: { id: 'track.t.mute', value: true },
      }),
    ).toBe(true)

    const outputPath = path.join(workDir, 'muted.wav')
    expect(await native.exportAudio({ outputPath, format: 'wav16' })).toBe(true)
    expect(wavPeak(outputPath).peak).toBe(0)
  })

  it('applies calls in the order they are made', async (ctx) => {
    if (!engineReady) return ctx.skip()
    await native.resetProject()
    await native.addTrack({ trackId: 't', instrument: 'polysynth' })

    // Fire a burst without awaiting, as a fader drag does. The last value must win.
    const pending: Promise<boolean>[] = []
    for (let i = 0; i <= 100; i++) {
      pending.push(
        native.dispatchCommand({
          command: 'parameter.set',
          params: { id: 'track.t.volume', value: i / 100 },
        }),
      )
    }
    await Promise.all(pending)

    const described: Array<{ id: string; value: number; min: number; max: number; type: string }> =
      await native.describeParameters()
    const volume = described.find((p) => p.id === 'track.t.volume')
    expect(volume).toBeDefined()
    expect(volume!.value).toBeCloseTo(1, 5)
    expect(volume!.min).toBe(0)
    expect(volume!.max).toBe(2)
    expect(volume!.type).toBe('float')
  })

  it('clamps parameter values to their range', async (ctx) => {
    if (!engineReady) return ctx.skip()
    await native.resetProject()
    await native.addTrack({ trackId: 't', instrument: 'polysynth' })
    await native.dispatchCommand({
      command: 'parameter.set',
      params: { id: 'track.t.filterCutoff', value: 1e12 },
    })
    const described: Array<{ id: string; value: number }> = await native.describeParameters()
    expect(described.find((p) => p.id === 'track.t.filterCutoff')!.value).toBe(20000)
  })

  it('saves and restores the whole project as JSON', async (ctx) => {
    if (!engineReady) return ctx.skip()
    await native.resetProject()
    await native.addTrack({ trackId: 'lead', instrument: 'polysynth' })
    await native.addTrack({ trackId: 'drums', instrument: 'drumMachine' })
    await native.dispatchCommand({
      command: 'effect.insert',
      params: { trackId: 'lead', effectType: 'delay' },
    })
    await native.dispatchCommand({
      command: 'parameter.set',
      params: { id: 'effects.lead.0.feedback', value: 0.6 },
    })
    await native.setBpm(97)
    await native.addClip({
      clipId: 'c',
      trackId: 'drums',
      startBeat: 0,
      lengthBeats: 4,
      notes: [{ pitch: 36, startBeat: 0, lengthBeats: 0.25, velocity: 127 }],
    })

    const saved: string = await native.getProjectState()
    const parsed = JSON.parse(saved)
    expect(parsed.tracks.map((t: { id: string }) => t.id)).toEqual(['lead', 'drums'])
    expect(parsed.clips).toHaveLength(1)
    expect(parsed.transport.bpm).toBe(97)

    await native.resetProject()
    expect(JSON.parse(await native.getProjectState()).tracks).toHaveLength(0)

    expect(await native.loadProjectState(saved)).toBe(true)
    expect(await native.getProjectState()).toBe(saved)
    expect(await native.loadProjectState('not json')).toBe(false)
  })

  it('reports meter levels per track', async (ctx) => {
    if (!engineReady) return ctx.skip()
    await native.resetProject()
    await native.addTrack({ trackId: 'a', instrument: 'polysynth' })
    const levels = await native.getMeterLevels()
    expect(Object.keys(levels).sort()).toEqual(['a', 'master'])
    expect(levels.a).toHaveProperty('peakLeft')
  })

  it('produces sound on the live device path when the transport plays', async (ctx) => {
    if (!engineReady) return ctx.skip()
    await native.resetProject()
    await native.setMetronomeEnabled(false)
    await native.addTrack({ trackId: 'live', instrument: 'polysynth' })
    await native.addClip({
      clipId: 'c',
      trackId: 'live',
      startBeat: 0,
      lengthBeats: 4,
      notes: [{ pitch: 60, startBeat: 0, lengthBeats: 4, velocity: 110 }],
    })

    await native.transportPlay()
    let trackPeak = 0
    let masterPeak = 0
    for (let i = 0; i < 40; i++) {
      await new Promise((r) => setTimeout(r, 25))
      const levels = await native.getMeterLevels()
      trackPeak = Math.max(trackPeak, levels.live?.peakLeft ?? 0)
      masterPeak = Math.max(masterPeak, levels.master.peakLeft)
    }
    const position = (await native.getTransportState()).ppqPosition
    await native.transportStop()

    expect(position).toBeGreaterThan(0.5)
    expect(trackPeak).toBeGreaterThan(0.01)
    expect(masterPeak).toBeGreaterThan(0.01)
  })

  it('reports which export formats this build supports', () => {
    const formats = native.getExportFormats()
    expect(formats.wav16).toBe(true)
    expect(formats.wav24).toBe(true)
    expect(formats.flac).toBe(true)
    expect(typeof formats.mp3).toBe('boolean')
  })
})
