import { useEffect, useRef, useState, useCallback } from 'react'
import { Application, extend, useApplication } from '@pixi/react'
import { Viewport } from 'pixi-viewport'
import type { Graphics } from 'pixi.js'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { GridLayer } from './GridLayer'
import { ClipContainer } from './ClipContainer'
import { SelectionBox } from './SelectionBox'
import { LoopRegion } from './LoopRegion'
import { pixelToBeat, snapToBeat, beatToPixel } from '../../utils/beat-math'
import { createClip, deleteClip, moveClip, resizeClip, duplicateClip } from '../../utils/clip-operations'
import { usePianoRollStore } from '../../stores/piano-roll-store'
import { useContextMenu } from '../../hooks/use-context-menu'
import { ContextMenu } from '../shared/ContextMenu'

// Import pixi-setup to register base PixiJS components
import './pixi-setup'

// Register Viewport with @pixi/react
extend({ Viewport })

// Declare pixi-viewport JSX types for @pixi/react
declare module '@pixi/react' {
  interface PixiElements {
    viewport: import('@pixi/react').PixiReactElementProps<typeof Viewport>
  }
}

const TRACK_HEIGHT = 80
const WORLD_WIDTH = 10000
const EDGE_THRESHOLD = 6

type DragMode = 'none' | 'create' | 'select' | 'move' | 'resize-left' | 'resize-right'

interface DragState {
  mode: DragMode
  startX: number
  startY: number
  currentX: number
  currentY: number
  clipId?: string
  trackId?: string
  /** Original clip positions for move operations */
  originals?: Record<string, { startBeat: number; trackId: string }>
}

interface TimelineCanvasProps {
  containerRef: React.RefObject<HTMLDivElement | null>
}

/**
 * Inner component rendered inside <Application> — only PixiJS rendering, no DOM interactions.
 */
