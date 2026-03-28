import React, { useMemo } from 'react'
import type { Track } from '../../types/timeline'
import { useTimelineStore } from '../../stores/timeline-store'
import { useShallow } from 'zustand/shallow'
import { MidiClip } from './MidiClip'
import { AudioClip } from './AudioClip'

export interface ClipContainerProps {
  track: Track
  trackIndex: number
}

const TRACK_ROW_HEIGHT = 80

export const ClipContainer: React.FC<ClipContainerProps> = React.memo(
  ({ track, trackIndex }) => {
    const allClips = useTimelineStore((s) => s.clips)
    const pixelsPerBeat = useTimelineStore((s) => s.pixelsPerBeat)
    const selectedClipIds = useTimelineStore((s) => s.selectedClipIds)
    const clips = useMemo(
      () => Object.values(allClips).filter((c) => c.trackId === track.id),
      [allClips, track.id],
    )

    const trackRowY = 4 // vertical padding within the container

    return (
      <pixiContainer y={trackIndex * TRACK_ROW_HEIGHT}>
        {clips.map((clip) => {
          const isSelected = selectedClipIds.has(clip.id)

          if (clip.type === 'midi') {
            return (
              <MidiClip
                key={clip.id}
                clip={clip}
                trackColorIndex={track.colorIndex}
                pixelsPerBeat={pixelsPerBeat}
                isSelected={isSelected}
                trackRowY={trackRowY}
              />
            )
          }

          return (
            <AudioClip
              key={clip.id}
              clip={clip}
              trackColorIndex={track.colorIndex}
              pixelsPerBeat={pixelsPerBeat}
              isSelected={isSelected}
              trackRowY={trackRowY}
            />
          )
        })}
      </pixiContainer>
    )
  },
)

ClipContainer.displayName = 'ClipContainer'
