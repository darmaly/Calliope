import { useState, useEffect } from 'react'
import { TestTone } from './components/TestTone'

export default function App() {
  const [engineInfo, setEngineInfo] = useState<{
    juceVersion: string
    audioDevices: string[]
  } | null>(null)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    window.calliope.getEngineInfo()
      .then(setEngineInfo)
      .catch((err: Error) => setError(err.message))
  }, [])

  return (
    <div style={{ padding: '2rem', fontFamily: 'system-ui, sans-serif' }}>
      <h1>Calliope</h1>
      <p style={{ color: '#666' }}>AI-Powered Digital Audio Workstation</p>

      <section style={{ marginTop: '2rem' }}>
        <h2>Engine Status</h2>
        {error && <p style={{ color: 'red' }}>Error: {error}</p>}
        {engineInfo ? (
          <div>
            <p><strong>JUCE Version:</strong> {engineInfo.juceVersion}</p>
            <p><strong>Audio Devices:</strong></p>
            <ul>
              {engineInfo.audioDevices.map((device, i) => (
                <li key={i}>{device}</li>
              ))}
            </ul>
          </div>
        ) : (
          !error && <p>Loading engine info...</p>
        )}
      </section>

      <section style={{ marginTop: '2rem' }}>
        <TestTone />
      </section>
    </div>
  )
}
