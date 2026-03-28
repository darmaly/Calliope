import { useEffect } from 'react'
import { useTimelineStore } from '../stores/timeline-store'

/**
 * Subscribes to engine command events via the preload bridge and
 * mirrors transport state changes into the Zustand timeline store.
 */
export function useEngineSync(): void {
  useEffect(() => {
    if (!window.calliope?.onCommandEvent) return

    window.calliope.onCommandEvent((event) => {
      try {
        const data = typeof event.data === 'string' ? JSON.parse(event.data) : event.data

        if (event.type === 'transport' || event.command?.startsWith('transport')) {
          useTimelineStore.getState().setTransportState({
            isPlaying: data.state === 'playing',
            currentBeat: data.ppqPosition ?? data.currentBeat ?? 0,
            bpm: data.bpm ?? useTimelineStore.getState().bpm,
            timeSignature: {
              numerator: data.timeSigNumerator ?? useTimelineStore.getState().timeSignature.numerator,
              denominator: data.timeSigDenominator ?? useTimelineStore.getState().timeSignature.denominator,
            },
          })
        }
      } catch {
        // Ignore malformed events
      }
    })

    return () => {
      window.calliope?.removeCommandEventListener()
    }
  }, [])
}
