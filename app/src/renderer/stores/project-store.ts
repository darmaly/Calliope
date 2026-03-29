import { create } from 'zustand'

export interface ToastMessage {
  message: string
  type: 'success' | 'error'
  retryFn?: () => void
}

export interface ProjectState {
  /** Path to the current project file on disk, null if unsaved */
  filePath: string | null
  /** Project display name derived from filePath or 'Untitled' */
  projectName: string
  /** Whether the project has unsaved changes */
  isDirty: boolean
  /** ISO timestamp of last successful save */
  lastSaved: string | null
  /** Whether autosave is enabled */
  autosaveEnabled: boolean
  /** Autosave interval in milliseconds */
  autosaveIntervalMs: number
  /** ISO timestamp of last autosave */
  lastAutosaved: string | null
  /** Whether the export dialog is open */
  exportDialogOpen: boolean
  /** Export progress: null = not exporting, 0-1 = in progress */
  exportProgress: number | null
  /** Toast notification message */
  toastMessage: ToastMessage | null
}

export interface ProjectActions {
  /** Mark the project as having unsaved changes */
  markDirty: () => void
  /** Mark the project as saved (clean state) */
  markClean: (filePath?: string) => void
  /** Set the current project file path */
  setFilePath: (filePath: string | null) => void
  /** Update autosave settings */
  setAutosave: (enabled: boolean, intervalMs?: number) => void
  /** Record an autosave event */
  recordAutosave: (filePath: string, timestamp: string) => void
  /** Reset to new project state */
  reset: () => void
  /** Save the project (delegates to IPC) */
  save: () => Promise<boolean>
  /** Save As (delegates to IPC) */
  saveAs: () => Promise<boolean>
  /** Load a project (delegates to IPC) */
  load: () => Promise<boolean>
  /** Create a new project */
  newProject: () => Promise<void>
  /** Show the export dialog */
  showExportDialog: () => void
  /** Hide the export dialog */
  hideExportDialog: () => void
  /** Set export progress (null = not exporting, 0-1 = in progress) */
  setExportProgress: (pct: number | null) => void
  /** Show a toast notification */
  showToast: (msg: ToastMessage) => void
  /** Hide the current toast */
  hideToast: () => void
}

const deriveProjectName = (filePath: string | null): string => {
  if (!filePath) return 'Untitled'
  const parts = filePath.replace(/\\/g, '/').split('/')
  const fileName = parts[parts.length - 1]
  return fileName.replace(/\.ltproj$/, '')
}

const initialState: ProjectState = {
  filePath: null,
  projectName: 'Untitled',
  isDirty: false,
  lastSaved: null,
  autosaveEnabled: true,
  autosaveIntervalMs: 120000,
  lastAutosaved: null,
  exportDialogOpen: false,
  exportProgress: null,
  toastMessage: null,
}

export const useProjectStore = create<ProjectState & ProjectActions>((set, get) => ({
  ...initialState,

  markDirty: () => {
    if (!get().isDirty) {
      set({ isDirty: true })
      // Notify main process for autosave tracking
      window.calliope?.projectMarkDirty?.()
    }
  },

  markClean: (filePath?: string) => {
    const updates: Partial<ProjectState> = {
      isDirty: false,
      lastSaved: new Date().toISOString(),
    }
    if (filePath !== undefined) {
      updates.filePath = filePath
      updates.projectName = deriveProjectName(filePath)
    }
    set(updates)
  },

  setFilePath: (filePath: string | null) =>
    set({
      filePath,
      projectName: deriveProjectName(filePath),
    }),

  setAutosave: (enabled: boolean, intervalMs?: number) => {
    const updates: Partial<ProjectState> = { autosaveEnabled: enabled }
    if (intervalMs !== undefined) updates.autosaveIntervalMs = intervalMs
    set(updates)
    // Sync to main process
    window.calliope?.projectSetAutosave?.(enabled, intervalMs)
  },

  recordAutosave: (filePath: string, timestamp: string) =>
    set({
      lastAutosaved: timestamp,
      isDirty: false,
      filePath,
      projectName: deriveProjectName(filePath),
    }),

  reset: () => set({ ...initialState }),

  save: async () => {
    const { filePath } = get()
    const result = await window.calliope.projectSave(filePath ?? undefined)
    if (result.success && result.filePath) {
      get().markClean(result.filePath)
    }
    return result.success
  },

  saveAs: async () => {
    const result = await window.calliope.projectSaveAs()
    if (result.success && result.filePath) {
      get().markClean(result.filePath)
    }
    return result.success
  },

  load: async () => {
    const result = await window.calliope.projectLoad()
    if (result.success && result.filePath) {
      set({
        filePath: result.filePath,
        projectName: deriveProjectName(result.filePath),
        isDirty: false,
        lastSaved: new Date().toISOString(),
      })
    }
    return result.success
  },

  newProject: async () => {
    await window.calliope.projectNew()
    set({ ...initialState })
  },

  showExportDialog: () => set({ exportDialogOpen: true }),
  hideExportDialog: () => set({ exportDialogOpen: false }),
  setExportProgress: (pct: number | null) => set({ exportProgress: pct }),
  showToast: (msg: ToastMessage) => set({ toastMessage: msg }),
  hideToast: () => set({ toastMessage: null }),
}))

// Subscribe to command events to auto-mark dirty
export function initProjectDirtyTracking(): () => void {
  const handleCommandEvent = () => {
    useProjectStore.getState().markDirty()
  }

  window.calliope?.onCommandEvent?.(handleCommandEvent)

  // Subscribe to autosave notifications
  window.calliope?.onProjectAutosaved?.((data) => {
    useProjectStore.getState().recordAutosave(data.filePath, data.timestamp)
  })

  return () => {
    window.calliope?.removeCommandEventListener?.()
    window.calliope?.removeProjectAutosavedListener?.()
  }
}