function CanvasContent({ containerRef }: TimelineCanvasProps) {
  const app = useApplication()
  const viewportRef = useRef<Viewport | null>(null)

  const { tracks, clips, pixelsPerBeat, scrollX, scrollY, gridResolution, snapEnabled, selectedClipIds, currentBeat } =
    useTimelineStore(
      useShallow((s) => ({
        tracks: s.tracks,
        clips: s.clips,
        pixelsPerBeat: s.pixelsPerBeat,
        scrollX: s.scrollX,
        scrollY: s.scrollY,
        gridResolution: s.gridResolution,
        snapEnabled: s.snapEnabled,
        selectedClipIds: s.selectedClipIds,
        currentBeat: s.currentBeat,
      })),
    )

  const setScrollX = useTimelineStore((s) => s.setScrollX)
  const setScrollY = useTimelineStore((s) => s.setScrollY)
  const setPixelsPerBeat = useTimelineStore((s) => s.setPixelsPerBeat)

  const [size, setSize] = useState({ width: 800, height: 600 })

  // Observe container size
  useEffect(() => {
    const el = containerRef.current
    if (!el) return

    const observer = new ResizeObserver((entries) => {
      const entry = entries[0]
      if (entry) {
        const w = entry.contentRect.width
        const h = entry.contentRect.height
        setSize({ width: w, height: h })
        // Force PixiJS renderer to resize to match container
        const pixiApp = (app as any)?.app ?? app
        if (pixiApp?.renderer) {
          pixiApp.renderer.resize(w, h)
        }
      }
    })
    observer.observe(el)
    return () => observer.disconnect()
  }, [containerRef, app])

  const worldHeight = Math.max(tracks.length * TRACK_HEIGHT, size.height)

  // Handle wheel events manually for scroll/zoom
  useEffect(() => {
    const el = containerRef.current
    if (!el) return

    const handleWheel = (e: WheelEvent) => {
      e.preventDefault()

      const state = useTimelineStore.getState()
      const ppb = state.pixelsPerBeat
      const sx = state.scrollX
      const sy = state.scrollY

      if (e.ctrlKey || e.metaKey) {
        // Zoom
        const factor = e.deltaY > 0 ? 1 / 1.1 : 1.1
        const next = Math.min(Math.max(ppb * factor, 6), 192)
        setPixelsPerBeat(next)
      } else {
        // Scroll — deltaX for horizontal (trackpad swipe), deltaY for vertical
        // Also: shift+scroll = horizontal scroll (for mouse wheel users)
        const dx = e.shiftKey ? e.deltaY : e.deltaX
        const dy = e.shiftKey ? 0 : e.deltaY

        // Horizontal scroll: clamp to 0..maxScrollX
        // maxScrollX = rightmost clip end (rounded up to nearest bar) * ppb
        const clips = Object.values(state.clips)
        let maxBeat = 32 // minimum 32 beats visible
        for (const clip of clips) {
          const clipEnd = clip.startBeat + clip.lengthBeats
          if (clipEnd > maxBeat) maxBeat = clipEnd
        }
        const beatsPerBar = state.timeSignature.numerator
        maxBeat = Math.ceil(maxBeat / beatsPerBar) * beatsPerBar + beatsPerBar * 2 // 2 bars padding
        const maxScrollX = Math.max(0, maxBeat * ppb - (el?.clientWidth ?? 800))
        setScrollX(Math.max(0, Math.min(maxScrollX, sx + dx)))

        // Vertical scroll: clamp to 0..maxScrollY
        const maxScrollY = Math.max(0, state.tracks.length * 80 - (el?.clientHeight ?? 600))
        setScrollY(Math.max(0, Math.min(maxScrollY, sy + dy)))
      }
    }

    el.addEventListener('wheel', handleWheel, { passive: false })
    return () => el.removeEventListener('wheel', handleWheel)
  }, [containerRef, setScrollX, setScrollY, setPixelsPerBeat])

  // Imperatively sync scroll position to avoid React reconciler lag
  const scrollContainerRef = useRef<import('pixi.js').Container | null>(null)
  useEffect(() => {
    const unsub = useTimelineStore.subscribe((state) => {
      if (scrollContainerRef.current) {
        scrollContainerRef.current.x = -state.scrollX
        scrollContainerRef.current.y = -state.scrollY
      }
    })
    return unsub
  }, [])

  return (
    <>
      <pixiContainer ref={(ref: import('pixi.js').Container | null) => { scrollContainerRef.current = ref }} x={-scrollX} y={-scrollY}>
        <GridLayer viewportWidth={size.width} viewportHeight={size.height} scrollX={scrollX} />

        {/* Clip containers per track */}
        {tracks.map((track, index) => (
          <ClipContainer key={track.id} track={track} trackIndex={index} />
        ))}

        {/* Loop region */}
        <LoopRegion worldHeight={worldHeight} />

        {/* Selection rubber-band box — disabled for now, handled by DOM overlay */}
        <SelectionBox
          startX={0}
          startY={0}
          endX={0}
          endY={0}
          visible={false}
        />
      </pixiContainer>

    </>
  )
}

