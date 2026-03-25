//===== HELPER FUNCTIONS =====//

function getCSSVar(canvas, name) {
    return getComputedStyle(canvas).getPropertyValue(`--${name}`);
}

function convertHexToRGBA(hexCode, opacity = 1) {
    let hex = hexCode.replace('#', '');

    if (hex.length === 3) {
        hex = `${hex[0]}${hex[0]}${hex[1]}${hex[1]}${hex[2]}${hex[2]}`;
    }

    const r = parseInt(hex.substring(0, 2), 16);
    const g = parseInt(hex.substring(2, 4), 16);
    const b = parseInt(hex.substring(4, 6), 16);

    /* Backward compatibility for whole number based opacity values. */
    if (opacity > 1 && opacity <= 100) {
        opacity = opacity / 100;
    }

    return `rgba(${r},${g},${b},${opacity})`;
}

// Linear interpolation within a single frame (mirrors getSample)
function sampleFrame(frame, phase) {
    const len = frame.length;
    const pos = phase * (len - 1);
    const i0 = Math.floor(pos) % len;
    const i1 = (i0 + 1) % len;
    const t = pos - Math.floor(pos);
    return frame[i0] + t * (frame[i1] - frame[i0]);
}

// Helper — CanvasRenderingContext2D rounded rect (pre-roundRect API compat)
function roundRect(ctx, x, y, w, h, r) {
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.lineTo(x + w - r, y);
    ctx.arcTo(x + w, y, x + w, y + r, r);
    ctx.lineTo(x + w, y + h - r);
    ctx.arcTo(x + w, y + h, x + w - r, y + h, r);
    ctx.lineTo(x + r, y + h);
    ctx.arcTo(x, y + h, x, y + h - r, r);
    ctx.lineTo(x, y + r);
    ctx.arcTo(x, y, x + r, y, r);
    ctx.closePath();
}

/**
 * @param {Blob} file
 * @param {function(string)} result
 * @returns {string}
 */
function readFileToString(file, result) {
    const r = new FileReader();
    r.onload = e => {
        result(e.target.result);
    }
    r.readAsText(file);
}

//===== Bridge: JS → C++ =====//
function sendToPlugin(paramId, value) {
    if (window.__JUCE__?.backend)
        window.__JUCE__.backend.emitEvent("setParam", {paramId, value});
}

//===== Bridge: C++ → JS =====//
// Called by C++: webView.emitEventIfBrowserIsVisible("paramChanged", ...)
function setupJUCEListeners() {
    if (!window.__JUCE__?.backend) return;

    document.querySelectorAll("[data-load-wavetable]").forEach((btn) => {
        const oscId = btn.getAttribute("data-load-wavetable");
        btn.addEventListener("click", () => {
            window.__JUCE__.backend.emitEvent("openFilePicker", {oscId: oscId});
        });
    });

    window.__JUCE__.backend.addEventListener("activateLicense", (data) => {
        // `data` contains license info
        if (data.valid) {
            alert
            ("Valid license file: " + JSON.stringify(data));
        } else {
            alert("License file is not valid");
        }
    });

    window.__JUCE__.backend.addEventListener("getLicenseInfo", (data) => {
        // `data` contains license info
    });

    window.__JUCE__.backend.addEventListener("wavetableData", (data) => {
        setWavetableFrames(data.oscId, data.frames, data.wtName);
    });

    window.__JUCE__.backend.addEventListener("wavetablePosition", (data) => {
        setWavetablePosition(data.oscId, data.position);
    });

    window.__JUCE__.backend.addEventListener("paramChanged", data => {
        if (data.paramId === "framePos") {
            setWavetablePosition("osc1", data.value); // hard-coded for now
        } else {
            setKnobValue(data.paramId, data.value);
        }
    });
}

document.querySelectorAll('.radio-button').forEach((btn) => {
    btn.addEventListener('click', () => btn.classList.toggle('active'));
});

// __JUCE__ is injected asynchronously — wait for it if not yet available
if (window.__JUCE__?.backend) {
    setupJUCEListeners();
} else {
    window.addEventListener("juceReady", setupJUCEListeners, {once: true});
}