import { useState } from 'react'

export function TestTone() {
  const [playing, setPlaying] = useState(false)
  const [frequency, setFrequency] = useState(440)

  const toggle = async () => {
    if (playing) {
      await window.calliope.stopTestTone()
    } else {
      await window.calliope.startTestTone(frequency)
    }
    setPlaying(!playing)
  }

  return (
    <div>
      <h2>Test Tone</h2>
      <div style={{ marginTop: '0.5rem' }}>
        <label>
          Frequency: {frequency} Hz
          <input
            type="range"
            min={100}
            max={2000}
            value={frequency}
            onChange={(e) => setFrequency(Number(e.target.value))}
            disabled={playing}
          />
        </label>
      </div>
      <button onClick={toggle} style={{ marginTop: '0.5rem' }}>
        {playing ? 'Stop' : 'Play'} Test Tone
      </button>
    </div>
  )
}
