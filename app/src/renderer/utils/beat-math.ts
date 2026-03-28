export function beatToPixel(beat: number, pixelsPerBeat: number, scrollX: number): number {
  return beat * pixelsPerBeat - scrollX
}

export function pixelToBeat(pixel: number, pixelsPerBeat: number, scrollX: number): number {
  return (pixel + scrollX) / pixelsPerBeat
}

export function snapToBeat(beat: number, gridResolution: number): number {
  return Math.round(beat / gridResolution) * gridResolution
}
