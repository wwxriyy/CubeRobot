/*
 * CubeRobot Control Panel – Client Logic.
 * Version 6.0 - Emotions: Neutral, Sleep, Angry, Surprise, Hearts, Shy, Happy, Sad, Cyclops.
 * Copyright (c) 2025 @wwxriyy (Instagram / Telegram)
 * MIT License
 */

const ip = "192.168.1.33"; // Replace with your ESP's IP address

// ----- Lighting Control Elements -----
const powerCheckbox = document.getElementById('powerCheckbox');
const modeSelect = document.getElementById('modeSelect');
const redSlider = document.getElementById('redSlider');
const greenSlider = document.getElementById('greenSlider');
const blueSlider = document.getElementById('blueSlider');
const redValueSpan = document.getElementById('redValue');
const greenValueSpan = document.getElementById('greenValue');
const blueValueSpan = document.getElementById('blueValue');
const colorPreview = document.getElementById('colorPreview');

// ----- Light Sensor Elements -----
const ldrEnableCheckbox = document.getElementById('ldrEnableCheckbox');
const ldrThresholdSlider = document.getElementById('ldrThresholdSlider');
const ldrThresholdValue = document.getElementById('ldrThresholdValue');
const lightLevelBar = document.getElementById('lightLevelBar');
const lightLevelValue = document.getElementById('lightLevelValue');

// ----- DHT11 Climate Elements -----
const dhtEnableCheckbox = document.getElementById('dhtEnableCheckbox');
const temperatureValue = document.getElementById('temperatureValue');
const humidityValue = document.getElementById('humidityValue');
const eyePreviewGrid = document.getElementById('eyePreviewGrid');

// ----- Hardware Status Elements -----
const hwWifi = document.getElementById('hwWifi');
const hwDht = document.getElementById('hwDht');
const hwLed = document.getElementById('hwLed');
const hwFps = document.getElementById('hwFps');
const hwLight = document.getElementById('hwLight');
const hwUptime = document.getElementById('hwUptime');

// ----- Emotion Elements -----
const emotionPrev = document.getElementById('emotionPrev');
const emotionNext = document.getElementById('emotionNext');
const currentEmotionSpan = document.getElementById('currentEmotion');
const eyeStateDisplay = document.getElementById('eyeStateDisplay');
const emotionButtonsContainer = document.getElementById('emotionButtons');
const autoMoodCheckbox = document.getElementById('autoMoodCheckbox');

// ----- Mood Timer Elements -----
const moodMinSlider = document.getElementById('moodMinSlider');
const moodMaxSlider = document.getElementById('moodMaxSlider');
const moodMinValue = document.getElementById('moodMinValue');
const moodMaxValue = document.getElementById('moodMaxValue');

// ----- Blink Interval Elements -----
const blinkMinSlider = document.getElementById('blinkMinSlider');
const blinkMaxSlider = document.getElementById('blinkMaxSlider');
const blinkMinValue = document.getElementById('blinkMinValue');
const blinkMaxValue = document.getElementById('blinkMaxValue');

// ----- Eye Animation Elements -----
const levitationEnable = document.getElementById('levitationEnable');
const levitationSpeed = document.getElementById('levitationSpeed');
const levitationSpeedValue = document.getElementById('levitationSpeedValue');
const levitationAmp = document.getElementById('levitationAmp');
const levitationAmpValue = document.getElementById('levitationAmpValue');
const lookAroundEnable = document.getElementById('lookAroundEnable');
const lookSpeed = document.getElementById('lookSpeed');
const lookSpeedValue = document.getElementById('lookSpeedValue');
const lookAmp = document.getElementById('lookAmp');
const lookAmpValue = document.getElementById('lookAmpValue');

// ----- FPS Elements -----
const fpsValue = document.getElementById('fpsValue');
const fpsBar = document.getElementById('fpsBar');

// ----- Common Elements -----
const statusDot = document.getElementById('status-dot');
const statusText = document.getElementById('status-text');

// Current RGB values
let currentRed = 0, currentGreen = 0, currentBlue = 0;

