import { createRequire } from 'module'
import { app } from 'electron'
import path from 'path'

const require = createRequire(import.meta.url)

interface NativeAddon {
  getEngineInfo(): Promise<{ juceVersion: string; audioDevices: string[] }>
  startTestTone(frequency: number): Promise<boolean>
  stopTestTone(): Promise<void>
}

function loadAddon(): NativeAddon {
  // In dev: addon is at project_root/build/Release/calliope_addon.node
  // In production: addon is unpacked from asar
  const addonPath = app.isPackaged
    ? path.join(process.resourcesPath, 'build', 'Release', 'calliope_addon.node')
    : path.join(__dirname, '..', '..', '..', 'build', 'Release', 'calliope_addon.node')
  return require(addonPath) as NativeAddon
}

export const native = loadAddon()
