// globalSettingsManager.js
import { generateRainbowColors, generateRandomColors, generateSimilarColors } from './colorUtils.js';
import { currentEffectId } from './effectsManager.js';
import { effects } from './effectsManager.js' //Import the effects object

let globalSettings = {}; // Object to store global settings

function openGlobalSettingsModal() {
    const globalSettingsModal = document.getElementById('globalSettingsModal');
    const overlay = document.getElementById('overlay');
    globalSettingsModal.classList.add('show');
    overlay.classList.add('show');
}
function closeGlobalSettingsModal() {
    const globalSettingsModal = document.getElementById('globalSettingsModal');
    const overlay = document.getElementById('overlay');
    globalSettingsModal.classList.remove('show');
    overlay.classList.remove('show');
}

function setupGlobalSettingsModal(loadGlobalSettings, saveGlobalSettings, resetAllSettings) {
    const editEffectButton = document.getElementById('editEffectButton');

    // Add listener to the ESC key to close global modal
    document.addEventListener('keydown', function (event) {
        if (event.key === 'Escape' && document.getElementById('globalSettingsModal').classList.contains('show')) {
            closeGlobalSettingsModal();
        }
        // Add listener to the enter key to submit global modal
        if (event.key === 'Enter' && document.getElementById('globalSettingsModal').classList.contains('show')) {
            document.getElementById('submitButton').click();
        }
    });
    // Restore defaults logic
    const restoreDefaultsButton = document.getElementById('restoreDefaultsButton');
    restoreDefaultsButton.addEventListener('click', () => {
        resetAllSettings(globalSettings, loadGlobalSettings);
        closeGlobalSettingsModal();
    });
}
// Function to reset all global settings to default
function resetGlobalSettings(globalSettings) {
    globalSettings.effectBasis = 'ripple';
    globalSettings.effectDuration = 3000;
    globalSettings.desiredBehavior = 'normal';
    globalSettings.rippleDirection = 'allDirections';
    globalSettings.rippleDelay = 3000;
    globalSettings.rippleLifeSpan = 3000;
    globalSettings.rippleSpeed = 0.5;
    globalSettings.decayPerTick = 0.985;
    globalSettings.hueDeltaTick = 200;
    globalSettings.numberOfRipples = 1;
    globalSettings.colors = generateRainbowColors(7); // Default to 7 rainbow colors
}

