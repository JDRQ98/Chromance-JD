// File: /js/main.js
// main.js
import { initEffectsManager, updateCurrentEffect } from './effectsManager.js';
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

export { nodeSpecificSettings, globalSettings, generateRainbowColors, generateRandomColors, generateSimilarColors };