import { useEffect } from 'react'
import { useMixerStore } from '../stores/mixer-store'

export function useMeterPolling() {
  const mixerVisible = useMixerStore((s) => s.mixerVisible)

  useEffect(() => {
    if (!mixerVisible) return
    let rafId: number
    const poll = async () => {
      try {
        const levels = await window.calliope.getMeterLevels()
        const store = useMixerStore.getState()
        // Update per-track levels
        for (const [trackId, data] of Object.entries(levels) as [string, { peakLeft: number; peakRight: number }][]) {
          if (trackId === 'master') {
            store.setMasterLevel({ peakL: data.peakLeft, peakR: data.peakRight })
          } else {
            store.setTrackLevel(trackId, { peakL: data.peakLeft, peakR: data.peakRight })
          }
        }
      } catch {
        /* engine not ready */
      }
      rafId = requestAnimationFrame(poll)
    }
    rafId = requestAnimationFrame(poll)
    return () => cancelAnimationFrame(rafId)
  }, [mixerVisible])
}
