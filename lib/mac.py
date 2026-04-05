#!/usr/bin/env python3
"""mac.py — macOS native GUI automation via CoreGraphics.
No accessibility permission needed for CGEvent mouse/keyboard.

IMPORTANT — what works and what doesn't:
  screenshot()  — always works, no permissions needed
  activate()    — always works, brings app to front
  click(x,y)    — works but NEVER guess coordinates. Use aim() first
  type_text()   — works, sends keystrokes to frontmost app
  key()         — works, sends special keys (return, tab, etc)
  windows()     — needs accessibility permission for window details

NEVER calculate/estimate click coordinates from screenshots.
Coordinate guessing will never reliably hit the right element.
Instead: use aim() to draw a red dot on the screenshot, LLM confirms
the dot is on target, THEN click. Adjust via crop() if off.

Usage: python3 mac.py screenshot | aim X Y | click X Y | type "text"

Proven workflow for clicking GUI elements:
  1. activate(app)           — bring app to front
  2. screenshot()            — capture screen
  3. crop(path, x,y,w,h)    — zoom into region of interest
  4. aim(x, y)               — draw green crosshair+dot on screenshot
  5. crop the aim image      — zoom in to verify dot is on target
  6. LLM reviews cropped aim — confirms dot is on correct element
  7. If off: adjust x,y and re-aim. If on target: click(x, y)
  8. screenshot() after click — verify result

NEVER skip steps 3-6. Guessing coordinates without visual confirmation
will miss the target nearly every time. The crop+aim loop is fast and
reliable; blind coordinate math is not.
"""
import subprocess, sys, os, time

def _cg():
    from Quartz import (CGEventCreateMouseEvent, CGEventCreateKeyboardEvent,
        CGEventPost, CGEventSetFlags, CGEventKeyboardSetUnicodeString,
        kCGEventLeftMouseDown, kCGEventLeftMouseUp, kCGEventMouseMoved,
        kCGHIDEventTap, CGPointMake, kCGEventKeyDown, kCGEventKeyUp,
        kCGEventFlagsChanged, CGDisplayBounds, CGMainDisplayID)
    return type('CG', (), {k: v for k, v in locals().items()})

def screen_scale():
    """Retina scale factor: screenshot_pixels / logical_points."""
    cg = _cg()
    b = cg.CGDisplayBounds(cg.CGMainDisplayID())
    w_logical = int(b.size.width)
    r = subprocess.run(['screencapture', '-x', '/tmp/_mac_scale.png'], capture_output=True)
    import struct
    with open('/tmp/_mac_scale.png', 'rb') as f:
        f.read(16); w_px = struct.unpack('>I', f.read(4))[0]
    os.unlink('/tmp/_mac_scale.png')
    return w_px / w_logical

def screenshot(path='/tmp/mac_screen.png'):
    subprocess.run(['screencapture', '-x', path], check=True)
    return path

def activate(app):
    subprocess.run(['osascript', '-e', f'tell application "{app}" to activate'], check=True)
    time.sleep(0.3)

def aim(x, y, path='/tmp/mac_screen.png', out='/tmp/mac_aim.png', radius=12):
    """Draw red dot at logical point (x,y) on screenshot. LLM reviews before clicking.
    Screenshots are in Retina pixels, so we scale x,y up by screen_scale().
    Returns path to annotated image."""
    scale = screen_scale()
    px, py = int(x * scale), int(y * scale)
    # Draw red dot using sips + temp overlay, or raw PNG manipulation
    # Simplest: use python to draw circle
    subprocess.run(['cp', path, out], check=True)
    _draw_dot(out, px, py, int(radius * scale))
    return out

def _draw_dot(png_path, px, py, r):
    """Draw a red filled circle on a PNG via CoreGraphics."""
    try:
        from Quartz import (CGImageSourceCreateWithURL, CGImageDestinationCreateWithURL,
            CGImageSourceCreateImageAtIndex, CGBitmapContextCreate, CGBitmapContextCreateImage,
            CGContextSetRGBFillColor, CGContextFillEllipseInRect, CGContextDrawImage,
            kCGImageAlphaPremultipliedLast, CGRectMake, CGImageGetWidth, CGImageGetHeight,
            CGImageGetColorSpace, CGImageDestinationAddImage, CGImageDestinationFinalize)
        from Foundation import CFURLCreateWithFileSystemPath, kCFURLPOSIXPathStyle, kCFAllocatorDefault
        url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, png_path, kCFURLPOSIXPathStyle, False)
        src = CGImageSourceCreateWithURL(url, None)
        img = CGImageSourceCreateImageAtIndex(src, 0, None)
        w, h = CGImageGetWidth(img), CGImageGetHeight(img)
        cs = CGImageGetColorSpace(img)
        ctx = CGBitmapContextCreate(None, w, h, 8, w * 4, cs, kCGImageAlphaPremultipliedLast)
        CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), img)
        # Crosshairs: full-width horizontal + full-height vertical lines
        from Quartz import CGContextFillRect
        CGContextSetRGBFillColor(ctx, 0.0, 1.0, 0.0, 0.7)
        CGContextFillRect(ctx, CGRectMake(0, h - py - 1, w, 3))  # horizontal
        CGContextFillRect(ctx, CGRectMake(px - 1, 0, 3, h))      # vertical
        # Dot at intersection
        CGContextSetRGBFillColor(ctx, 0.0, 1.0, 0.0, 1.0)
        CGContextFillEllipseInRect(ctx, CGRectMake(px - r, h - py - r, r * 2, r * 2))
        CGContextSetRGBFillColor(ctx, 0.0, 0.0, 0.0, 1.0)
        CGContextFillEllipseInRect(ctx, CGRectMake(px - 4, h - py - 4, 8, 8))
        result = CGBitmapContextCreateImage(ctx)
        dest = CGImageDestinationCreateWithURL(url, 'public.png', 1, None)
        CGImageDestinationAddImage(dest, result, None)
        CGImageDestinationFinalize(dest)
    except Exception as e:
        print(f'dot draw failed: {e}', file=sys.stderr)

