/**
 * @type {HTMLDivElement}
 */
const activateModal = document.getElementById('activation-modal');
/**
 * @type {HTMLDivElement}
 */
const dropZone = document.getElementById('license-dropzone');
/**
 * @type {HTMLInputElement}
 */
const fileInput = document.getElementById('license-file-input');
/**
 * @type {HTMLTextAreaElement}
 */
const manualInput = document.getElementById('license-text-input');
/**
 * @type {HTMLAnchorElement}
 */
const browseBtn = document.getElementById('license-browse-btn');
/**
 * @type {HTMLButtonElement}
 */
const activateBtn = document.getElementById('activate-license-btn');
/**
 * @type {HTMLButtonElement}
 */
const evalBtn = document.getElementById('continue-evaluating-btn');

browseBtn.onclick = () => {
    window.__JUCE__.backend.emitEvent("openLicenseFilePicker");
};

dropZone.addEventListener('dragover', e => {
    e.preventDefault();
    dropZone.classList.add('dragging');
});
dropZone.addEventListener('dragleave', () => dropZone.classList.remove('dragging'));
dropZone.addEventListener('drop', e => {
    e.preventDefault();
    dropZone.classList.remove('dragging');
    const file = e.dataTransfer.files[0];
    if (file) {
        readFile(file);
    }
});

fileInput.addEventListener('change', () => {
    if (fileInput.files[0]) {
        readFile(fileInput.files[0]);
    }
});

manualInput.addEventListener('input', () => {
    activateBtn.disabled = manualInput.value.trim().length === 0;
});

function readFile(file) {
    readFileToString(file, (content) => {
        manualInput.value = content;
        const event = new Event('input', {bubbles: true});
        manualInput.dispatchEvent(event);
    });
}

activateBtn.onclick = () => {
    const content = manualInput.value.trim();
    const json = JSON.parse(content);
    window.__JUCE__.backend.emitEvent("activateLicense", JSON.stringify(json));
};

evalBtn.onclick = () => {
    activateModal.classList.remove('visible');
}

document.getElementById('menu-dropdown-btn').onclick = () => {
    activateModal.classList.add('visible');
}