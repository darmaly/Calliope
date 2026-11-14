import type { CalliopeAPI } from '../../shared/api'

declare global {
  interface Window {
    /** Exposed by the preload script. See app/src/shared/api.ts. */
    calliope: CalliopeAPI
  }
}

export {}