// Function to save global settings for the current effect
function saveGlobalSettings(globalSettings, updateNodeStyles) {

    if (!effects[currentEffectId]) {
        console.error('Current effect not found in local storage');
        return;
    }
    // Get all input values
    globalSettings.effectBasis = document.getElementById('effectBasis').value;
    globalSettings.effectDuration = Number(document.getElementById('effectDuration').value);
    globalSettings.desiredBehavior = document.getElementById('desiredBehavior').value;
    globalSettings.rippleDirection = document.getElementById('rippleDirection').value;
    globalSettings.rippleDelay = Number(document.getElementById('rippleDelay').value);
    globalSettings.rippleLifeSpan = Number(document.getElementById('rippleLifeSpan').value);
    globalSettings.rippleSpeed = Number(document.getElementById('rippleSpeed').value);
    globalSettings.decayPerTick = Number(document.getElementById('decayPerTick').value);
    globalSettings.hueDeltaTick = Number(document.getElementById('hueDeltaTick').value);
    globalSettings.numberOfRipples = document.getElementById('numberOfRipples').value;
    globalSettings.colors = Array.from(document.querySelectorAll('#globalColorContainer input')).map(input => input.value);
    effects[currentEffectId].globalSettings = globalSettings;
    // Get the effect name input
    const effectNameInput = document.getElementById('effectNameInput');
    if (effectNameInput) {
        effects[currentEffectId].name = effectNameInput.value;
        const effectDropdown = document.getElementById('effectDropdown');
        const selectedOption = effectDropdown.options[effectDropdown.selectedIndex];
        if (selectedOption) {
            selectedOption.text = effectNameInput.value; //Update the dropdown text
        }
        document.getElementById('effectTitle').textContent = `Effect editor: ${effectNameInput.value}`
    }
    localStorage.setItem('effects', JSON.stringify(effects));
    console.log("Global settings for effect", currentEffectId, "are", globalSettings);
    // Update node styles
    updateNodeStyles(globalSettings, effects[currentEffectId].nodeSpecificSettings);
}
// Function to load global settings for the current effect
function loadGlobalSettings(globalSettings) {

    if (!effects[currentEffectId]) {
        console.error('Current effect not found in local storage');
        return;
    }
    const storedSettings = effects[currentEffectId].globalSettings;
    // Update input values if settings exist
    if (storedSettings) {
        document.getElementById('effectBasis').value = storedSettings.effectBasis;
        document.getElementById('effectDuration').value = storedSettings.effectDuration;
        document.getElementById('desiredBehavior').value = storedSettings.desiredBehavior;
        document.getElementById('rippleDirection').value = storedSettings.rippleDirection;
        document.getElementById('rippleDelay').value = storedSettings.rippleDelay;
        document.getElementById('rippleLifeSpan').value = storedSettings.rippleLifeSpan;
        document.getElementById('rippleSpeed').value = storedSettings.rippleSpeed;
        document.getElementById('rippleSpeedDisplay').textContent = parseFloat(storedSettings.rippleSpeed).toFixed(2);
        document.getElementById('decayPerTick').value = storedSettings.decayPerTick;
        document.getElementById('decayPerTickDisplay').textContent = parseFloat(storedSettings.decayPerTick).toFixed(3);
        document.getElementById('hueDeltaTick').value = storedSettings.hueDeltaTick;
        document.getElementById('numberOfRipples').value = storedSettings.numberOfRipples;
        // Load colors into swatches
        const colorContainer = document.getElementById('globalColorContainer');
        colorContainer.innerHTML = ''; // Clear existing swatches
        storedSettings.colors.forEach(color => {
            const colorInput = document.createElement('input');
            colorInput.type = 'color';
            colorInput.value = color;
            colorInput.classList.add('color-swatch');
            colorContainer.appendChild(colorInput);
        });
    }
    // Load effect name
    const effectNameInput = document.getElementById('effectNameInput');
    if (effectNameInput && effects[currentEffectId].name) {
        effectNameInput.value = effects[currentEffectId].name;
    }

    console.log("Loaded global settings for effect", currentEffectId, "are", storedSettings);
}
// Function to initialize the global settings manager
function initGlobalSettingsManager(globalSettings, resetAllSettings, updateNodeStyles, loadGlobalSettings) {
    // Cache frequently accessed DOM elements
    const rippleSpeedInput = document.getElementById('rippleSpeed');
    const rippleSpeedDisplay = document.getElementById('rippleSpeedDisplay');
    const decayPerTickInput = document.getElementById('decayPerTick');
    const decayPerTickDisplay = document.getElementById('decayPerTickDisplay');
    // Event listeners for the range elements
    rippleSpeedInput.addEventListener('input', () => {
        rippleSpeedDisplay.textContent = parseFloat(rippleSpeedInput.value).toFixed(2);
    });

    decayPerTickInput.addEventListener('input', () => {
        decayPerTickDisplay.textContent = parseFloat(decayPerTickInput.value).toFixed(3);
    });
    setupGlobalSettingsModal(loadGlobalSettings, saveGlobalSettings, resetAllSettings);
    // Load settings to localStorage if they are not present
    const effects = JSON.parse(localStorage.getItem('effects') || '{}');
    if (!effects[1] || !effects[1].globalSettings) {
        resetGlobalSettings(globalSettings);
    } else {
        Object.assign(globalSettings, effects[1].globalSettings);
    }

    // Event listener for fire ripple button
    const fireRippleButton = document.getElementById('fireRippleButton');
    fireRippleButton.addEventListener('click', () => {
        console.log("Fire ripple button clicked");
        //Add the logic for firing the ripple, using all the properties from the interface
    });
    // Event listener for the submit button
    const submitButton = document.getElementById('submitButton');
    submitButton.addEventListener('click', () => {
        console.log("Submit button clicked");
        saveGlobalSettings(globalSettings, updateNodeStyles); // Pass updateNodeStyles
        closeGlobalSettingsModal();
    });
    // Event listener for discard button
    const discardButton = document.getElementById('discardButton');
    discardButton.addEventListener('click', () => {
        console.log("Discard button clicked");
        loadGlobalSettings(globalSettings);
        closeGlobalSettingsModal();
    });
    // Add logic for adding more global colors
    const addGlobalColorButton = document.getElementById('addGlobalColorButton');
    const globalColorContainer = document.getElementById('globalColorContainer');
    addGlobalColorButton.addEventListener('click', () => {
        if (globalColorContainer.children.length < 25) {
            const colorInput = document.createElement('input');
            colorInput.type = 'color';
            colorInput.value = '#ffffff'; // Default color
            colorInput.classList.add('color-swatch');
            globalColorContainer.appendChild(colorInput);
        }
    });
    // Add logic for removing global colors
    const removeGlobalColorButton = document.getElementById('removeGlobalColorButton');
    removeGlobalColorButton.addEventListener('click', () => {
        if (globalColorContainer.children.length > 1) {
            globalColorContainer.removeChild(globalColorContainer.lastChild);
        }
    });
    // Add logic for rainbow global colors button
    const globalRainbowButton = document.getElementById('globalRainbowButton');
    globalRainbowButton.addEventListener('click', () => {
        const colorCount = document.querySelectorAll('#globalColorContainer input').length;
        const rainbowColors = generateRainbowColors(colorCount);
        const colorContainer = document.getElementById('globalColorContainer');
        colorContainer.innerHTML = ''; // Clear existing swatches
        rainbowColors.forEach(color => {
            const colorInput = document.createElement('input');
            colorInput.type = 'color';
            colorInput.value = color;
            colorInput.classList.add('color-swatch');
            colorContainer.appendChild(colorInput);
        });
    });
    // Add logic for random global colors button
    const globalRandomButton = document.getElementById('globalRandomButton');
    globalRandomButton.addEventListener('click', () => {
        const colorCount = document.querySelectorAll('#globalColorContainer input').length;
        const randomColors = generateRandomColors(colorCount);
        const colorContainer = document.getElementById('globalColorContainer');
        colorContainer.innerHTML = ''; // Clear existing swatches
        randomColors.forEach(color => {
            const colorInput = document.createElement('input');
            colorInput.type = 'color';
            colorInput.value = color;
            colorInput.classList.add('color-swatch');
            colorContainer.appendChild(colorInput);
        });
    });
    // Add logic for similar global colors button
    const globalSimilarButton = document.getElementById('globalSimilarButton');
    globalSimilarButton.addEventListener('click', () => {
        const colorCount = document.querySelectorAll('#globalColorContainer input').length;
        const firstColor = document.querySelector('#globalColorContainer input')?.value;
        const similarColors = generateSimilarColors(colorCount, firstColor);
        const colorContainer = document.getElementById('globalColorContainer');
        colorContainer.innerHTML = ''; // Clear existing swatches
        similarColors.forEach(color => {
            const colorInput = document.createElement('input');
            colorInput.type = 'color';
            colorInput.value = color;
            colorInput.classList.add('color-swatch');
            colorContainer.appendChild(colorInput);
        });
    });
}
export { initGlobalSettingsManager, globalSettings, resetGlobalSettings, loadGlobalSettings, openGlobalSettingsModal, closeGlobalSettingsModal };