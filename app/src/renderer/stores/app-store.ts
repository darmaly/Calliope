import { create } from 'zustand'

type FocusedPanel = 'timeline' | 'piano-roll' | 'mixer'

interface AppState {
  focusedPanel: FocusedPanel
}

interface AppActions {
  setFocusedPanel: (panel: FocusedPanel) => void
}

export const useAppStore = create<AppState & AppActions>((set) => ({
  focusedPanel: 'timeline',

  setFocusedPanel: (panel) => set(() => ({ focusedPanel: panel })),
}))