// Emotion mapping
const emotionModes = ['neutral', 'sleep', 'angry', 'surprise', 'hearts', 'shy', 'happy', 'sad', 'cyclops'];
const emotionNames = ['Neutral', 'Sleep', 'Angry', 'Surprise', 'Hearts', 'Shy', 'Happy', 'Sad', 'Cyclops'];
let currentEmotionIndex = 0;
let lastFps = 0;
let lastLight = 0;

// Debounce timer for emotion buttons
let emotionDebounceTimer = null;

// ============================================================
// Lighting Functions (Power LED does NOT affect animations)
// ============================================================
function updateColorPreview() {
    colorPreview.style.backgroundColor = `rgb(${currentRed}, ${currentGreen}, ${currentBlue})`;
    colorPreview.style.opacity = powerCheckbox.checked ? "1" : "0.5";
}
function setRed(v) {
    currentRed = Number(v);
    redValueSpan.textContent = v;
    updateColorPreview();
    fetch(`http://${ip}/red?value=` + v).catch(() => {});
}
function setGreen(v) {
    currentGreen = Number(v);
    greenValueSpan.textContent = v;
    updateColorPreview();
    fetch(`http://${ip}/green?value=` + v).catch(() => {});
}
function setBlue(v) {
    currentBlue = Number(v);
    blueValueSpan.textContent = v;
    updateColorPreview();
    fetch(`http://${ip}/blue?value=` + v).catch(() => {});
}
function toggleLED(state) {
    // Only send power command to ESP, do not interfere with animations
    fetch(`http://${ip}/power?state=` + (state ? 1 : 0)).catch(() => {});
    updateColorPreview();
    // No extra actions
}
function setMode(mode) {
    fetch(`http://${ip}/mode?value=` + mode).catch(() => {});
}

// ============================================================
// Light Sensor Functions
// ============================================================
function setLdrEnable(enable) {
    fetch(`http://${ip}/ldr?enable=` + (enable ? 1 : 0)).catch(() => {});
}
function setLdrThreshold(val) {
    ldrThresholdValue.textContent = val;
    fetch(`http://${ip}/ldr?threshold=` + val).catch(() => {});
}

// ============================================================
// DHT11 Climate Functions
// ============================================================
function setDhtEnable(enable) {
    updateClimateValues({ enabled: enable, temperature: null, humidity: null });
    fetch(`http://${ip}/dht?enable=` + (enable ? 1 : 0)).catch(() => {});
}

function updateClimateValues(data) {
    if (!data || !data.enabled) {
        temperatureValue.textContent = '--.- C';
        humidityValue.textContent = '-- %';
        return;
    }

    if (data.ok === false) {
        temperatureValue.textContent = 'ERR';
        humidityValue.textContent = 'ERR';
        return;
    }

    temperatureValue.textContent = data.temperature === null
        ? '--.- C'
        : `${Number(data.temperature).toFixed(1)} C`;
    humidityValue.textContent = data.humidity === null
        ? '-- %'
        : `${Math.round(Number(data.humidity))} %`;
}

function formatUptime(seconds) {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = Math.floor(seconds % 60);
    if (h > 0) return `${h}h ${m}m`;
    if (m > 0) return `${m}m ${s}s`;
    return `${s}s`;
}

function updateHardwareStatus(data) {
    if (!data) return;
    if (hwWifi) hwWifi.textContent = 'Online';
    if (hwDht) hwDht.textContent = data.dhtEnabled ? (data.dhtOk ? 'OK' : 'ERR') : 'OFF';
    if (hwLed) hwLed.textContent = data.power ? 'ON' : 'OFF';
    if (hwFps) hwFps.textContent = Math.round(lastFps).toString();
    if (hwLight) hwLight.textContent = lastLight.toString();
    if (hwUptime && data.uptime !== undefined) hwUptime.textContent = formatUptime(data.uptime);
}

// ============================================================
// Auto Mood Functions
// ============================================================
function setAutoMood(enable) {
    fetch(`http://${ip}/autoswitch?enable=` + (enable ? 1 : 0)).catch(() => {});
}

