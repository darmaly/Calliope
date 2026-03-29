import { useEffect, useRef } from 'react'
import { createPortal } from 'react-dom'

interface ToastProps {
  message: string
  type: 'success' | 'error'
  onRetry?: () => void
  onDismiss: () => void
}

export function Toast({ message, type, onRetry, onDismiss }: ToastProps) {
  const timerRef = useRef<ReturnType<typeof setTimeout> | null>(null)

  // Auto-dismiss after 5 seconds unless error with retry
  useEffect(() => {
    const shouldAutoDismiss = !(type === 'error' && onRetry)
    if (shouldAutoDismiss) {
      timerRef.current = setTimeout(onDismiss, 5000)
    }
    return () => {
      if (timerRef.current) {
        clearTimeout(timerRef.current)
      }
    }
  }, [type, onRetry, onDismiss])

  const bgColor = type === 'error' ? 'bg-[#cc3344]' : 'bg-[#338844]'

  return createPortal(
    <div
      className={`fixed bottom-4 right-4 z-50 ${bgColor} text-[#eeeeee] rounded-lg shadow-2xl px-4 py-3 max-w-[360px] flex items-start gap-3`}
    >
      <p className="flex-1 text-sm">{message}</p>
      <div className="flex items-center gap-2 shrink-0">
        {type === 'error' && onRetry && (
          <button
            onClick={onRetry}
            className="text-xs font-medium px-2 py-1 rounded bg-[rgba(255,255,255,0.2)] hover:bg-[rgba(255,255,255,0.3)] transition-colors"
          >
            Retry
          </button>
        )}
        <button
          onClick={onDismiss}
          className="text-xs font-medium px-1 py-1 rounded hover:bg-[rgba(255,255,255,0.2)] transition-colors"
          aria-label="Dismiss"
        >
          X
        </button>
      </div>
    </div>,
    document.body
  )
}