def crop(path, x, y, w, h, out='/tmp/mac_crop.png'):
    """Crop screenshot region for closer inspection. Coords in logical points."""
    scale = screen_scale()
    sx, sy, sw, sh = int(x*scale), int(y*scale), int(w*scale), int(h*scale)
    subprocess.run(['sips', '-c', str(sh), str(sw), '--cropOffset', str(sy), str(sx),
                    path, '--out', out], capture_output=True)
    return out

def click(x, y, pause=0.05):
    """Click at logical point. NEVER call without aim() confirmation first."""
    cg = _cg()
    p = cg.CGPointMake(x, y)
    for t in (cg.kCGEventLeftMouseDown, cg.kCGEventLeftMouseUp):
        e = cg.CGEventCreateMouseEvent(None, t, p, 0)
        cg.CGEventPost(cg.kCGHIDEventTap, e)
        time.sleep(pause)

def type_text(text, delay=0.02):
    """Type text into frontmost app. Works without accessibility."""
    cg = _cg()
    for ch in text:
        e = cg.CGEventCreateKeyboardEvent(None, 0, True)
        cg.CGEventKeyboardSetUnicodeString(e, len(ch), ch)
        cg.CGEventPost(cg.kCGHIDEventTap, e)
        e2 = cg.CGEventCreateKeyboardEvent(None, 0, False)
        cg.CGEventKeyboardSetUnicodeString(e2, len(ch), ch)
        cg.CGEventPost(cg.kCGHIDEventTap, e2)
        time.sleep(delay)

def key(name):
    """Press special key. Names: return tab escape delete space up down left right."""
    codes = {'return': 36, 'tab': 48, 'escape': 53, 'delete': 51, 'space': 49,
             'up': 126, 'down': 125, 'left': 123, 'right': 124}
    code = codes.get(name.lower(), 0)
    cg = _cg()
    for down in (True, False):
        e = cg.CGEventCreateKeyboardEvent(None, code, down)
        cg.CGEventPost(cg.kCGHIDEventTap, e)
        time.sleep(0.02)

def select_all():
    """Cmd+A."""
    cg = _cg()
    e = cg.CGEventCreateKeyboardEvent(None, 0, True)
    cg.CGEventSetFlags(e, 1 << 20)
    cg.CGEventPost(cg.kCGHIDEventTap, e)
    e2 = cg.CGEventCreateKeyboardEvent(None, 0, False)
    cg.CGEventPost(cg.kCGHIDEventTap, e2)
    time.sleep(0.05)

def fill(x, y, text):
    """Click field then type. NEVER call without aim() confirmation first."""
    click(x, y)
    time.sleep(0.1)
    select_all()
    type_text(text)

def windows(app=None):
    """List windows. Needs accessibility for window details."""
    try:
        r = subprocess.run(['osascript', '-e',
            f'tell application "System Events" to get {{position, size}} of every window of process "{app}"'
            if app else
            'tell application "System Events" to get name of every process whose visible is true'],
            capture_output=True, text=True, timeout=5)
        return r.stdout.strip()
    except: return ''

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('mac.py screenshot | aim X Y | crop X Y W H | click X Y | type "text" | key name | activate app')
        sys.exit(1)
    cmd = sys.argv[1]
    if cmd == 'screenshot': print(screenshot(sys.argv[2] if len(sys.argv) > 2 else '/tmp/mac_screen.png'))
    elif cmd == 'aim': print(aim(float(sys.argv[2]), float(sys.argv[3])))
    elif cmd == 'crop': print(crop(sys.argv[2], float(sys.argv[3]), float(sys.argv[4]), float(sys.argv[5]), float(sys.argv[6])))
    elif cmd == 'click': click(float(sys.argv[2]), float(sys.argv[3]))
    elif cmd == 'type': type_text(sys.argv[2])
    elif cmd == 'key': key(sys.argv[2])
    elif cmd == 'activate': activate(sys.argv[2])
    elif cmd == 'fill': fill(float(sys.argv[2]), float(sys.argv[3]), sys.argv[4])
    elif cmd == 'windows': print(windows(sys.argv[2] if len(sys.argv) > 2 else None))
    else: print(f'unknown: {cmd}')
