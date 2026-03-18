// One entry per oscillator — keyed by osc ID (e.g. "osc1", "osc2")
const wavetableDisplays = new Map();

function getOrCreateDisplay(oscId) {
    if (!wavetableDisplays.has(oscId)) {
        const canvas = document.querySelector(`[data-wavetable-display="${oscId}"]`);
        if (!canvas) {
            console.warn(`No wavetable canvas found for oscId: ${oscId}`);
            return null;
        }

        const display = {
            canvas,
            ctx: canvas.getContext("2d"),
            frames: null,
            position: 0.0,
        };
        wavetableDisplays.set(oscId, display);

        // Keep canvas resolution in sync with its CSS/layout width
        new ResizeObserver(() => {
            canvas.width = canvas.clientWidth;
            drawWavetable(oscId);
        }).observe(canvas);
    }
    return wavetableDisplays.get(oscId);
}

// Called by JUCE bridge when frame data arrives
function setWavetableFrames(oscId, frames) {
    const display = getOrCreateDisplay(oscId);
    if (!display) return;
    display.frames = frames;
    drawWavetable(oscId);
}

// Called by JUCE bridge when the position knob moves
function setWavetablePosition(oscId, position) {
    const display = getOrCreateDisplay(oscId);
    if (!display) return;
    display.position = position;
    drawWavetable(oscId);
}

function drawWavetable(oscId) {
    const borderRadius = 0;

    const display = getOrCreateDisplay(oscId);
    if (!display) return;

    const {canvas, ctx, frames, position} = display;
    const w = canvas.width;
    const h = canvas.height;
    const pad = 0;

    ctx.clearRect(0, 0, w, h);

    // --- Background ---------------------------------------------------------
    ctx.fillStyle = getCSSVar(canvas, "display");
    roundRect(ctx, pad, pad, w - pad * 2, h - pad * 2, borderRadius);
    ctx.fill();

    // --- Border -------------------------------------------------------------
    // ctx.strokeStyle = getCSSVar(canvas, "border");
    // ctx.lineWidth = 1;
    // roundRect(ctx, pad, pad, w - pad * 2, h - pad * 2, borderRadius);
    // ctx.stroke();

    if (!frames || frames.length === 0) {
        ctx.fillStyle = getCSSVar(canvas, "text-disabled");
        ctx.font = "13px -apple-system, sans-serif";
        ctx.textAlign = "center";
        ctx.fillText("No wavetable loaded", w / 2, h / 2 + 5);
        return;
    }

    const numFrames = frames.length;
    const framePos = position * (numFrames - 1);
    const f0 = Math.floor(framePos) % numFrames;
    const f1 = (f0 + 1) % numFrames;
    const frac = framePos - Math.floor(framePos);

    const cx = w / 2;
    const cy = h / 2;
    const halfH = (h - pad * 2) * 0.42;
    const left = pad;
    const right = w - pad;
    const drawPoints = right - left;

    // --- Build waveform path ------------------------------------------------
    ctx.beginPath();
    for (let px = 0; px < drawPoints; px++) {
        const phase = px / drawPoints;
        const s = sampleFrame(frames[f0], phase) + frac * (sampleFrame(frames[f1], phase) - sampleFrame(frames[f0], phase));
        const x = left + px;
        const y = cy - s * halfH;
        px === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
    }

    // --- Gradient fill under wave -------------------------------------------
    const fillGrad = ctx.createLinearGradient(0, pad, 0, h - pad);
    const waveColor = getCSSVar(canvas, "primary-accent");
    fillGrad.addColorStop(0, convertHexToRGBA(waveColor, 0.25));
    fillGrad.addColorStop(1, convertHexToRGBA(waveColor, 0.00));

    ctx.lineTo(right, cy);
    ctx.lineTo(left, cy);
    ctx.closePath();
    ctx.fillStyle = fillGrad;
    ctx.fill();

    // --- Waveform line ------------------------------------------------------
    ctx.beginPath();
    for (let px = 0; px < drawPoints; px++) {
        const phase = px / drawPoints;
        const s = sampleFrame(frames[f0], phase) + frac * (sampleFrame(frames[f1], phase) - sampleFrame(frames[f0], phase));
        const x = left + px;
        const y = cy - s * halfH;
        px === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
    }
    ctx.strokeStyle = waveColor;
    ctx.lineWidth = 1.5;
    ctx.lineJoin = "round";
    ctx.stroke();

    // --- Frame label --------------------------------------------------------
    const frameIdx = Math.round(position * (numFrames - 1));
    ctx.fillStyle = getCSSVar(canvas, "text-disabled");
    ctx.font = "11px -apple-system, sans-serif";
    ctx.textAlign = "right";
    ctx.fillText("Frame " + (frameIdx + 1), right - 6, pad + 14);
}

document.querySelectorAll("[data-wavetable-display]").forEach(canvas => {
    const oscId = canvas.dataset.wavetableDisplay;
    getOrCreateDisplay(oscId);  // registers the canvas in the Map
    drawWavetable(oscId);       // draws the placeholder
});