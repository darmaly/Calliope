import { Piano, SlidersHorizontal } from 'lucide-react'
import { TransportControls } from './TransportControls'
import { BpmDisplay } from './BpmDisplay'
import { TimeSignatureDisplay } from './TimeSignatureDisplay'
import { PositionDisplay } from './PositionDisplay'
import { LoopToggle } from './LoopToggle'
import { MetronomeToggle } from './MetronomeToggle'
import { useTimelineStore } from '../../stores/timeline-store'
import { usePianoRollStore } from '../../stores/piano-roll-store'
import { useMixerStore } from '../../stores/mixer-store'

function Separator() {
  return <div className="w-px h-5 bg-[#3a3a5a] mx-4" />
}

export function TransportBar() {
  return (
    <div className="h-[48px] flex items-center px-4 bg-[#252542] border-b border-[#3a3a5a] shrink-0">
      {/* Transport controls: Play, Stop, Record */}
      <TransportControls />

      <Separator />

      {/* Tempo section: BPM + Time Signature */}
      <div className="flex items-center gap-4">
        <BpmDisplay />
        <TimeSignatureDisplay />
      </div>

      <Separator />

      {/* Position section: Time + Bars:Beats */}
      <PositionDisplay />

      <Separator />

      {/* Toggle section: Loop + Metronome */}
      <div className="flex items-center gap-2">
        <LoopToggle />
        <MetronomeToggle />
      </div>

      {/* Spacer */}
      <div className="flex-1" />

      {/* Panel toggle buttons (right side) */}
      <div className="flex items-center gap-2">
        <PianoRollToggle />
        <MixerToggle />
      </div>
    </div>
  )
}

function PianoRollToggle() {
  const activeClipId = usePianoRollStore((s) => s.activeClipId)

  const handleToggle = () => {
    const prState = usePianoRollStore.getState()
    if (prState.activeClipId) {
      prState.setActiveClip(null)
    } else {
      const tlState = useTimelineStore.getState()
      const midiClip = Object.values(tlState.clips).find((c) => c.type === 'midi')
      if (midiClip) {
        prState.setActiveClip(midiClip.id)
      }
    }
  }

  return (
    <button
      onPointerDown={handleToggle}
      className={`flex items-center gap-1 px-2 py-1 rounded text-[13px] transition-colors ${
        activeClipId
          ? 'text-[#6c63ff] underline'
          : 'text-[#999999] hover:text-[#eeeeee] hover:bg-[#3a3a5a]'
      }`}
      title="Toggle Piano Roll"
    >
      <Piano size={16} />
      Piano Roll
    </button>
  )
}

function MixerToggle() {
  const mixerVisible = useMixerStore((s) => s.panelVisible)
  return (
    <button
      className={`flex items-center gap-1 px-2 py-1 rounded text-[13px] transition-colors ${
        mixerVisible
          ? 'text-[#6c63ff] bg-[#6c63ff]/10'
          : 'text-[#999999] hover:text-[#eeeeee] hover:bg-[#3a3a5a]'
      }`}
      title="Toggle Mixer"
      onPointerDown={() => useMixerStore.getState().togglePanel()}
    >
      <SlidersHorizontal size={16} />
      Mixer
    </button>
  )
}
