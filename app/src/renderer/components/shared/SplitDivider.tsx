import { useCallback, useRef } from 'react'

interface SplitDividerProps {
  onDrag: (deltaY: number) => void
  onDoubleClick: () => void
}

export function SplitDivider({ onDrag, onDoubleClick }: SplitDividerProps) {
  const lastYRef = useRef(0)

  const handlePointerDown = useCallback(
    (e: React.PointerEvent<HTMLDivElement>) => {
      e.preventDefault()
      lastYRef.current = e.clientY
      document.body.style.userSelect = 'none'

      const handlePointerMove = (ev: PointerEvent) => {
        const deltaY = ev.clientY - lastYRef.current
        lastYRef.current = ev.clientY
        onDrag(deltaY)
      }

      const handlePointerUp = () => {
        document.body.style.userSelect = ''
        document.removeEventListener('pointermove', handlePointerMove)
        document.removeEventListener('pointerup', handlePointerUp)
      }

      document.addEventListener('pointermove', handlePointerMove)
      document.addEventListener('pointerup', handlePointerUp)
    },
    [onDrag],
  )

  return (
    <div
      className="relative shrink-0 cursor-row-resize group"
      style={{ height: 8 }}
      onPointerDown={handlePointerDown}
      onDoubleClick={onDoubleClick}
    >
      {/* Visible line centered in hit area */}
      <div
        className="absolute left-0 right-0 top-1/2 -translate-y-1/2 bg-[#3a3a5a] group-hover:bg-[#6c63ff] transition-colors"
        style={{ height: 2 }}
      />
    </div>
  )
}
