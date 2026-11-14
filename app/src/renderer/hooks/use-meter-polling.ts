import { useEffect } from 'react'
import { useMixerStore } from '../stores/mixer-store'

/** Polls level meters from the engine while the mixer is visible. */
export function useMeterPolling() {
  const mixerVisible = useMixerStore((s) => s.mixerVisible)

  useEffect(() => {
    if (!mixerVisible) return
    let cancelled = false
    let raf = 0

    const poll = async () => {
      try {
        const levels = await window.calliope.getMeterLevels()
        if (cancelled) return
        const store = useMixerStore.getState()
        for (const [trackId, data] of Object.entries(levels)) {
          const level = { peakL: data.peakLeft, peakR: data.peakRight }
          if (trackId === 'master') store.setMasterLevel(level)
          else store.setTrackLevel(trackId, level)
        }
      } catch {
        /* engine not ready */
      }
      // Ask again only once the previous answer is in.
      if (!cancelled) raf = requestAnimationFrame(poll)
    }

    raf = requestAnimationFrame(poll)
    return () => {
      cancelled = true
      cancelAnimationFrame(raf)
    }
  }, [mixerVisible])
}
