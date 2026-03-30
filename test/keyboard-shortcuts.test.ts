import { describe, it, expect } from 'vitest'
import { routeShortcut } from '../app/src/renderer/hooks/use-keyboard-shortcuts'

type FocusablePanel = 'timeline' | 'piano-roll' | 'mixer'

const no_mod = { ctrl: false, shift: false, alt: false }
const ctrl = { ctrl: true, shift: false, alt: false }
const ctrl_shift = { ctrl: true, shift: true, alt: false }

describe('routeShortcut', () => {
  describe('global shortcuts (fire regardless of panel)', () => {
    const panels: FocusablePanel[] = ['timeline', 'piano-roll', 'mixer']

    it('Space => global:play-toggle for all panels', () => {
      for (const panel of panels) {
        expect(routeShortcut(' ', no_mod, panel)).toBe('global:play-toggle')
      }
    })

    it('r => global:record-toggle for all panels', () => {
      for (const panel of panels) {
        expect(routeShortcut('r', no_mod, panel)).toBe('global:record-toggle')
      }
    })

    it('l => global:loop-toggle for all panels', () => {
      for (const panel of panels) {
        expect(routeShortcut('l', no_mod, panel)).toBe('global:loop-toggle')
      }
    })

    it('Ctrl+Z => global:undo for all panels', () => {
      for (const panel of panels) {
        expect(routeShortcut('z', ctrl, panel)).toBe('global:undo')
      }
    })

    it('Ctrl+Shift+Z => global:redo for all panels', () => {
      for (const panel of panels) {
        expect(routeShortcut('z', ctrl_shift, panel)).toBe('global:redo')
      }
    })

    it('Ctrl+S => global:save for all panels', () => {
      for (const panel of panels) {
        expect(routeShortcut('s', ctrl, panel)).toBe('global:save')
      }
    })

    it('Ctrl+Shift+E => global:export for all panels', () => {
      for (const panel of panels) {
        expect(routeShortcut('e', ctrl_shift, panel)).toBe('global:export')
      }
    })
  })

  describe('timeline-specific shortcuts', () => {
    it('Delete => timeline:delete-selected when focused on timeline', () => {
      expect(routeShortcut('Delete', no_mod, 'timeline')).toBe('timeline:delete-selected')
    })

    it('Ctrl+A => timeline:select-all when focused on timeline', () => {
      expect(routeShortcut('a', ctrl, 'timeline')).toBe('timeline:select-all')
    })

    it('Ctrl+D => timeline:duplicate when focused on timeline', () => {
      expect(routeShortcut('d', ctrl, 'timeline')).toBe('timeline:duplicate')
    })

    it('Escape => timeline:clear-selection when focused on timeline', () => {
      expect(routeShortcut('Escape', no_mod, 'timeline')).toBe('timeline:clear-selection')
    })

    it('s => timeline:toggle-snap when focused on timeline', () => {
      expect(routeShortcut('s', no_mod, 'timeline')).toBe('timeline:toggle-snap')
    })
  })

  describe('piano-roll-specific shortcuts', () => {
    it('Delete => piano-roll:delete-selected when focused on piano-roll', () => {
      expect(routeShortcut('Delete', no_mod, 'piano-roll')).toBe('piano-roll:delete-selected')
    })

    it('Ctrl+A => piano-roll:select-all when focused on piano-roll', () => {
      expect(routeShortcut('a', ctrl, 'piano-roll')).toBe('piano-roll:select-all')
    })

    it('Ctrl+D => piano-roll:duplicate when focused on piano-roll', () => {
      expect(routeShortcut('d', ctrl, 'piano-roll')).toBe('piano-roll:duplicate')
    })

    it('Escape => piano-roll:escape when focused on piano-roll', () => {
      expect(routeShortcut('Escape', no_mod, 'piano-roll')).toBe('piano-roll:escape')
    })

    it('q => piano-roll:quantize when focused on piano-roll', () => {
      expect(routeShortcut('q', no_mod, 'piano-roll')).toBe('piano-roll:quantize')
    })

    it('Ctrl+C => piano-roll:copy when focused on piano-roll', () => {
      expect(routeShortcut('c', ctrl, 'piano-roll')).toBe('piano-roll:copy')
    })

    it('Ctrl+V => piano-roll:paste when focused on piano-roll', () => {
      expect(routeShortcut('v', ctrl, 'piano-roll')).toBe('piano-roll:paste')
    })

    it('Ctrl+X => piano-roll:cut when focused on piano-roll', () => {
      expect(routeShortcut('x', ctrl, 'piano-roll')).toBe('piano-roll:cut')
    })
  })

  describe('mixer-specific shortcuts', () => {
    it('m => mixer:toggle-mute when focused on mixer', () => {
      expect(routeShortcut('m', no_mod, 'mixer')).toBe('mixer:toggle-mute')
    })

    it('s => mixer:toggle-solo when focused on mixer', () => {
      expect(routeShortcut('s', no_mod, 'mixer')).toBe('mixer:toggle-solo')
    })
  })

  describe('shortcuts that should NOT fire', () => {
    it('Delete => null when focused on mixer', () => {
      expect(routeShortcut('Delete', no_mod, 'mixer')).toBeNull()
    })

    it('q => null when focused on timeline', () => {
      expect(routeShortcut('q', no_mod, 'timeline')).toBeNull()
    })

    it('q => null when focused on mixer', () => {
      expect(routeShortcut('q', no_mod, 'mixer')).toBeNull()
    })

    it('m => null when focused on timeline', () => {
      expect(routeShortcut('m', no_mod, 'timeline')).toBeNull()
    })

    it('m => null when focused on piano-roll', () => {
      expect(routeShortcut('m', no_mod, 'piano-roll')).toBeNull()
    })
  })
})
