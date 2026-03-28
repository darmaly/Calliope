import { useEffect, useRef } from 'react'
import { useTimelineStore } from '../stores/timeline-store'

/**
 * Polls the C++ engine transport state via requestAnimationFrame
 * and mirrors position/BPM/time-signature into the Zustand store.
 * Throttled to ~30 Hz (every 2nd frame) to reduce IPC overhead.
 */
export function usePlayhead() {
  const frameRef = useRef(0)
  const rafRef = useRef<number | null>(null)
  const setTransportState = useTimelineStore((s) => s.setTransportState)

  useEffect(() => {
    let cancelled = false

    const poll = () => {
      if (cancelled) return

      frameRef.current++

      // Throttle: only query engine every 2nd frame (~30 Hz at 60fps)
      if (frameRef.current % 2 === 0 && window.calliope?.getTransportState) {
        window.calliope.getTransportState()
          .then((state) => {
            if (cancelled) return
            setTransportState({
              isPlaying: state.state === 'playing',
              currentBeat: state.ppqPosition,
              bpm: state.bpm,
              timeSignature: {
                numerator: state.timeSigNumerator,
                denominator: state.timeSigDenominator,
              },
            })
          })
          .catch(() => {
            // Engine not available — graceful fallback for dev without native engine
          })
      }

      rafRef.current = requestAnimationFrame(poll)
    }

    rafRef.current = requestAnimationFrame(poll)

    return () => {
      cancelled = true
      if (rafRef.current !== null) {
        cancelAnimationFrame(rafRef.current)
      }
    }
  }, [setTransportState])
}
