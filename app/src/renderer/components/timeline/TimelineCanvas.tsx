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
 * Inner component rendered inside <Application> so useApplication() works.
 */
function ViewportContent({ containerRef }: TimelineCanvasProps) {
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
  const selectClip = useTimelineStore((s) => s.selectClip)
  const toggleClipSelection = useTimelineStore((s) => s.toggleClipSelection)
  const selectClipsInRect = useTimelineStore((s) => s.selectClipsInRect)
  const clearSelection = useTimelineStore((s) => s.clearSelection)

  const contextMenu = useContextMenu()

  const [size, setSize] = useState({ width: 800, height: 600 })
  const [drag, setDrag] = useState<DragState>({
    mode: 'none',
    startX: 0,
    startY: 0,
    currentX: 0,
    currentY: 0,
  })

  // Ghost clip for creation preview
  const [ghost, setGhost] = useState<{ x: number; y: number; width: number; height: number } | null>(null)

  // Observe container size
  useEffect(() => {
    const el = containerRef.current
    if (!el) return

    const observer = new ResizeObserver((entries) => {
      const entry = entries[0]
      if (entry) {
        setSize({
          width: entry.contentRect.width,
          height: entry.contentRect.height,
        })
      }
    })
    observer.observe(el)
    return () => observer.disconnect()
  }, [containerRef])

  const worldHeight = Math.max(tracks.length * TRACK_HEIGHT, size.height)

  // Handle wheel events manually for scroll/zoom
  useEffect(() => {
    const el = containerRef.current
    if (!el) return

    const handleWheel = (e: WheelEvent) => {
      e.preventDefault()

      const ppb = useTimelineStore.getState().pixelsPerBeat
      const sx = useTimelineStore.getState().scrollX
      const sy = useTimelineStore.getState().scrollY

      if (e.ctrlKey || e.metaKey) {
        const factor = e.deltaY > 0 ? 1 / 1.1 : 1.1
        const next = Math.min(Math.max(ppb * factor, 6), 192)
        setPixelsPerBeat(next)
      } else if (e.shiftKey) {
        setScrollX(Math.max(0, sx + e.deltaY))
      } else {
        setScrollY(Math.max(0, sy + e.deltaY))
      }
    }

    el.addEventListener('wheel', handleWheel, { passive: false })
    return () => el.removeEventListener('wheel', handleWheel)
  }, [containerRef, setScrollX, setScrollY, setPixelsPerBeat])

  // Helper: find clip at pixel position
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
          // Check edge proximity for resize
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

  // Handle pointer down on canvas overlay
  const handlePointerDown = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      if (e.button === 2) return // right-click handled by context menu

      const rect = (e.currentTarget as HTMLElement).getBoundingClientRect()
      const px = e.clientX - rect.left
      const py = e.clientY - rect.top

      const hit = findClipAt(px, py)
      const state = useTimelineStore.getState()

      if (hit) {
        const { clip, edge } = hit

        // Handle selection
        if (e.shiftKey) {
          selectClip(clip.id, true)
        } else if (e.ctrlKey || e.metaKey) {
          toggleClipSelection(clip.id)
        } else if (!state.selectedClipIds.has(clip.id)) {
          selectClip(clip.id)
        }

        if (edge === 'left') {
          setDrag({
            mode: 'resize-left',
            startX: px,
            startY: py,
            currentX: px,
            currentY: py,
            clipId: hit.clip.id,
          })
        } else if (edge === 'right') {
          setDrag({
            mode: 'resize-right',
            startX: px,
            startY: py,
            currentX: px,
            currentY: py,
            clipId: hit.clip.id,
          })
        } else {
          // Move mode — store originals for all selected clips
          const originals: Record<string, { startBeat: number; trackId: string }> = {}
          const selected = state.selectedClipIds.has(clip.id) ? state.selectedClipIds : new Set([clip.id])
          for (const cid of selected) {
            const c = state.clips[cid]
            if (c) originals[cid] = { startBeat: c.startBeat, trackId: c.trackId }
          }
          setDrag({
            mode: 'move',
            startX: px,
            startY: py,
            currentX: px,
            currentY: py,
            originals,
          })
        }
      } else {
        // Empty area — start create or rubber-band select
        clearSelection()
        setDrag({
          mode: 'create',
          startX: px,
          startY: py,
          currentX: px,
          currentY: py,
        })
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

      // Update cursor based on hover position
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
        // Show ghost clip
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

        // If drag is wide enough, switch to create mode; else it becomes rubber-band
        const dx = Math.abs(px - drag.startX)
        if (dx < 4) {
          setGhost(null)
        }
      }

      if (drag.mode === 'move' && drag.originals) {
        const deltaPx = px - drag.startX
        const deltaBeat = deltaPx / state.pixelsPerBeat
        const deltaTrackRows = Math.round(((py - drag.startY)) / TRACK_HEIGHT)

        for (const [cid, orig] of Object.entries(drag.originals)) {
          let newBeat = orig.startBeat + deltaBeat
          if (state.snapEnabled) newBeat = snapToBeat(newBeat, state.gridResolution)
          newBeat = Math.max(0, newBeat)

          const origTrackIdx = state.tracks.findIndex((t) => t.id === orig.trackId)
          let newTrackIdx = Math.max(0, Math.min(state.tracks.length - 1, origTrackIdx + deltaTrackRows))
          const newTrackId = state.tracks[newTrackIdx]?.id ?? orig.trackId

          moveClip(cid, newBeat, newTrackId)
        }
      }

      if (drag.mode === 'resize-right' && drag.clipId) {
        const clip = state.clips[drag.clipId]
        if (clip) {
          let endBeat = pixelToBeat(px, state.pixelsPerBeat, state.scrollX)
          if (state.snapEnabled) endBeat = snapToBeat(endBeat, state.gridResolution)
          const minLength = state.gridResolution
          const newLength = Math.max(minLength, endBeat - clip.startBeat)
          resizeClip(clip.id, clip.startBeat, newLength)
        }
      }

      if (drag.mode === 'resize-left' && drag.clipId) {
        const clip = state.clips[drag.clipId]
        if (clip) {
          let newStart = pixelToBeat(px, state.pixelsPerBeat, state.scrollX)
          if (state.snapEnabled) newStart = snapToBeat(newStart, state.gridResolution)
          const origEnd = clip.startBeat + clip.lengthBeats
          const minLength = state.gridResolution
          newStart = Math.min(newStart, origEnd - minLength)
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

      if (drag.mode === 'create') {
        const dx = Math.abs(px - drag.startX)
        const trackIndex = Math.floor((drag.startY + state.scrollY) / TRACK_HEIGHT)

        if (dx >= 4 && trackIndex >= 0 && trackIndex < state.tracks.length) {
          // Create clip
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
          // It was a click+drag that didn't go far enough, or a rubber-band selection
          // Check if it was a rubber-band
          const dy = Math.abs(py - drag.startY)
          if (dy >= 4 || dx >= 4) {
            // Rubber-band select
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
            if (intersecting.length > 0) {
              selectClipsInRect(intersecting)
            }
          }
        }
      }

      setDrag({ mode: 'none', startX: 0, startY: 0, currentX: 0, currentY: 0 })
      setGhost(null)
      ;(e.currentTarget as HTMLElement).releasePointerCapture(e.pointerId)
    },
    [drag, selectClipsInRect],
  )

  // Handle right-click context menu on clip
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
              const state = useTimelineStore.getState()
              const c = state.clips[clip.id]
              if (!c) return
              const playheadBeat = state.currentBeat
              if (playheadBeat > c.startBeat && playheadBeat < c.startBeat + c.lengthBeats) {
                // Resize original to end at playhead
                resizeClip(c.id, c.startBeat, playheadBeat - c.startBeat)
                // Create new clip starting at playhead
                createClip(c.trackId, playheadBeat, c.startBeat + c.lengthBeats - playheadBeat, c.type)
              }
            },
          },
        ])
      }
    },
    [findClipAt, selectClip, contextMenu],
  )

  // Handle Escape during move to cancel
  useEffect(() => {
    const handleKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape' && drag.mode === 'move' && drag.originals) {
        // Restore original positions
        for (const [cid, orig] of Object.entries(drag.originals)) {
          moveClip(cid, orig.startBeat, orig.trackId)
        }
        setDrag({ mode: 'none', startX: 0, startY: 0, currentX: 0, currentY: 0 })
      }
    }
    document.addEventListener('keydown', handleKey)
    return () => document.removeEventListener('keydown', handleKey)
  }, [drag])

  // Selection box coordinates (rubber-band)
  const showSelectionBox = drag.mode === 'create' && !ghost
  const selBoxStartX = drag.startX
  const selBoxStartY = drag.startY
  const selBoxEndX = drag.currentX
  const selBoxEndY = drag.currentY

  return (
    <>
      <viewport
        ref={(ref: Viewport | null) => {
          viewportRef.current = ref
        }}
        screenWidth={size.width}
        screenHeight={size.height}
        worldWidth={WORLD_WIDTH}
        worldHeight={worldHeight}
        events={app.renderer.events}
        passiveWheel={false}
      >
        <GridLayer viewportWidth={size.width} viewportHeight={size.height} scrollX={scrollX} />

        {/* Clip containers per track */}
        {tracks.map((track, index) => (
          <ClipContainer key={track.id} track={track} trackIndex={index} />
        ))}

        {/* Loop region */}
        <LoopRegion worldHeight={worldHeight} />

        {/* Selection rubber-band box */}
        <SelectionBox
          startX={selBoxStartX}
          startY={selBoxStartY}
          endX={selBoxEndX}
          endY={selBoxEndY}
          visible={showSelectionBox && (Math.abs(selBoxEndX - selBoxStartX) > 4 || Math.abs(selBoxEndY - selBoxStartY) > 4)}
        />
      </viewport>

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
    </>
  )
}

export function TimelineCanvas({ containerRef }: TimelineCanvasProps) {
  return (
    <Application resizeTo={containerRef} background="#1a1a2e" antialias>
      <ViewportContent containerRef={containerRef} />
    </Application>
  )
}
