export function MixerToolbar() {
  return (
    <div
      style={{
        height: 36,
        display: 'flex',
        alignItems: 'center',
        paddingLeft: 12,
        paddingRight: 12,
        backgroundColor: '#252542',
        borderBottom: '1px solid #3a3a5a',
        flexShrink: 0,
      }}
    >
      <span style={{ fontSize: 13, fontWeight: 500, color: '#eeeeee', userSelect: 'none' }}>
        Mixer
      </span>
      {/* Right side: reserved for future view controls */}
      <div style={{ flex: 1 }} />
    </div>
  )
}