// ============================================================
// Emotion Functions
// ============================================================
function setEmotionMode(mode) {
    const index = emotionModes.indexOf(mode);
    if (index !== -1) {
        currentEmotionIndex = index;
        currentEmotionSpan.textContent = emotionNames[index];
        document.querySelectorAll('.emotion-icon').forEach(btn => {
            btn.classList.toggle('active', btn.getAttribute('data-emotion') === mode);
        });
        document.querySelectorAll('.eye-preview').forEach(btn => {
            btn.classList.toggle('active', btn.getAttribute('data-emotion') === mode);
        });
    }
    fetch(`http://${ip}/manual_emotion?emotion=` + mode).catch(() => {});
}
function nextEmotion() {
    if (emotionDebounceTimer) clearTimeout(emotionDebounceTimer);
    emotionDebounceTimer = setTimeout(() => {
        currentEmotionIndex = (currentEmotionIndex + 1) % emotionModes.length;
        setEmotionMode(emotionModes[currentEmotionIndex]);
    }, 200);
}
function prevEmotion() {
    if (emotionDebounceTimer) clearTimeout(emotionDebounceTimer);
    emotionDebounceTimer = setTimeout(() => {
        currentEmotionIndex = (currentEmotionIndex - 1 + emotionModes.length) % emotionModes.length;
        setEmotionMode(emotionModes[currentEmotionIndex]);
    }, 200);
}
function buildEmotionButtons() {
    emotionButtonsContainer.innerHTML = '';
    emotionModes.forEach(mode => {
        const btn = document.createElement('button');
        btn.className = 'emotion-icon';
        btn.setAttribute('data-emotion', mode);
        btn.textContent = emotionNames[emotionModes.indexOf(mode)];
        btn.addEventListener('click', () => setEmotionMode(mode));
        emotionButtonsContainer.appendChild(btn);
    });
}

// ============================================================
// Mood Timer Functions
// ============================================================
function setMoodMin(val) {
    moodMinValue.textContent = val;
    fetch(`http://${ip}/mood?min=` + val).catch(() => {});
    if (parseInt(moodMaxSlider.value) < parseInt(val)) {
        moodMaxSlider.value = val;
        moodMaxValue.textContent = val;
        fetch(`http://${ip}/mood?max=` + val).catch(() => {});
    }
}
function setMoodMax(val) {
    moodMaxValue.textContent = val;
    fetch(`http://${ip}/mood?max=` + val).catch(() => {});
    if (parseInt(moodMinSlider.value) > parseInt(val)) {
        moodMinSlider.value = val;
        moodMinValue.textContent = val;
        fetch(`http://${ip}/mood?min=` + val).catch(() => {});
    }
}

// ============================================================
// Blink Interval Functions
// ============================================================
function setBlinkMin(val) {
    blinkMinValue.textContent = val;
    fetch(`http://${ip}/blink?min=` + val).catch(() => {});
    if (blinkMaxSlider && parseInt(blinkMaxSlider.value) < parseInt(val)) {
        blinkMaxSlider.value = val;
        blinkMaxValue.textContent = val;
        fetch(`http://${ip}/blink?max=` + val).catch(() => {});
    }
}
function setBlinkMax(val) {
    blinkMaxValue.textContent = val;
    fetch(`http://${ip}/blink?max=` + val).catch(() => {});
    if (blinkMinSlider && parseInt(blinkMinSlider.value) > parseInt(val)) {
        blinkMinSlider.value = val;
        blinkMinValue.textContent = val;
        fetch(`http://${ip}/blink?min=` + val).catch(() => {});
    }
}

// ============================================================
// Eye Animation Functions (completely independent)
// ============================================================
function setLevitation(enable, speed, amp) {
    let url = `http://${ip}/levitation?`;
    if (enable !== undefined) url += `enable=${enable ? 1 : 0}&`;
    if (speed !== undefined) url += `speed=${speed}&`;
    if (amp !== undefined) url += `amp=${amp}`;
    fetch(url).catch(() => {});
}
function setLookAround(enable, speed, amp) {
    let url = `http://${ip}/lookaround?`;
    if (enable !== undefined) url += `enable=${enable ? 1 : 0}&`;
    if (speed !== undefined) url += `speed=${speed}&`;
    if (amp !== undefined) url += `amp=${amp}`;
    fetch(url).catch(() => {});
}

