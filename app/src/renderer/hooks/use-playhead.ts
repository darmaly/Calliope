import { useEffect } from 'react'
import { useTimelineStore } from '../stores/timeline-store'

/**
 * Mirrors the engine's playback position into the timeline store, about 30
 * times a second. Tempo, time signature and loop settings are not read back:
 * they belong to the project document and flow the other way.
 */
export function usePlayhead() {
  useEffect(() => {
    let cancelled = false
    let frame = 0
    let raf = 0
    let waiting = false

    const poll = () => {
      if (cancelled) return
      frame++

      // Every 2nd frame, and never more than one request in flight.
      if (frame % 2 === 0 && !waiting && window.calliope?.getTransportState) {
        waiting = true
        window.calliope
          .getTransportState()
          .then((state) => {
            if (cancelled) return
            useTimelineStore
              .getState()
              .setPlaybackPosition(state.state === 'playing', state.ppqPosition)
          })
          .catch(() => {
            // Engine not available (e.g. renderer running without the addon)
          })
          .finally(() => {
            waiting = false
          })
      }

      raf = requestAnimationFrame(poll)
    }

    raf = requestAnimationFrame(poll)
    return () => {
      cancelled = true
      cancelAnimationFrame(raf)
    }
  }, [])
}
