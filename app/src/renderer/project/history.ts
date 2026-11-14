// Undo and redo for the project document.
//
// A document is a tree of immutable parts, so remembering one is just keeping
// a reference to it; undo puts an earlier one back and the engine-sync layer
// brings the engine along. That covers everything in the document (clips,
// notes, tracks, mixer, effects, tempo) with one mechanism, whoever made the
// change.

import type { ProjectDocument } from './document'
import { applyDocument, getDocument, sameDocument, subscribeDocument } from './document'

/** Changes closer together than this are one undo step (a drag, a typed value). */
export const COALESCE_MS = 400
export const MAX_HISTORY = 100

export interface History {
  undo(): boolean
  redo(): boolean
  canUndo(): boolean
  canRedo(): boolean
  /** Forget everything, e.g. after opening another project. */
  clear(): void
  /**
   * Run `fn` and record everything it changes as exactly one undo step,
   * however long it takes. Used for an AI turn.
   */
  transaction<T>(fn: () => Promise<T>): Promise<T>
  dispose(): void
}

export function createHistory(): History {
  const undoStack: ProjectDocument[] = []
  const redoStack: ProjectDocument[] = []

  let current = getDocument()
  // The document as it was before the group of changes now in progress.
  let groupStart: ProjectDocument | null = null
  let timer: ReturnType<typeof setTimeout> | null = null
  let applying = false
  let inTransaction = false

  const commit = () => {
    if (timer !== null) {
      clearTimeout(timer)
      timer = null
    }
    if (groupStart !== null && !sameDocument(groupStart, current)) {
      undoStack.push(groupStart)
      if (undoStack.length > MAX_HISTORY) undoStack.shift()
      redoStack.length = 0
    }
    groupStart = null
  }

  const unsubscribe = subscribeDocument((doc) => {
    if (applying) {
      current = doc
      return
    }
    if (groupStart === null) groupStart = current
    current = doc
    if (inTransaction) return
    if (timer !== null) clearTimeout(timer)
    timer = setTimeout(commit, COALESCE_MS)
  })

  const restore = (doc: ProjectDocument) => {
    applying = true
    try {
      applyDocument(doc)
    } finally {
      applying = false
    }
    current = getDocument()
  }

  return {
    undo() {
      commit()
      const previous = undoStack.pop()
      if (!previous) return false
      redoStack.push(current)
      restore(previous)
      return true
    },
    redo() {
      commit()
      const next = redoStack.pop()
      if (!next) return false
      undoStack.push(current)
      restore(next)
      return true
    },
    canUndo() {
      return undoStack.length > 0 || (groupStart !== null && !sameDocument(groupStart, current))
    },
    canRedo() {
      return redoStack.length > 0
    },
    clear() {
      if (timer !== null) clearTimeout(timer)
      timer = null
      groupStart = null
      undoStack.length = 0
      redoStack.length = 0
      current = getDocument()
    },
    async transaction<T>(fn: () => Promise<T>): Promise<T> {
      commit()
      inTransaction = true
      try {
        return await fn()
      } finally {
        inTransaction = false
        commit()
      }
    },
    dispose() {
      if (timer !== null) clearTimeout(timer)
      unsubscribe()
    },
  }
}
