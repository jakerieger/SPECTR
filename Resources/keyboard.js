/** @type {HTMLCanvasElement} */
const keyboardCanvas = document.getElementById('keyboard');

const getColor = (name) => getCSSVar(keyboardCanvas, name);

// 88 keys: MIDI 21 (A0) → 108 (C8)
const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
const BLACK_OFFSETS = {'C#': 0.65, 'D#': 1.45, 'F#': 3.65, 'G#': 4.45, 'A#': 5.25}

function buildKeys() {
    const white = [], black = [];
    for (let midi = 21; midi <= 108; midi++) {
        const note = NOTE_NAMES[midi % 12];
        const octave = Math.floor((midi - 12) / 12);
        const key = {midi, note, octave, name: note + octave};
        (note.includes('#') ? black : white).push(key);
    }
    return {white, black};
}

function layoutKeys(white, black, canvasW, canvasH) {
    const wW = canvasW / 52;   // white key width
    const bW = wW * 0.58;
    const bH = canvasH * 0.62;

    white.forEach((k, i) => {
        k.x = i * wW;
        k.w = wW;
        k.h = canvasH;
    });

    black.forEach(k => {
        const offset = BLACK_OFFSETS[k.note];
        if (offset == null) return;
        const cIdx = white.findIndex(w => w.note === 'C' && w.octave === k.octave);
        if (cIdx === -1) return;
        k.x = (cIdx + offset) * wW - bW / 2;
        k.w = bW;
        k.h = bH;
    });
}

function drawPiano(canvas, {white, black}, pressedMidis = new Set()) {
    const ctx = canvas.getContext('2d');
    const dpr = devicePixelRatio;

    canvas.width = canvas.clientWidth * dpr;
    canvas.height = canvas.clientHeight * dpr;
    ctx.scale(dpr, dpr);

    layoutKeys(white, black, canvas.clientWidth, canvas.clientHeight);

    // White keys
    for (const k of white) {
        ctx.fillStyle = pressedMidis.has(k.midi) ? getColor('text-primary') : getColor('text-disabled');
        ctx.strokeStyle = getColor('background');
        ctx.lineWidth = 0.5;
        ctx.beginPath();
        ctx.roundRect(k.x + 0.5, 0.5, k.w - 1, k.h - 1, [0, 0, 3, 3]);
        ctx.fill();
        ctx.stroke();
    }

    // Black keys
    for (const k of black) {
        if (k.x == null) continue;
        ctx.fillStyle = pressedMidis.has(k.midi) ? getColor('text-primary') : getColor('display-secondary');
        ctx.beginPath();
        ctx.roundRect(k.x, 0, k.w, k.h, [0, 0, 3, 3]);
        ctx.fill();
    }
}

function getKeyAtPoint(x, y, {white, black}) {
    // Black keys are visually on top — check first
    for (const k of black) {
        if (k.x != null && x >= k.x && x <= k.x + k.w && y <= k.h) return k;
    }
    for (const k of white) {
        if (x >= k.x && x <= k.x + k.w) return k;
    }
    return null;
}

const keys = buildKeys();
const pressed = new Set();

new ResizeObserver(() => drawPiano(keyboardCanvas, keys, pressed)).observe(keyboardCanvas);

let isMouseDown = false;
let activeKey = null;

function pressKey(key) {
    if (key === activeKey) return;
    releaseKey();
    if (!key) return;
    activeKey = key;
    pressed.add(key.midi);
    // noteOn(key.midi);
    drawPiano(keyboardCanvas, keys, pressed);
}

function releaseKey() {
    if (!activeKey) return;
    pressed.delete(activeKey.midi);
    // noteOff(activeKey.midi);
    activeKey = null;
    drawPiano(keyboardCanvas, keys, pressed);
}

function keyFromEvent(e) {
    const rect = keyboardCanvas.getBoundingClientRect();
    return getKeyAtPoint(e.clientX - rect.left, e.clientY - rect.top, keys);
}

keyboardCanvas.addEventListener('mousedown', e => {
    isMouseDown = true;
    pressKey(keyFromEvent(e));
});

keyboardCanvas.addEventListener('mousemove', e => {
    if (!isMouseDown) return;
    pressKey(keyFromEvent(e));  // no-op if same key, switches if different
});

keyboardCanvas.addEventListener('mouseup', () => {
    isMouseDown = false;
    releaseKey();
});

keyboardCanvas.addEventListener('mouseleave', () => {
    // cursor left the canvas entirely — kill the note
    isMouseDown = false;
    releaseKey();
});

// Safety net: mouse released outside the canvas
window.addEventListener('mouseup', () => {
    if (!isMouseDown) return;
    isMouseDown = false;
    releaseKey();
});