// Reading and writing project files.
//
// The renderer owns the project's contents and sends them here as text; this
// module owns the file system. The renderer can only write to a path the user
// picked in a native dialog during this session.

import { dialog, ipcMain } from 'electron'
import type { BrowserWindow } from 'electron'
import { promises as fs } from 'fs'
import path from 'path'
import type { ProjectFileResult, ProjectSaveRequest } from '../shared/api'

const PROJECT_EXTENSION = 'calliope'
const PROJECT_FILTERS = [
  { name: 'Calliope Project', extensions: [PROJECT_EXTENSION] },
]
const MAX_PROJECT_BYTES = 64 * 1024 * 1024

const approvedPaths = new Set<string>()
let hasUnsavedChanges = false

export function projectHasUnsavedChanges(): boolean {
  return hasUnsavedChanges
}

/** Write via a temporary file so a crash mid-write cannot destroy the previous save. */
async function writeFileAtomic(filePath: string, contents: string): Promise<void> {
  const tempPath = `${filePath}.${process.pid}.tmp`
  try {
    await fs.writeFile(tempPath, contents, 'utf8')
    await fs.rename(tempPath, filePath)
  } catch (err) {
    await fs.rm(tempPath, { force: true }).catch(() => {})
    throw err
  }
}

const errorMessage = (err: unknown): string => (err instanceof Error ? err.message : String(err))

export function registerProjectIpc(getWindow: () => BrowserWindow | null): void {
  ipcMain.handle('project:save', async (_e, request: ProjectSaveRequest): Promise<ProjectFileResult> => {
    if (typeof request?.contents !== 'string') {
      return { success: false, error: 'Nothing to save.' }
    }

    let filePath = request.filePath ? path.resolve(request.filePath) : null
    if (!filePath || !approvedPaths.has(filePath)) {
      const safeName = (request.suggestedName ?? 'Untitled').replace(/[\\/:*?"<>|]/g, '').trim()
      const options = {
        title: 'Save Project',
        defaultPath: filePath ?? `${safeName || 'Untitled'}.${PROJECT_EXTENSION}`,
        filters: PROJECT_FILTERS,
      }
      const window = getWindow()
      const result = window
        ? await dialog.showSaveDialog(window, options)
        : await dialog.showSaveDialog(options)
      if (result.canceled || !result.filePath) return { success: false, canceled: true }
      filePath = path.resolve(result.filePath)
    }

    try {
      await writeFileAtomic(filePath, request.contents)
      approvedPaths.add(filePath)
      return { success: true, filePath }
    } catch (err) {
      return { success: false, filePath, error: errorMessage(err) }
    }
  })

  ipcMain.handle('project:open', async (): Promise<ProjectFileResult> => {
    const options = {
      title: 'Open Project',
      filters: PROJECT_FILTERS,
      properties: ['openFile' as const],
    }
    const window = getWindow()
    const result = window
      ? await dialog.showOpenDialog(window, options)
      : await dialog.showOpenDialog(options)
    if (result.canceled || result.filePaths.length === 0) return { success: false, canceled: true }

    const filePath = path.resolve(result.filePaths[0])
    try {
      const stat = await fs.stat(filePath)
      if (stat.size > MAX_PROJECT_BYTES) {
        return { success: false, filePath, error: 'The file is too large to be a project.' }
      }
      const contents = await fs.readFile(filePath, 'utf8')
      approvedPaths.add(filePath)
      return { success: true, filePath, contents }
    } catch (err) {
      return { success: false, filePath, error: errorMessage(err) }
    }
  })

  ipcMain.on('project:setDirty', (_e, dirty: boolean) => {
    hasUnsavedChanges = dirty === true
  })
}
