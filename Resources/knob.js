// ─── Constants ────────────────────────────────────────────────────────────
const MIN_ANGLE = -135 * (Math.PI / 180);   // -135° in radians
const MAX_ANGLE = 135 * (Math.PI / 180);   //  135° in radians
const DRAG_PX = 200;                       // px of drag = full 0→1 sweep

// ─── Draw a single knob onto its canvas ──────────────────────────────────
function drawKnob(canvas, value) {
    const dpr = window.devicePixelRatio || 1;
    const s = canvas.width;          // native pixel size (160 at 2x)
    const cx = s / 2;
    const cy = s / 2;
    const r = s * 0.38;              // outer radius
    const ctx = canvas.getContext("2d");

    ctx.clearRect(0, 0, s, s);

    // --- Track arc (full sweep, dim) ------------------------------------
    const trackWidth = s * 0.03;
    ctx.beginPath();
    ctx.arc(cx, cy, r, MIN_ANGLE - Math.PI * 0.5, MAX_ANGLE - Math.PI * 0.5);
    ctx.strokeStyle = getCSSVar(canvas, "input");
    ctx.lineWidth = trackWidth;
    ctx.lineCap = "round";
    ctx.stroke();

    // --- Value arc (filled portion) -------------------------------------
    const valueAngle = MIN_ANGLE + value * (MAX_ANGLE - MIN_ANGLE);
    ctx.beginPath();
    ctx.arc(cx, cy, r, MIN_ANGLE - Math.PI * 0.5, valueAngle - Math.PI * 0.5);
    ctx.strokeStyle = getCSSVar(canvas, "primary-accent");
    ctx.lineWidth = trackWidth;
    ctx.lineCap = "round";
    ctx.stroke();

    // --- Body circle ----------------------------------------------------
    const bodyR = r * 0.80;
    ctx.beginPath();
    ctx.arc(cx, cy, bodyR, 0, Math.PI * 2);
    ctx.fillStyle = getCSSVar(canvas, "input");
    ctx.fill();

    // Rim for body circle
    // ctx.strokeStyle = "#444";
    // ctx.lineWidth = s * 0.015;
    // ctx.stroke();

    // --- Indicator line --------------------------------------------------
    const dotAngle = valueAngle - Math.PI * 0.5;
    const lineInner = bodyR * 0.25;
    const lineOuter = bodyR * 0.65;
    ctx.beginPath();
    ctx.moveTo(cx + Math.cos(dotAngle) * lineInner, cy + Math.sin(dotAngle) * lineInner);
    ctx.lineTo(cx + Math.cos(dotAngle) * lineOuter, cy + Math.sin(dotAngle) * lineOuter);
    ctx.strokeStyle = getCSSVar(canvas, "input-border");
    ctx.lineWidth = s * 0.02;
    ctx.lineCap = "round";
    ctx.stroke();
}

// ─── Initialize all canvases and set starting visuals ────────────────────
const knobs = {};   // keyed by paramId

document.querySelectorAll("canvas[data-param]").forEach(canvas => {
    const paramId = canvas.dataset.param;
    const value = parseFloat(canvas.dataset.value);

    knobs[paramId] = {canvas, value};
    drawKnob(canvas, value);
});

// ─── Update knob value + label display ───────────────────────────────────
function setKnobValue(paramId, value, redraw = true) {
    value = Math.max(0, Math.min(1, value));
    const entry = knobs[paramId];
    if (!entry) return;
    entry.value = value;
    if (redraw) drawKnob(entry.canvas, value);

    const valEl = document.getElementById("val-" + paramId);
    if (valEl) valEl.textContent = value.toFixed(2);
}

// ─── Drag handling ───────────────────────────────────────────────────────
let drag = null;   // { paramId, startY, startVal }

document.querySelectorAll("canvas[data-param]").forEach(canvas => {
    canvas.addEventListener("pointerdown", e => {
        const paramId = canvas.dataset.param;
        drag = {
            paramId,
            startY: e.clientY,
            startVal: knobs[paramId].value
        };
        canvas.setPointerCapture(e.pointerId);
        e.preventDefault();
    });

    // Double-click resets to 0.5
    canvas.addEventListener("dblclick", () => {
        const paramId = canvas.dataset.param;
        setKnobValue(paramId, 0.5);
        sendToPlugin(paramId, 0.5);
    });
});

window.addEventListener("pointermove", e => {
    if (!drag) return;
    const delta = (drag.startY - e.clientY) / DRAG_PX;
    const newVal = Math.max(0, Math.min(1, drag.startVal + delta));
    setKnobValue(drag.paramId, newVal);
    sendToPlugin(drag.paramId, newVal);
});

window.addEventListener("pointerup", () => {
    drag = null;
});