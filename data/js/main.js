// File: /js/main.js

import { initEffectsManager, updateCurrentEffect, effects, currentEffectId } from './effectsManager.js';
import { initNodeManager, updateNodeStyles, setActiveNodes, getActiveNodes } from './nodeManager.js';
import { initModalManager, updateModal } from './modalManager.js';
import { initGlobalSettingsManager, globalSettings, resetGlobalSettings, loadGlobalSettings } from './globalSettingsManager.js'
import { generateRainbowColors, generateRandomColors, generateSimilarColors } from './colorUtils.js';
import { drawHexagon } from './drawVisualizer.js'

let nodeSpecificSettings = {};

function resetAllSettings(globalSettings, loadGlobalSettings) {
    resetGlobalSettings(globalSettings);
    nodeSpecificSettings = {};
    if (window.nodeManager){
        window.nodeManager.deactivateAllNodes()
    }
    setActiveNodes([9])
    updateModal([], getActiveNodes(), nodeSpecificSettings, globalSettings, updateNodeStyles);
    updateNodeStyles(globalSettings, nodeSpecificSettings);
    loadGlobalSettings(globalSettings);
}

function calculateNodePositions() {
    const container = document.getElementById('container');
    const containerWidth = container.offsetWidth;
    const containerHeight = container.offsetHeight;
    const numNodes = 19;
    const baseNodeSize = 30; //This will now match the CSS variable

    // Calculate the horizontal and vertical spacing
    const horizontalSpacing = containerWidth / 6; //Approx 6 columns
    const verticalSpacing = containerHeight / 6; //Approx 6 rows

    const nodePositions = [
        { x: 3 * horizontalSpacing, y: 0 * verticalSpacing },
        { x: 1.5 * horizontalSpacing, y: 1 * verticalSpacing },
        { x: 4.5 * horizontalSpacing, y: 1 * verticalSpacing },
        { x: 0.5 * horizontalSpacing, y: 2 * verticalSpacing },
        { x: 3 * horizontalSpacing, y: 2 * verticalSpacing },
        { x: 5.5 * horizontalSpacing, y: 2 * verticalSpacing },
         { x: 1.5 * horizontalSpacing, y: 3 * verticalSpacing },
        { x: 4.5 * horizontalSpacing, y: 3 * verticalSpacing },
        { x: 0.5 * horizontalSpacing, y: 4 * verticalSpacing },
        { x: 3 * horizontalSpacing, y: 4 * verticalSpacing },
        { x: 5.5 * horizontalSpacing, y: 4 * verticalSpacing },
        { x: 1.5 * horizontalSpacing, y: 5 * verticalSpacing },
         { x: 4.5 * horizontalSpacing, y: 5 * verticalSpacing },
        { x: 0.5 * horizontalSpacing, y: 6 * verticalSpacing },
        { x: 3 * horizontalSpacing, y: 6 * verticalSpacing },
         { x: 5.5 * horizontalSpacing, y: 6 * verticalSpacing },
        { x: 1.5 * horizontalSpacing, y: 7 * verticalSpacing },
        { x: 4.5 * horizontalSpacing, y: 7 * verticalSpacing },
        { x: 3 * horizontalSpacing, y: 8 * verticalSpacing },
    ];

    document.querySelectorAll('.hex-wrap').forEach((nodeWrapper, index) => {
        nodeWrapper.style.left = `${nodePositions[index].x - (baseNodeSize / 2)}px`;
        nodeWrapper.style.top = `${nodePositions[index].y - (baseNodeSize / 2)}px`;
    });
}

function initializeApp() {
    calculateNodePositions();
    initGlobalSettingsManager(globalSettings, resetAllSettings, updateNodeStyles, loadGlobalSettings);
    initEffectsManager(globalSettings, nodeSpecificSettings, updateNodeStyles, loadGlobalSettings, updateModal, resetAllSettings, setActiveNodes);
    initNodeManager(updateNodeStyles, updateModal, globalSettings, nodeSpecificSettings, updateCurrentEffect);
    initModalManager(nodeSpecificSettings, globalSettings, updateNodeStyles, updateModal, updateCurrentEffect, setActiveNodes, getActiveNodes);
    updateNodeStyles(globalSettings, nodeSpecificSettings);
    drawHexagon();
}

document.addEventListener('DOMContentLoaded', () => {
    initializeApp();
    window.addEventListener('resize', calculateNodePositions);
});

// Function to send the configuration to the microcontroller
function sendConfigurationToMicrocontroller() {
    const activeNodes = getActiveNodes();
    // Gather the current effect's data.
    const currentEffect = effects[currentEffectId];

    // Prepare the data to send.
    const data = {
        globalSettings: currentEffect.globalSettings,
        activeNodes: activeNodes,
        nodeSpecificSettings: {} // Initialize as an empty object
    };

    // Iterate over the active nodes and collect any node-specific settings that differ from the global settings.
    activeNodes.forEach(nodeId => {
        //Check to see if the node specific settings exist
        if (nodeSpecificSettings[nodeId]) {
            const nodeSettings = {};

            //Get all the edit checkboxes
            const editCheckboxes = Array.from(document.getElementById('modal').querySelectorAll('.edit-button-checkbox'));

            //Loop through all the edit checkboxes
            editCheckboxes.forEach(checkbox => {
                const setting = checkbox.dataset.setting;

                //For each checkbox, check if it's checked
                if (checkbox.checked) {
                    //If the checkbox is checked, add the corresponding value
                    if (setting === 'startingColor') {
                        //The starting color needs to be obtained from the modal as it's the one the user is configuring
                        const colorInputs = Array.from(document.getElementById('modal').querySelectorAll('#modalColorContainer input'));
                        nodeSettings.startingColor = colorInputs.map(input => input.value);
                    } else {
                        //Same here, obtain the values from the modal
                        const inputElement = document.getElementById(`modal${setting.charAt(0).toUpperCase() + setting.slice(1)}`);
                        nodeSettings[setting] = inputElement.value;
                    }
                }
            });

            // If any node settings were collected, add them to the data object.
            if (Object.keys(nodeSettings).length > 0) {
                data.nodeSpecificSettings[nodeId] = nodeSettings;
            }
        }
    });

    // Convert the data to JSON.
    const jsonData = JSON.stringify(data);

    // Log the JSON data to the console
    console.log("Sending this JSON to the microcontroller:", jsonData);

    // Send the POST request to the microcontroller.  Replace with the correct URL for your microcontroller.
    fetch('http://hexagono.local/updateInternalVariables', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: jsonData
    })
    .then(response => {
        if (!response.ok) {
            // If the response status is not OK, reject the promise to go to the catch block
            return response.text().then(text => {
                throw new Error(`HTTP error! status: ${response.status}, body: ${text}`);
            });
        }
        // Check if the content type is JSON before parsing
        const contentType = response.headers.get("content-type");
        if (contentType && contentType.includes("application/json")) {
            return response.json();
        } else {
            return response.text(); // Handle non-JSON responses
        }
    })
    .then(responseData => {
        // Handle the response from the microcontroller.
        console.log('Configuration sent successfully:', responseData);
    })
    .catch(error => {
        // Handle errors.
        console.error('Error sending configuration:', error);
        alert('Error sending configuration. See console for details.');  // Or display a more user-friendly message.
    });
}
// Make the function globally accessible
window.mainJS = {
    sendConfigurationToMicrocontroller: sendConfigurationToMicrocontroller
};

export { nodeSpecificSettings, globalSettings, generateRainbowColors, generateRandomColors, generateSimilarColors };