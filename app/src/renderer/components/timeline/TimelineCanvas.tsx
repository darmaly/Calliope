import { useEffect, useRef, useState, useCallback } from 'react'
import { Application, extend, useApplication } from '@pixi/react'
import { Viewport } from 'pixi-viewport'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { GridLayer } from './GridLayer'

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

interface TimelineCanvasProps {
  containerRef: React.RefObject<HTMLDivElement | null>
}

/**
 * Inner component rendered inside <Application> so useApplication() works.
 */
function ViewportContent({ containerRef }: TimelineCanvasProps) {
  const app = useApplication()
  const viewportRef = useRef<Viewport | null>(null)

  const { tracks, pixelsPerBeat, scrollX } = useTimelineStore(
    useShallow((s) => ({
      tracks: s.tracks,
      pixelsPerBeat: s.pixelsPerBeat,
      scrollX: s.scrollX,
    }))
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
        // Ctrl/Cmd + wheel: horizontal zoom
        const factor = e.deltaY > 0 ? 1 / 1.1 : 1.1
        const next = Math.min(Math.max(ppb * factor, 6), 192)
        setPixelsPerBeat(next)
      } else if (e.shiftKey) {
        // Shift + wheel: horizontal scroll
        setScrollX(Math.max(0, sx + e.deltaY))
      } else {
        // Plain wheel: vertical scroll
        setScrollY(Math.max(0, sy + e.deltaY))
      }
    }

    el.addEventListener('wheel', handleWheel, { passive: false })
    return () => el.removeEventListener('wheel', handleWheel)
  }, [containerRef, setScrollX, setScrollY, setPixelsPerBeat])

  return (
    <viewport
      ref={(ref: Viewport | null) => { viewportRef.current = ref }}
      screenWidth={size.width}
      screenHeight={size.height}
      worldWidth={WORLD_WIDTH}
      worldHeight={worldHeight}
      events={app.renderer.events}
      passiveWheel={false}
    >
      <GridLayer
        viewportWidth={size.width}
        viewportHeight={size.height}
        scrollX={scrollX}
      />
    </viewport>
  )
}

export function TimelineCanvas({ containerRef }: TimelineCanvasProps) {
  return (
    <Application
      resizeTo={containerRef}
      background="#1a1a2e"
      antialias
    >
      <ViewportContent containerRef={containerRef} />
    </Application>
  )
}
