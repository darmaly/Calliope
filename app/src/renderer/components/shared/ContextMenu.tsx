import { useEffect, useRef } from 'react'
import { createPortal } from 'react-dom'

export interface ContextMenuItem {
  label: string
  action: () => void
  destructive?: boolean
}

interface ContextMenuProps {
  items: ContextMenuItem[]
  position: { x: number; y: number }
  onClose: () => void
}

export function ContextMenu({ items, position, onClose }: ContextMenuProps) {
  const menuRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    const handleClick = (e: MouseEvent) => {
      if (menuRef.current && !menuRef.current.contains(e.target as Node)) {
        onClose()
      }
    }
    const handleKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape') onClose()
    }
    // Delay listener attachment so the triggering right-click doesn't immediately close
    requestAnimationFrame(() => {
      document.addEventListener('mousedown', handleClick)
      document.addEventListener('keydown', handleKey)
    })
    return () => {
      document.removeEventListener('mousedown', handleClick)
      document.removeEventListener('keydown', handleKey)
    }
  }, [onClose])

  return createPortal(
    <div
      ref={menuRef}
      className="fixed bg-[#252542] border border-[#3a3a5a] rounded-md shadow-lg py-1 min-w-[160px]"
      style={{
        left: position.x,
        top: position.y,
        zIndex: 50,
      }}
    >
      {items.map((item, i) => (
        <button
          key={i}
          className={`w-full text-left px-4 py-2 text-[13px] hover:bg-[#2a2a4a] transition-colors ${
            item.destructive ? 'text-[#ef4444]' : 'text-[#eeeeee]'
          }`}
          onClick={() => {
            item.action()
            onClose()
          }}
        >
          {item.label}
        </button>
      ))}
    </div>,
    document.body,
  )
}
