interface CalliopeAPI {
  getEngineInfo(): Promise<{ juceVersion: string; audioDevices: string[] }>
  startTestTone(frequency: number): Promise<boolean>
  stopTestTone(): Promise<void>
}

declare global {
  interface Window {
    calliope: CalliopeAPI
  }
}

export {}