// ============================================================
// Load Full State from ESP
// ============================================================
function loadState() {
    fetch(`http://${ip}/status`)
        .then(r => {
            if (!r.ok) throw new Error(`HTTP error ${r.status}`);
            return r.json();
        })
        .then(data => {
            // Lighting
            powerCheckbox.checked = data.power;
            modeSelect.value = data.mode;
            currentRed = data.red;
            currentGreen = data.green;
            currentBlue = data.blue;
            redSlider.value = data.red;
            greenSlider.value = data.green;
            blueSlider.value = data.blue;
            redValueSpan.textContent = data.red;
            greenValueSpan.textContent = data.green;
            blueValueSpan.textContent = data.blue;
            updateColorPreview();

            // Light Sensor
            ldrEnableCheckbox.checked = data.ldrEnabled;
            ldrThresholdSlider.value = data.ldrThreshold;
            ldrThresholdValue.textContent = data.ldrThreshold;

            // DHT11 Climate
            if (data.dhtEnabled !== undefined && dhtEnableCheckbox) {
                dhtEnableCheckbox.checked = data.dhtEnabled;
                updateClimateValues({
                    enabled: data.dhtEnabled,
                    ok: data.dhtOk,
                    temperature: data.temperature,
                    humidity: data.humidity
                });
            }

            // Emotions
            const emotion = data.currentEmotion;
            if (emotionModes.includes(emotion)) {
                currentEmotionIndex = emotionModes.indexOf(emotion);
                currentEmotionSpan.textContent = emotionNames[currentEmotionIndex];
                document.querySelectorAll('.emotion-icon').forEach(btn => {
                    btn.classList.toggle('active', btn.getAttribute('data-emotion') === emotion);
                });
                document.querySelectorAll('.eye-preview').forEach(btn => {
                    btn.classList.toggle('active', btn.getAttribute('data-emotion') === emotion);
                });
            } else {
                currentEmotionSpan.textContent = 'Neutral';
            }
            eyeStateDisplay.textContent = data.sleeping ? 'Sleeping' : 'Awake';
            updateHardwareStatus(data);

            // Auto mood
            if (data.autoMoodEnabled !== undefined && autoMoodCheckbox) {
                autoMoodCheckbox.checked = data.autoMoodEnabled;
            }

            // Mood Timer
            if (data.moodMin !== undefined) {
                moodMinSlider.value = data.moodMin;
                moodMinValue.textContent = data.moodMin;
            }
            if (data.moodMax !== undefined) {
                moodMaxSlider.value = data.moodMax;
                moodMaxValue.textContent = data.moodMax;
            }

            // Blink Interval
            if (data.blinkMin !== undefined) {
                blinkMinSlider.value = data.blinkMin;
                blinkMinValue.textContent = data.blinkMin;
            }
            if (data.blinkMax !== undefined) {
                blinkMaxSlider.value = data.blinkMax;
                blinkMaxValue.textContent = data.blinkMax;
            }

            // Eye Animations
            if (data.levitationEnabled !== undefined && levitationEnable) {
                levitationEnable.checked = data.levitationEnabled;
                levitationSpeed.value = data.levitationSpeed;
                levitationSpeedValue.textContent = data.levitationSpeed;
                levitationAmp.value = data.levitationAmp;
                levitationAmpValue.textContent = data.levitationAmp;
            }
            if (data.lookEnabled !== undefined && lookAroundEnable) {
                lookAroundEnable.checked = data.lookEnabled;
                lookSpeed.value = data.lookSpeed;
                lookSpeedValue.textContent = data.lookSpeed;
                lookAmp.value = data.lookAmp;
                lookAmpValue.textContent = data.lookAmp;
            }

            statusDot.style.background = "#4caf50";
            statusText.textContent = "Connected";
        })
        .catch(err => {
            console.error('loadState error:', err);
            statusDot.style.background = "#ff5555";
            statusText.textContent = "Disconnected";
        });
}

// ============================================================
// Load Real-Time Light Level
// ============================================================
function loadLightLevel() {
    fetch(`http://${ip}/light`)
        .then(r => r.json())
        .then(data => {
            const level = data.light;
            lastLight = level;
            lightLevelBar.style.width = (level / 1023 * 100) + '%';
            lightLevelValue.textContent = level;
        })
        .catch(() => {});
}

