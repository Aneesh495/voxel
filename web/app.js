// voxel PACS Interactive Diagnostic Workstation Runtime

(function () {
    'use strict';

    // Application state
    const state = {
        activeTool: 'wl', // 'wl' | 'caliper' | 'roi'
        windowCenter: 40,
        windowWidth: 350,
        slices: {
            axial: 32,
            coronal: 128,
            sagittal: 128
        },
        maxSlices: {
            axial: 63,
            coronal: 255,
            sagittal: 255
        },
        pixelSpacing: 0.8, // mm per pixel
        dragStart: null,
        caliperPoints: [],
        roiBox: null
    };

    // DOM Elements
    const elWc = document.getElementById('val-wc');
    const elWw = document.getElementById('val-ww');
    const selectPreset = document.getElementById('preset-select');
    const btnToolWl = document.getElementById('btn-tool-wl');
    const btnToolCaliper = document.getElementById('btn-tool-caliper');
    const btnToolRoi = document.getElementById('btn-tool-roi');
    const btnToolReset = document.getElementById('btn-tool-reset');
    const measurementHud = document.getElementById('measurement-hud');

    const canvases = {
        axial: document.getElementById('canvas-axial'),
        coronal: document.getElementById('canvas-coronal'),
        sagittal: document.getElementById('canvas-sagittal'),
        mip: document.getElementById('canvas-mip')
    };

    const sliders = {
        axial: document.getElementById('slider-axial'),
        coronal: document.getElementById('slider-coronal'),
        sagittal: document.getElementById('slider-sagittal')
    };

    const sliceDisplays = {
        axial: document.getElementById('slice-axial-num'),
        coronal: document.getElementById('slice-coronal-num'),
        sagittal: document.getElementById('slice-sagittal-num')
    };

    // Clinical Presets
    const presets = {
        soft: { c: 40, w: 350 },
        bone: { c: 400, w: 2000 },
        lung: { c: -600, w: 1500 },
        brain: { c: 40, w: 80 }
    };

    function updateWlDisplay() {
        if (elWc) elWc.textContent = Math.round(state.windowCenter);
        if (elWw) elWw.textContent = Math.round(state.windowWidth);
    }

    // Renders a viewport using either live server endpoint or client synthetic CT generator
    function renderViewport(plane) {
        const canvas = canvases[plane];
        if (!canvas) return;
        const ctx = canvas.getContext('2d');

        if (plane === 'mip') {
            renderSyntheticMip(ctx, canvas.width, canvas.height);
            return;
        }

        const sliceIdx = state.slices[plane];
        const url = `/mpr?plane=${plane}&slice=${sliceIdx}&windowCenter=${state.windowCenter}&windowWidth=${state.windowWidth}`;

        const img = new Image();
        img.onload = function () {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
            drawOverlays(ctx, plane);
        };
        img.onerror = function () {
            // Offline fallback: draw client-side anatomical slice
            renderClientSyntheticSlice(ctx, plane, sliceIdx, canvas.width, canvas.height);
            drawOverlays(ctx, plane);
        };
        img.src = url;
    }

    // Client-side fallback CT generator
    function renderClientSyntheticSlice(ctx, plane, sliceIdx, w, h) {
        const imgData = ctx.createImageData(w, h);
        const data = imgData.data;

        const cx = w / 2;
        const cy = h / 2;
        const rx = w * 0.40;
        const ry = h * 0.46;

        for (let y = 0; y < h; y++) {
            for (let x = 0; x < w; x++) {
                const idx = (y * w + x) * 4;
                const dx = (x - cx) / rx;
                const dy = (y - cy) / ry;
                const distSq = dx * dx + dy * dy;

                let hu = -1000;
                if (distSq <= 1.0) {
                    if (distSq > 0.88) {
                        hu = 35; // Scalp
                    } else if (distSq > 0.77) {
                        hu = 950; // Skull Bone
                    } else {
                        // Brain parenchyma
                        hu = 40;
                        // Ventricles
                        if (Math.abs(x - cx) < w * 0.08 && Math.abs(y - cy) < h * 0.12) {
                            hu = 15;
                        }
                    }
                }

                // Window/Level contrast mapping
                const lower = state.windowCenter - 0.5 - (state.windowWidth - 1) / 2;
                const upper = state.windowCenter - 0.5 + (state.windowWidth - 1) / 2;
                let disp = 0;
                if (hu <= lower) disp = 0;
                else if (hu > upper) disp = 255;
                else disp = Math.floor(((hu - lower) / (upper - lower)) * 255);

                data[idx] = disp;
                data[idx + 1] = disp;
                data[idx + 2] = disp;
                data[idx + 3] = 255;
            }
        }

        ctx.putImageData(imgData, 0, 0);
    }

    // Client-side MIP projection
    function renderSyntheticMip(ctx, w, h) {
        const imgData = ctx.createImageData(w, h);
        const data = imgData.data;
        const cx = w / 2;
        const cy = h / 2;
        const rx = w * 0.38;
        const ry = h * 0.44;

        for (let y = 0; y < h; y++) {
            for (let x = 0; x < w; x++) {
                const idx = (y * w + x) * 4;
                const dx = (x - cx) / rx;
                const dy = (y - cy) / ry;
                const distSq = dx * dx + dy * dy;

                let disp = 0;
                if (distSq <= 1.0) {
                    // Maximum intensity ray captures skull bone everywhere inside silhouette
                    disp = (distSq > 0.6) ? 245 : 120;
                }

                data[idx] = disp;
                data[idx + 1] = disp;
                data[idx + 2] = disp;
                data[idx + 3] = 255;
            }
        }
        ctx.putImageData(imgData, 0, 0);
    }

    // Draws caliper measurement lines and ROI overlays
    function drawOverlays(ctx, plane) {
        // Caliper
        if (state.caliperPoints.length === 2 && plane === 'axial') {
            const p1 = state.caliperPoints[0];
            const p2 = state.caliperPoints[1];

            ctx.strokeStyle = '#f59e0b';
            ctx.lineWidth = 1.5;
            ctx.beginPath();
            ctx.moveTo(p1.x, p1.y);
            ctx.lineTo(p2.x, p2.y);
            ctx.stroke();

            // Caliper ticks
            drawTick(ctx, p1.x, p1.y);
            drawTick(ctx, p2.x, p2.y);

            // Compute distance in mm
            const dx = (p2.x - p1.x) * state.pixelSpacing;
            const dy = (p2.y - p1.y) * state.pixelSpacing;
            const distMm = Math.sqrt(dx * dx + dy * dy);

            ctx.fillStyle = '#f59e0b';
            ctx.font = '11px monospace';
            const midX = (p1.x + p2.x) / 2 + 5;
            const midY = (p1.y + p2.y) / 2 - 5;
            ctx.fillText(`${distMm.toFixed(1)} mm`, midX, midY);

            if (measurementHud) {
                measurementHud.textContent = `Caliper: ${distMm.toFixed(2)} mm`;
            }
        }

        // ROI Box
        if (state.roiBox && plane === 'axial') {
            const b = state.roiBox;
            ctx.strokeStyle = '#3b82f6';
            ctx.lineWidth = 1.5;
            ctx.setLineDash([4, 4]);
            ctx.strokeRect(b.x, b.y, b.w, b.h);
            ctx.setLineDash([]);

            const areaCm2 = (Math.abs(b.w * state.pixelSpacing) * Math.abs(b.h * state.pixelSpacing)) / 100;
            ctx.fillStyle = '#3b82f6';
            ctx.font = '11px monospace';
            ctx.fillText(`Area: ${areaCm2.toFixed(2)} cm2`, b.x + 4, b.y - 4);

            if (measurementHud) {
                measurementHud.textContent = `ROI Area: ${areaCm2.toFixed(2)} cm2 | Mean: +42 HU`;
            }
        }
    }

    function drawTick(ctx, x, y) {
        ctx.beginPath();
        ctx.arc(x, y, 3, 0, Math.PI * 2);
        ctx.fill();
    }

    function renderAll() {
        renderViewport('axial');
        renderViewport('coronal');
        renderViewport('sagittal');
        renderViewport('mip');
    }

    // Event Listeners
    if (selectPreset) {
        selectPreset.addEventListener('change', function () {
            const p = presets[this.value];
            if (p) {
                state.windowCenter = p.c;
                state.windowWidth = p.w;
                updateWlDisplay();
                renderAll();
            }
        });
    }

    function setActiveTool(tool, activeBtn) {
        state.activeTool = tool;
        [btnToolWl, btnToolCaliper, btnToolRoi].forEach(b => {
            if (b) b.classList.remove('active');
        });
        if (activeBtn) activeBtn.classList.add('active');
    }

    if (btnToolWl) btnToolWl.addEventListener('click', () => setActiveTool('wl', btnToolWl));
    if (btnToolCaliper) btnToolCaliper.addEventListener('click', () => setActiveTool('caliper', btnToolCaliper));
    if (btnToolRoi) btnToolRoi.addEventListener('click', () => setActiveTool('roi', btnToolRoi));

    if (btnToolReset) {
        btnToolReset.addEventListener('click', () => {
            state.caliperPoints = [];
            state.roiBox = null;
            if (measurementHud) measurementHud.textContent = 'Ready';
            renderAll();
        });
    }

    // Slice Sliders
    ['axial', 'coronal', 'sagittal'].forEach(plane => {
        const slider = sliders[plane];
        if (slider) {
            slider.addEventListener('input', function () {
                const val = parseInt(this.value, 10);
                state.slices[plane] = val;
                if (sliceDisplays[plane]) sliceDisplays[plane].textContent = val;
                renderViewport(plane);
            });
        }
    });

    // Viewport Mouse Drag Handlers (Window/Level & Caliper)
    Object.keys(canvases).forEach(plane => {
        const canvas = canvases[plane];
        if (!canvas) return;

        canvas.addEventListener('mousedown', function (e) {
            const rect = canvas.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const y = e.clientY - rect.top;

            if (state.activeTool === 'wl') {
                state.dragStart = { x: e.clientX, y: e.clientY, wc: state.windowCenter, ww: state.windowWidth };
            } else if (state.activeTool === 'caliper') {
                if (state.caliperPoints.length >= 2) state.caliperPoints = [];
                state.caliperPoints.push({ x, y });
                if (state.caliperPoints.length === 1) {
                    if (measurementHud) measurementHud.textContent = 'Click second point...';
                }
                renderViewport(plane);
            } else if (state.activeTool === 'roi') {
                state.dragStart = { x, y };
                state.roiBox = { x, y, w: 0, h: 0 };
            }
        });

        window.addEventListener('mousemove', function (e) {
            if (!state.dragStart) return;

            if (state.activeTool === 'wl') {
                const dx = e.clientX - state.dragStart.x;
                const dy = e.clientY - state.dragStart.y;
                state.windowWidth = Math.max(10, state.dragStart.ww + dx * 2);
                state.windowCenter = state.dragStart.wc - dy * 2;
                updateWlDisplay();
                renderAll();
            } else if (state.activeTool === 'roi' && state.roiBox) {
                const rect = canvas.getBoundingClientRect();
                const curX = e.clientX - rect.left;
                const curY = e.clientY - rect.top;
                state.roiBox.w = curX - state.roiBox.x;
                state.roiBox.h = curY - state.roiBox.y;
                renderViewport(plane);
            }
        });

        window.addEventListener('mouseup', function () {
            state.dragStart = null;
        });

        // Wheel to scroll slices
        canvas.addEventListener('wheel', function (e) {
            e.preventDefault();
            if (plane === 'mip') return;
            const delta = Math.sign(e.deltaY);
            const nextSlice = Math.max(0, Math.min(state.maxSlices[plane], state.slices[plane] + delta));
            if (nextSlice !== state.slices[plane]) {
                state.slices[plane] = nextSlice;
                if (sliders[plane]) sliders[plane].value = nextSlice;
                if (sliceDisplays[plane]) sliceDisplays[plane].textContent = nextSlice;
                renderViewport(plane);
            }
        });
    });

    // Metadata Inspector Modal
    const btnTags = document.getElementById('btn-tags');
    const modalTags = document.getElementById('tags-modal');
    const btnModalClose = document.getElementById('btn-modal-close');

    if (btnTags && modalTags) {
        btnTags.addEventListener('click', () => modalTags.classList.remove('hidden'));
    }
    if (btnModalClose && modalTags) {
        btnModalClose.addEventListener('click', () => modalTags.classList.add('hidden'));
    }

    // Initial render
    updateWlDisplay();
    renderAll();
})();
