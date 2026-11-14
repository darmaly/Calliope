// The running project: wires the document to the engine, to undo/redo and to
// files on disk. Started once, after the audio engine is up.

import {
  ProjectFormatError,
  applyDocument,
  emptyDocument,
  getDocument,
  parseDocument,
  sameDocument,
  serializeDocument,
  subscribeDocument,
} from './document'
import { createEngineSync } from './engine-sync'
import type { EngineSync } from './engine-sync'
import { createHistory } from './history'
import type { History } from './history'
import { useProjectStore } from '../stores/project-store'
import { usePianoRollStore } from '../stores/piano-roll-store'

const AUTOSAVE_INTERVAL_MS = 120_000

let engineSync: EngineSync | null = null
let history: History | null = null

const toast = (message: string, type: 'success' | 'error' = 'error') =>
  useProjectStore.getState().showToast({ message, type })

/**
 * Start following the document. Resets the engine and rebuilds it from the
 * document, so the two agree from the first moment (including after a
 * renderer reload, when the engine would otherwise still hold the old project).
 * Returns a function that stops the session.
 */
export async function startProjectSession(): Promise<() => void> {
  stopProjectSession()

  engineSync = createEngineSync(window.calliope, (message) => console.warn('[engine-sync]', message))
  await engineSync.rebuild()
  history = createHistory()

  const unsubscribeDirty = subscribeDocument(() => {
    const store = useProjectStore.getState()
    if (!store.isDirty) {
      store.markDirty()
      window.calliope.projectSetDirty(true)
    }
  })

  const autosave = setInterval(() => {
    const { isDirty, filePath } = useProjectStore.getState()
    // Only ever autosave over a file the user has already chosen.
    if (isDirty && filePath) void saveProject({ quiet: true })
  }, AUTOSAVE_INTERVAL_MS)

  const stop = () => {
    unsubscribeDirty()
    clearInterval(autosave)
    engineSync?.dispose()
    history?.dispose()
    engineSync = null
    history = null
    stopCurrent = null
  }
  stopCurrent = stop
  return stop
}

let stopCurrent: (() => void) | null = null

export function stopProjectSession(): void {
  stopCurrent?.()
}

/** Resolves once every document change made so far has reached the engine. */
export async function engineSettled(): Promise<void> {
  await engineSync?.settled()
}

export function undo(): boolean {
  return history?.undo() ?? false
}

export function redo(): boolean {
  return history?.redo() ?? false
}

/** Group everything `fn` changes into one undo step. */
export function asOneUndoStep<T>(fn: () => Promise<T>): Promise<T> {
  return history ? history.transaction(fn) : fn()
}

// ---------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------

function confirmDiscard(action: string): boolean {
  if (!useProjectStore.getState().isDirty) return true
  return window.confirm(`You have unsaved changes. ${action} anyway and lose them?`)
}

function replaceDocument(doc: ReturnType<typeof emptyDocument>): void {
  window.calliope.transportStop().catch(() => {})
  usePianoRollStore.getState().setActiveClip(null)
  applyDocument(doc)
  history?.clear()
}

export async function saveProject(options: { saveAs?: boolean; quiet?: boolean } = {}): Promise<boolean> {
  const store = useProjectStore.getState()
  const snapshot = getDocument()
  try {
    const result = await window.calliope.projectSave({
      contents: serializeDocument(snapshot),
      filePath: options.saveAs ? null : store.filePath,
      suggestedName: store.projectName,
    })
    if (result.canceled) return false
    if (!result.success || !result.filePath) {
      toast(`Could not save the project${result.error ? `: ${result.error}` : '.'}`)
      return false
    }
    useProjectStore.getState().markClean(result.filePath)
    // Edits made while the file was being written are still unsaved.
    if (!sameDocument(getDocument(), snapshot)) {
      useProjectStore.getState().markDirty()
    } else {
      window.calliope.projectSetDirty(false)
    }
    if (!options.quiet) toast('Project saved.', 'success')
    return true
  } catch (err) {
    toast(`Could not save the project: ${err instanceof Error ? err.message : String(err)}`)
    return false
  }
}

export async function openProject(): Promise<boolean> {
  if (!confirmDiscard('Open another project')) return false
  try {
    const result = await window.calliope.projectOpen()
    if (result.canceled) return false
    if (!result.success || result.contents === undefined || !result.filePath) {
      toast(`Could not open the project${result.error ? `: ${result.error}` : '.'}`)
      return false
    }
    const doc = parseDocument(result.contents)
    replaceDocument(doc)
    useProjectStore.getState().markClean(result.filePath)
    window.calliope.projectSetDirty(false)
    return true
  } catch (err) {
    toast(
      err instanceof ProjectFormatError
        ? err.message
        : `Could not open the project: ${err instanceof Error ? err.message : String(err)}`,
    )
    return false
  }
}

export async function newProject(): Promise<boolean> {
  if (!confirmDiscard('Start a new project')) return false
  replaceDocument(emptyDocument())
  useProjectStore.getState().reset()
  window.calliope.projectSetDirty(false)
  return true
}
