import { useState, useCallback } from 'react'
import type { ContextMenuItem } from '../components/shared/ContextMenu'

interface ContextMenuState {
  visible: boolean
  position: { x: number; y: number }
  items: ContextMenuItem[]
}

export function useContextMenu() {
  const [state, setState] = useState<ContextMenuState>({
    visible: false,
    position: { x: 0, y: 0 },
    items: [],
  })

  const show = useCallback(
    (event: React.MouseEvent | MouseEvent, items: ContextMenuItem[]) => {
      event.preventDefault()
      event.stopPropagation()
      setState({
        visible: true,
        position: { x: event.clientX, y: event.clientY },
        items,
      })
    },
    [],
  )

  const close = useCallback(() => {
    setState((prev) => ({ ...prev, visible: false }))
  }, [])

  return { ...state, show, close }
}