// ============================================================
// Load DHT11 Climate
// ============================================================
function loadClimate() {
    fetch(`http://${ip}/climate`)
        .then(r => r.json())
        .then(data => {
            if (dhtEnableCheckbox) dhtEnableCheckbox.checked = data.enabled;
            updateClimateValues(data);
        })
        .catch(() => {});
}

// ============================================================
// Load FPS
// ============================================================
function loadFps() {
    fetch(`http://${ip}/fps`)
        .then(r => r.json())
        .then(data => {
            const fps = data.fps;
            lastFps = fps;
            fpsValue.textContent = Math.round(fps);
            let color, widthPercent;
            if (fps < 30) {
                color = '#ff5555';
                widthPercent = (fps / 30) * 33;
            } else if (fps < 50) {
                color = '#ffaa00';
                widthPercent = 33 + ((fps - 30) / 20) * 34;
            } else {
                color = '#4caf50';
                widthPercent = 67 + ((fps - 50) / 50) * 33;
                if (widthPercent > 100) widthPercent = 100;
            }
            fpsBar.style.backgroundColor = color;
            fpsBar.style.width = widthPercent + '%';
        })
        .catch(() => {});
}

// ============================================================
// Event Listeners
// ============================================================
if (powerCheckbox) powerCheckbox.addEventListener('change', (e) => toggleLED(e.target.checked));
if (modeSelect) modeSelect.addEventListener('change', (e) => setMode(e.target.value));
if (redSlider) redSlider.addEventListener('input', (e) => setRed(e.target.value));
if (greenSlider) greenSlider.addEventListener('input', (e) => setGreen(e.target.value));
if (blueSlider) blueSlider.addEventListener('input', (e) => setBlue(e.target.value));

if (ldrEnableCheckbox) ldrEnableCheckbox.addEventListener('change', (e) => setLdrEnable(e.target.checked));
if (ldrThresholdSlider) ldrThresholdSlider.addEventListener('input', (e) => setLdrThreshold(e.target.value));
if (dhtEnableCheckbox) dhtEnableCheckbox.addEventListener('change', (e) => setDhtEnable(e.target.checked));

if (emotionPrev) emotionPrev.addEventListener('click', prevEmotion);
if (emotionNext) emotionNext.addEventListener('click', nextEmotion);

if (autoMoodCheckbox) autoMoodCheckbox.addEventListener('change', (e) => setAutoMood(e.target.checked));

if (moodMinSlider) moodMinSlider.addEventListener('input', (e) => setMoodMin(e.target.value));
if (moodMaxSlider) moodMaxSlider.addEventListener('input', (e) => setMoodMax(e.target.value));

if (blinkMinSlider) blinkMinSlider.addEventListener('input', (e) => setBlinkMin(e.target.value));
if (blinkMaxSlider) blinkMaxSlider.addEventListener('input', (e) => setBlinkMax(e.target.value));

if (levitationEnable) levitationEnable.addEventListener('change', (e) => setLevitation(e.target.checked));
if (levitationSpeed) {
    levitationSpeed.addEventListener('input', (e) => {
        levitationSpeedValue.textContent = e.target.value;
        setLevitation(undefined, e.target.value);
    });
}
if (levitationAmp) {
    levitationAmp.addEventListener('input', (e) => {
        levitationAmpValue.textContent = e.target.value;
        setLevitation(undefined, undefined, e.target.value);
    });
}
if (lookAroundEnable) lookAroundEnable.addEventListener('change', (e) => setLookAround(e.target.checked));
if (lookSpeed) {
    lookSpeed.addEventListener('input', (e) => {
        lookSpeedValue.textContent = e.target.value;
        setLookAround(undefined, e.target.value);
    });
}
if (lookAmp) {
    lookAmp.addEventListener('input', (e) => {
        lookAmpValue.textContent = e.target.value;
        setLookAround(undefined, undefined, e.target.value);
    });
}

// Build emotion buttons on page load
buildEmotionButtons();

// ============================================================
// Periodic Updates
// ============================================================
setInterval(loadState, 2000);
setInterval(loadLightLevel, 1000);
setInterval(loadClimate, 2000);
setInterval(loadFps, 1000);

// Initial Load
loadState();
loadLightLevel();
loadClimate();
loadFps();