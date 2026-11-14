import { Component } from 'react'
import type { ErrorInfo, ReactNode } from 'react'

interface Props {
  children: ReactNode
}

interface State {
  error: Error | null
}

/**
 * Catches a rendering error anywhere below it. Without this, React unmounts
 * the whole tree and the user is left looking at an empty window with no
 * idea what happened or whether their work is safe.
 */
export class ErrorBoundary extends Component<Props, State> {
  state: State = { error: null }

  static getDerivedStateFromError(error: Error): State {
    return { error }
  }

  componentDidCatch(error: Error, info: ErrorInfo): void {
    console.error('Render error:', error, info.componentStack)
  }

  render(): ReactNode {
    const { error } = this.state
    if (!error) return this.props.children

    return (
      <div className="h-screen w-screen bg-[#1a1a2e] text-[#eeeeee] flex items-center justify-center p-8">
        <div className="max-w-[520px]">
          <h1 className="text-[18px] font-semibold mb-2">Something went wrong drawing the window</h1>
          <p className="text-[13px] text-[#aaaacc] mb-4">
            Your project is still in memory and the audio engine is still running. Try showing the
            window again; if that works, save your project.
          </p>
          <pre className="text-[11px] text-[#ff9999] bg-[#16162b] border border-[#3a3a5a] rounded p-3 mb-4 overflow-auto max-h-[160px] whitespace-pre-wrap">
            {error.message}
          </pre>
          <button
            onClick={() => this.setState({ error: null })}
            className="px-4 py-2 rounded bg-[#5555aa] text-[13px] hover:bg-[#6666bb]"
          >
            Try again
          </button>
        </div>
      </div>
    )
  }
}