export function TimelineCanvas({ containerRef }: TimelineCanvasProps) {
  const contextMenu = useContextMenu()
  const [drag, setDrag] = useState<DragState>({
    mode: 'none',
    startX: 0,
    startY: 0,
    currentX: 0,
    currentY: 0,
  })
  const [ghost, setGhost] = useState<{ x: number; y: number; width: number; height: number } | null>(null)

  // Double-click detection for opening MIDI clips in piano roll
  const lastClickRef = useRef<{ time: number; x: number; y: number }>({ time: 0, x: 0, y: 0 })

  const selectClip = useTimelineStore((s) => s.selectClip)
  const toggleClipSelection = useTimelineStore((s) => s.toggleClipSelection)
  const selectClipsInRect = useTimelineStore((s) => s.selectClipsInRect)
  const clearSelection = useTimelineStore((s) => s.clearSelection)

  const findClipAt = useCallback(
    (px: number, py: number) => {
      const state = useTimelineStore.getState()
      const trackIndex = Math.floor((py + state.scrollY) / TRACK_HEIGHT)
      if (trackIndex < 0 || trackIndex >= state.tracks.length) return null

      const track = state.tracks[trackIndex]
      const beat = pixelToBeat(px, state.pixelsPerBeat, state.scrollX)

      for (const clip of Object.values(state.clips)) {
        if (clip.trackId !== track.id) continue
        if (beat >= clip.startBeat && beat <= clip.startBeat + clip.lengthBeats) {
          const leftEdgePx = beatToPixel(clip.startBeat, state.pixelsPerBeat, state.scrollX)
          const rightEdgePx = beatToPixel(clip.startBeat + clip.lengthBeats, state.pixelsPerBeat, state.scrollX)

          let edge: 'left' | 'right' | 'body' = 'body'
          if (Math.abs(px - leftEdgePx) <= EDGE_THRESHOLD) edge = 'left'
          else if (Math.abs(px - rightEdgePx) <= EDGE_THRESHOLD) edge = 'right'

          return { clip, track, edge }
        }
      }
      return null
    },
    [],
  )

  const handlePointerDown = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      if (e.button === 2) return
      const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
      const px = e.clientX - rect.left
      const py = e.clientY - rect.top
      const hit = findClipAt(px, py)
      const state = useTimelineStore.getState()

      if (hit) {
        const { clip, edge } = hit
        if (e.shiftKey) {
          selectClip(clip.id, true)
        } else if (e.ctrlKey || e.metaKey) {
          toggleClipSelection(clip.id)
        } else if (!state.selectedClipIds.has(clip.id)) {
          selectClip(clip.id)
        }

        if (edge === 'left') {
          setDrag({ mode: 'resize-left', startX: px, startY: py, currentX: px, currentY: py, clipId: clip.id })
        } else if (edge === 'right') {
          setDrag({ mode: 'resize-right', startX: px, startY: py, currentX: px, currentY: py, clipId: clip.id })
        } else {
          const originals: Record<string, { startBeat: number; trackId: string }> = {}
          const selected = state.selectedClipIds.has(clip.id) ? state.selectedClipIds : new Set([clip.id])
          for (const cid of selected) {
            const c = state.clips[cid]
            if (c) originals[cid] = { startBeat: c.startBeat, trackId: c.trackId }
          }
          setDrag({ mode: 'move', startX: px, startY: py, currentX: px, currentY: py, originals })
        }
      } else {
        clearSelection()
        setDrag({ mode: 'create', startX: px, startY: py, currentX: px, currentY: py })
      }
      ;(e.currentTarget as HTMLElement).setPointerCapture(e.pointerId)
    },
    [findClipAt, selectClip, toggleClipSelection, clearSelection],
  )

  const handlePointerMove = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
      const px = e.clientX - rect.left
      const py = e.clientY - rect.top

      if (drag.mode === 'none') {
        const hit = findClipAt(px, py)
        if (hit?.edge === 'left' || hit?.edge === 'right') {
          ;(e.currentTarget as HTMLElement).style.cursor = 'col-resize'
        } else if (hit) {
          ;(e.currentTarget as HTMLElement).style.cursor = 'pointer'
        } else {
          ;(e.currentTarget as HTMLElement).style.cursor = 'crosshair'
        }
        return
      }

      setDrag((prev) => ({ ...prev, currentX: px, currentY: py }))
      const state = useTimelineStore.getState()

      if (drag.mode === 'create') {
        const trackIndex = Math.floor((drag.startY + state.scrollY) / TRACK_HEIGHT)
        if (trackIndex >= 0 && trackIndex < state.tracks.length) {
          const startBeat = pixelToBeat(Math.min(drag.startX, px), state.pixelsPerBeat, state.scrollX)
          const endBeat = pixelToBeat(Math.max(drag.startX, px), state.pixelsPerBeat, state.scrollX)
          const snappedStart = state.snapEnabled ? snapToBeat(startBeat, state.gridResolution) : startBeat
          const snappedEnd = state.snapEnabled ? snapToBeat(endBeat, state.gridResolution) : endBeat
          setGhost({
            x: beatToPixel(snappedStart, state.pixelsPerBeat, state.scrollX),
            y: trackIndex * TRACK_HEIGHT - state.scrollY,
            width: (snappedEnd - snappedStart) * state.pixelsPerBeat,
            height: TRACK_HEIGHT - 8,
          })
        }
        if (Math.abs(px - drag.startX) < 4) setGhost(null)
      }

      if (drag.mode === 'move' && drag.originals) {
        const deltaPx = px - drag.startX
        const deltaBeat = deltaPx / state.pixelsPerBeat
        const deltaTrackRows = Math.round((py - drag.startY) / TRACK_HEIGHT)
        for (const [cid, orig] of Object.entries(drag.originals)) {
          let newBeat = orig.startBeat + deltaBeat
          if (state.snapEnabled) newBeat = snapToBeat(newBeat, state.gridResolution)
          newBeat = Math.max(0, newBeat)
          const origTrackIdx = state.tracks.findIndex((t) => t.id === orig.trackId)
          const newTrackIdx = Math.max(0, Math.min(state.tracks.length - 1, origTrackIdx + deltaTrackRows))
          const newTrackId = state.tracks[newTrackIdx]?.id ?? orig.trackId
          moveClip(cid, newBeat, newTrackId)
        }
      }

      if (drag.mode === 'resize-right' && drag.clipId) {
        const clip = state.clips[drag.clipId]
        if (clip) {
          let endBeat = pixelToBeat(px, state.pixelsPerBeat, state.scrollX)
          if (state.snapEnabled) endBeat = snapToBeat(endBeat, state.gridResolution)
          resizeClip(clip.id, clip.startBeat, Math.max(state.gridResolution, endBeat - clip.startBeat))
        }
      }

      if (drag.mode === 'resize-left' && drag.clipId) {
        const clip = state.clips[drag.clipId]
        if (clip) {
          let newStart = pixelToBeat(px, state.pixelsPerBeat, state.scrollX)
          if (state.snapEnabled) newStart = snapToBeat(newStart, state.gridResolution)
          const origEnd = clip.startBeat + clip.lengthBeats
          newStart = Math.min(newStart, origEnd - state.gridResolution)
          newStart = Math.max(0, newStart)
          resizeClip(clip.id, newStart, origEnd - newStart)
        }
      }
    },
    [drag, findClipAt],
  )

  const handlePointerUp = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
      const px = e.clientX - rect.left
      const py = e.clientY - rect.top
      const state = useTimelineStore.getState()

      // Double-click detection: open MIDI clip in piano roll
      const now = Date.now()
      const last = lastClickRef.current
      const isDoubleClick =
        now - last.time < 300 &&
        Math.abs(px - last.x) < 4 &&
        Math.abs(py - last.y) < 4
      lastClickRef.current = { time: now, x: px, y: py }

      if (isDoubleClick) {
        const hit = findClipAt(px, py)
        if (hit && hit.clip.type === 'midi') {
          usePianoRollStore.getState().setActiveClip(hit.clip.id)
          setDrag({ mode: 'none', startX: 0, startY: 0, currentX: 0, currentY: 0 })
          setGhost(null)
          ;(e.currentTarget as HTMLElement).releasePointerCapture(e.pointerId)
          return
        }
      }

      if (drag.mode === 'create') {
        const dx = Math.abs(px - drag.startX)
        const trackIndex = Math.floor((drag.startY + state.scrollY) / TRACK_HEIGHT)
        if (dx >= 4 && trackIndex >= 0 && trackIndex < state.tracks.length) {
          const startBeat = pixelToBeat(Math.min(drag.startX, px), state.pixelsPerBeat, state.scrollX)
          const endBeat = pixelToBeat(Math.max(drag.startX, px), state.pixelsPerBeat, state.scrollX)
          const snappedStart = state.snapEnabled ? snapToBeat(startBeat, state.gridResolution) : startBeat
          const snappedEnd = state.snapEnabled ? snapToBeat(endBeat, state.gridResolution) : endBeat
          const length = snappedEnd - snappedStart
          if (length >= state.gridResolution) {
            const track = state.tracks[trackIndex]
            createClip(track.id, snappedStart, length, 'midi')
          }
        } else if (dx < 4) {
          const dy = Math.abs(py - drag.startY)
          if (dy >= 4 || dx >= 4) {
            const minX = Math.min(drag.startX, px)
            const maxX = Math.max(drag.startX, px)
            const minY = Math.min(drag.startY, py)
            const maxY = Math.max(drag.startY, py)
            const intersecting: string[] = []
            for (const clip of Object.values(state.clips)) {
              const clipLeft = beatToPixel(clip.startBeat, state.pixelsPerBeat, state.scrollX)
              const clipRight = beatToPixel(clip.startBeat + clip.lengthBeats, state.pixelsPerBeat, state.scrollX)
              const trackIdx = state.tracks.findIndex((t) => t.id === clip.trackId)
              const clipTop = trackIdx * TRACK_HEIGHT - state.scrollY
              const clipBottom = clipTop + TRACK_HEIGHT
              if (clipRight >= minX && clipLeft <= maxX && clipBottom >= minY && clipTop <= maxY) {
                intersecting.push(clip.id)
              }
            }
            if (intersecting.length > 0) selectClipsInRect(intersecting)
          }
        }
      }

      setDrag({ mode: 'none', startX: 0, startY: 0, currentX: 0, currentY: 0 })
      setGhost(null)
      ;(e.currentTarget as HTMLElement).releasePointerCapture(e.pointerId)
    },
    [drag, selectClipsInRect],
  )

  const handleContextMenu = useCallback(
    (e: React.MouseEvent<HTMLDivElement>) => {
      e.preventDefault()
      const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
      const px = e.clientX - rect.left
      const py = e.clientY - rect.top
      const hit = findClipAt(px, py)
      if (hit) {
        const { clip } = hit
        if (!useTimelineStore.getState().selectedClipIds.has(clip.id)) {
          selectClip(clip.id)
        }
        contextMenu.show(e, [
          { label: 'Delete Clip', action: () => deleteClip(clip.id), destructive: true },
          { label: 'Duplicate Clip', action: () => duplicateClip(clip.id) },
          {
            label: 'Split at Playhead',
            action: () => {
              const s = useTimelineStore.getState()
              const c = s.clips[clip.id]
              if (!c) return
              if (s.currentBeat > c.startBeat && s.currentBeat < c.startBeat + c.lengthBeats) {
                resizeClip(c.id, c.startBeat, s.currentBeat - c.startBeat)
                createClip(c.trackId, s.currentBeat, c.startBeat + c.lengthBeats - s.currentBeat, c.type)
              }
            },
          },
        ])
      }
    },
    [findClipAt, selectClip, contextMenu],
  )

  return (
    <div className="relative w-full h-full">
      <Application resizeTo={containerRef} background="#1a1a2e" antialias>
        <CanvasContent containerRef={containerRef} />
      </Application>

      {/* Transparent overlay for pointer events — sits on top of the PixiJS canvas */}
      <div
        className="absolute inset-0"
        style={{ zIndex: 10 }}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onContextMenu={handleContextMenu}
      />

      {/* Ghost clip preview during creation */}
      {ghost && (
        <div
          className="absolute pointer-events-none rounded border border-[#6c63ff] bg-[#6c63ff]/20"
          style={{
            left: ghost.x,
            top: ghost.y,
            width: ghost.width,
            height: ghost.height,
            zIndex: 11,
          }}
        />
      )}

      {/* Context menu */}
      {contextMenu.visible && (
        <ContextMenu items={contextMenu.items} position={contextMenu.position} onClose={contextMenu.close} />
      )}
    </div>
  )
}
