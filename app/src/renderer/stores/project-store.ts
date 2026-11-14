import { create } from 'zustand'

export interface ToastMessage {
  message: string
  type: 'success' | 'error'
  retryFn?: () => void
}

// Where the project lives on disk and whether it has unsaved changes, plus
// the export dialog / progress / toast UI state. The project's contents are
// in the timeline and mixer stores; saving and opening are in project/session.

export interface ProjectState {
  /** Path to the current project file on disk, null if unsaved */
  filePath: string | null
  /** Project display name derived from filePath or 'Untitled' */
  projectName: string
  /** Whether the project has unsaved changes */
  isDirty: boolean
  /** ISO timestamp of last successful save */
  lastSaved: string | null
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
  /** Mark the project as saved (clean state), optionally at a new path */
  markClean: (filePath?: string) => void
  /** Back to an unsaved, untitled project */
  reset: () => void
  showExportDialog: () => void
  hideExportDialog: () => void
  setExportProgress: (pct: number | null) => void
  showToast: (msg: ToastMessage) => void
  hideToast: () => void
}

export const PROJECT_EXTENSION = 'calliope'

export const deriveProjectName = (filePath: string | null): string => {
  if (!filePath) return 'Untitled'
  const parts = filePath.replace(/\\/g, '/').split('/')
  const fileName = parts[parts.length - 1]
  return fileName.replace(/\.(calliope|ltproj)$/i, '')
}

const initialState: ProjectState = {
  filePath: null,
  projectName: 'Untitled',
  isDirty: false,
  lastSaved: null,
  exportDialogOpen: false,
  exportProgress: null,
  toastMessage: null,
}

export const useProjectStore = create<ProjectState & ProjectActions>((set, get) => ({
  ...initialState,

  markDirty: () => {
    if (!get().isDirty) set({ isDirty: true })
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

  reset: () =>
    set((state) => ({
      ...initialState,
      // Leave whatever the user is looking at (a toast, an export) alone.
      exportDialogOpen: state.exportDialogOpen,
      exportProgress: state.exportProgress,
      toastMessage: state.toastMessage,
    })),

  showExportDialog: () => set({ exportDialogOpen: true }),
  hideExportDialog: () => set({ exportDialogOpen: false }),
  setExportProgress: (pct: number | null) => set({ exportProgress: pct }),
  showToast: (msg: ToastMessage) => set({ toastMessage: msg }),
  hideToast: () => set({ toastMessage: null }),
}))
