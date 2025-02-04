// File: /js/modalManager.js
import { generateRainbowColors, generateRandomColors, generateSimilarColors } from './colorUtils.js';
import { getActiveNodes } from './nodeManager.js';

class Modal {
    constructor(modalElement, overlayElement, selectedNodesDisplay, activateCheckbox, modalSettings, globalSettings, nodeSpecificSettings) {
        this.modalElement = modalElement;
        this.overlayElement = overlayElement;
        this.selectedNodesDisplay = selectedNodesDisplay;
        this.activateCheckbox = activateCheckbox;
        this.modalSettings = modalSettings
        this.editNodeButton = this.modalElement.querySelector('#editNodeButton');
        this.globalSettings = globalSettings;
        this.nodeSpecificSettings = nodeSpecificSettings;
        this.modalInputs = {};
        this.cacheModalInputs();
        this.activeNodes = [];
        this.selectedNodes = [];
        this.initialSettings = {
            activeNodes: [],
            nodeSpecificSettings: {}
        };
        this.activateCheckbox.addEventListener('change', () => this.handleActivateCheckboxChange());
        const slider = this.modalElement.querySelector('.slider')
        if (slider) {
            slider.addEventListener('click', () => {
                this.activateCheckbox.click();
            });
        }
        window.modal = this;
        console.log('Modal constructor called') // ADDED LOG
        // Add listener for closing the modal with overlay
        this.overlayElement.addEventListener('click', (event) => {
            if (event.target === this.overlayElement) {
                this.closeModal(setActiveNodes, updateNodeStyles, updateCurrentEffect);
            }
        });
    }
    cacheModalInputs() {
        this.modalInputs = {
            modalDesiredBehavior: this.modalElement.querySelector('#modalDesiredBehavior'),
            modalRippleDirection: this.modalElement.querySelector('#modalRippleDirection'),
            modalRippleDelay: this.modalElement.querySelector('#modalRippleDelay'),
            modalRippleLifeSpan: this.modalElement.querySelector('#modalRippleLifeSpan'),
            modalRippleSpeed: this.modalElement.querySelector('#modalRippleSpeed'),
            modalRippleSpeedDisplay: this.modalElement.querySelector('#modalRippleSpeedDisplay'),
            modalDecayPerTick: this.modalElement.querySelector('#modalDecayPerTick'),
            modalDecayPerTickDisplay: this.modalElement.querySelector('#modalDecayPerTickDisplay'),
            modalHueDeltaTick: this.modalElement.querySelector('#modalHueDeltaTick'),
        };
        // Add input listeners to the range elements to update the span elements
        this.modalInputs.modalRippleSpeed.addEventListener('input', () => {
            this.modalInputs.modalRippleSpeedDisplay.textContent = parseFloat(this.modalInputs.modalRippleSpeed.value).toFixed(2);
        });
        this.modalInputs.modalDecayPerTick.addEventListener('input', () => {
            this.modalInputs.modalDecayPerTickDisplay.textContent = parseFloat(this.modalInputs.modalDecayPerTick.value).toFixed(3);
        });
        // Add click event listener to the edit node button
        this.editNodeButton.addEventListener('click', () => {
            if (!this.modalSettings.classList.contains('show')) {
                this.modalSettings.classList.add('show');
            } else {
                this.modalSettings.classList.remove('show');
            }
        });
    }
    handleActivateCheckboxChange() {
        const selectedIds = this.selectedNodes.map(node => node.dataset.id);
        /* This functionality to hide/show the rest of the settings has been transferred to the 'Edit node button'
        if (selectedIds.length === 1) {
            if (this.activateCheckbox.checked) {
                this.modalSettings.classList.add('show');
            } else {
                this.modalSettings.classList.remove('show');
            }
        } else if (selectedIds.length > 1) {
            if (this.activateCheckbox.checked) {
                this.modalSettings.classList.add('show');
            } else {
                this.modalSettings.classList.remove('show');
            }
        }
        */
    }
    openModal() {
        this.takeSnapshot();
        this.modalElement.classList.add('show');
        this.overlayElement.classList.add('show');
    }
    closeModal(setActiveNodes, updateNodeStyles, updateCurrentEffect) {
        this.modalElement.classList.remove('show');
        this.overlayElement.classList.remove('show');
        window.nodeManager.deselectAllNodes();//Call node manager's method to deselect all
        this.selectedNodes = []; // Clear selected nodes array
        setActiveNodes(getActiveNodes());
        updateNodeStyles(this.globalSettings, this.nodeSpecificSettings);
        updateCurrentEffect(this.globalSettings, this.nodeSpecificSettings, getActiveNodes());
    }
    updateModalDisplay(selectedNodes, activeNodes) {
        console.log('Modal updateModalDisplay called with:', selectedNodes, activeNodes); // ADDED LOG
        this.activeNodes = activeNodes;
        this.selectedNodes = selectedNodes;
        const selectedIds = selectedNodes.map(node => node.dataset.id);

        //Update node display
        if (selectedIds.length > 0) {
            this.selectedNodesDisplay.textContent = 'Selected Nodes: ' + selectedIds.join(', ');
        } else {
            this.selectedNodesDisplay.textContent = 'No nodes selected';
        }
        // Check the state of the nodes for activation
        if (selectedIds.length === 1) {
            const selectedNodeId = selectedIds[0];
            if (activeNodes.includes(Number(selectedNodeId))) {
                this.activateCheckbox.checked = true;
                this.activateCheckbox.indeterminate = false; //remove indeterminate state
            } else {
                this.activateCheckbox.checked = false;
                this.activateCheckbox.indeterminate = false;
            }
            if (selectedNodes.length > 0) {
                this.loadNodeSettings(selectedNodes[0]);
            }
            if (this.selectedNodes.length > 0) {
                this.modalSettings.classList.remove('show');
            }
        } else if (selectedIds.length > 1) {
            const allActive = selectedIds.every(id => activeNodes.includes(Number(id)));
            if (allActive) {
                this.activateCheckbox.checked = true;
                this.activateCheckbox.indeterminate = false;
            } else {
                const noneActive = selectedIds.every(id => !activeNodes.includes(Number(id)));
                if (noneActive) {
                    this.activateCheckbox.checked = false;
                    this.activateCheckbox.indeterminate = false;
                } else {
                    this.activateCheckbox.checked = false;
                    this.activateCheckbox.indeterminate = true;
                    this.modalSettings.classList.remove('show');
                }
            }
            if (selectedNodes.length > 0) {
                this.loadNodeSettings(selectedNodes[0]);
            }
            if (this.selectedNodes.length > 0) {
                this.loadNodeSettings(selectedNodes[0]);
            }
            this.disableModalInputs();
        } else {
            this.activateCheckbox.checked = false;
            this.activateCheckbox.indeterminate = false;
            this.disableModalInputs();
            this.modalElement.classList.remove('show');
            this.overlayElement.classList.remove('show');
        }
        if (selectedIds.length > 0 && !this.modalElement.classList.contains('show')) {
            this.openModal();
        }
    }
    // Function to load node specific settings into the modal
    takeSnapshot() {
        this.initialSettings.activeNodes = [...this.activeNodes]; // Take a snapshot of active nodes
        this.initialSettings.nodeSpecificSettings = JSON.parse(JSON.stringify(this.nodeSpecificSettings)); // Deep clone node settings
        console.log('Taking a snapshot with these settings', this.initialSettings)
    }
    // Function to load node specific settings into the modal
    loadNodeSettings(node) {
        const nodeId = node.dataset.id;
        const selectedIds = this.selectedNodes.map(node => node.dataset.id);

        // Set values from global or node settings
        let desiredBehaviorValue = this.nodeSpecificSettings[nodeId]?.desiredBehavior ?? this.globalSettings.desiredBehavior;
        let rippleDirectionValue = this.nodeSpecificSettings[nodeId]?.rippleDirection ?? this.globalSettings.rippleDirection;
        let rippleDelayValue = this.nodeSpecificSettings[nodeId]?.rippleDelay ?? this.globalSettings.rippleDelay;
        let rippleLifeSpanValue = this.nodeSpecificSettings[nodeId]?.rippleLifeSpan ?? this.globalSettings.rippleLifeSpan;
        let rippleSpeedValue = this.nodeSpecificSettings[nodeId]?.rippleSpeed ?? this.globalSettings.rippleSpeed;
        let decayPerTickValue = this.nodeSpecificSettings[nodeId]?.decayPerTick ?? this.globalSettings.decayPerTick;
        let hueDeltaTickValue = this.nodeSpecificSettings[nodeId]?.hueDeltaTick ?? this.globalSettings.hueDeltaTick;
        // Load colors into swatches
        const colorContainer = this.modalElement.querySelector('#modalColorContainer');
        colorContainer.innerHTML = ''; // Clear existing swatches
        let colorsValue = [];
        if (this.nodeSpecificSettings[nodeId] && this.nodeSpecificSettings[nodeId].hasOwnProperty('startingColor')) {
            colorsValue = this.nodeSpecificSettings[nodeId].startingColor
        } else {
            colorsValue = this.globalSettings.colors;
        }
        if (typeof colorsValue === 'string') {
            colorsValue = [colorsValue];
        }
        if (!colorsValue) {
            colorsValue = [];
        }
        colorsValue.forEach(color => {
            const colorInput = document.createElement('input');
            colorInput.type = 'color';
            colorInput.value = color;
            colorInput.classList.add('color-swatch');
            // Disable the color swatches when the checkbox is not checked
            const colorEditCheckbox = Array.from(this.modalElement.querySelectorAll('.edit-button-checkbox')).find(checkbox => checkbox.dataset.setting === 'startingColor');
            if (colorEditCheckbox && !colorEditCheckbox.checked) {
                colorInput.disabled = true;
            }
            //Add the event listener
            colorInput.addEventListener('click', function (event) {
                const colorEditCheckbox = Array.from(this.modalElement.querySelectorAll('.edit-button-checkbox')).find(checkbox => checkbox.dataset.setting === 'startingColor');
                if (colorEditCheckbox && !colorEditCheckbox.checked) {
                    event.preventDefault();
                    return;
                }
                this.showPicker();
            });
            colorContainer.appendChild(colorInput);
        });
        // Set edit checkbox states based on node specific settings
        const editCheckboxes = this.modalElement.querySelectorAll('.edit-button-checkbox');
        editCheckboxes.forEach(checkbox => {
            const setting = checkbox.dataset.setting;
            let allSame = true;
            if (selectedIds.length === 1) {
                if (this.nodeSpecificSettings[nodeId] && this.nodeSpecificSettings[nodeId].hasOwnProperty(setting)) {
                    checkbox.checked = true;
                    checkbox.indeterminate = false;
                    // Enable the input if the checkbox is checked
                    this.enableModalInput(setting);
                } else {
                    checkbox.checked = false;
                    checkbox.indeterminate = false;
                }
            } else if (selectedIds.length > 1) {
                allSame = selectedIds.every(id => {
                    if (this.nodeSpecificSettings[id] && this.nodeSpecificSettings[id].hasOwnProperty(setting)) {
                        return this.nodeSpecificSettings[selectedIds[0]] && this.nodeSpecificSettings[selectedIds[0]][setting] === this.nodeSpecificSettings[id][setting];
                    } else {
                        return !(this.nodeSpecificSettings[selectedIds[0]] && this.nodeSpecificSettings[selectedIds[0]].hasOwnProperty(setting));
                    }
                });
                if (allSame) {
                    checkbox.checked = selectedIds.every(id => {
                        if (this.nodeSpecificSettings[id] && this.nodeSpecificSettings[id].hasOwnProperty(setting)) {
                            return this.nodeSpecificSettings[selectedIds[0]][setting] === this.nodeSpecificSettings[id][setting];
                        } else {
                            return !(this.nodeSpecificSettings[id] && this.nodeSpecificSettings[id].hasOwnProperty(setting));
                        }
                    })
                    checkbox.indeterminate = false;
                    // Enable the input if the checkbox is checked
                    if (checkbox.checked) {
                        this.enableModalInput(setting);
                    }
                } else {
                    checkbox.checked = false;
                    checkbox.indeterminate = true;
                    if (setting === 'desiredBehavior') {
                        desiredBehaviorValue = 'N/A';
                    } else if (setting === 'rippleDirection') {
                        rippleDirectionValue = 'N/A';
                    } else if (setting === 'rippleDelay') {
                        rippleDelayValue = 'N/A';
                    } else if (setting === 'rippleLifeSpan') {
                        rippleLifeSpanValue = 'N/A';
                    } else if (setting === 'rippleSpeed') {
                        rippleSpeedValue = 'N/A';
                    } else if (setting === 'decayPerTick') {
                        decayPerTickValue = 'N/A';
                    } else if (setting === 'hueDeltaTick') {
                        hueDeltaTickValue = 'N/A';
                    }
                }
            }
        });
        this.setModalInputValue('desiredBehavior', desiredBehaviorValue)
        this.setModalInputValue('rippleDirection', rippleDirectionValue)
        this.setModalInputValue('rippleDelay', rippleDelayValue)
        this.setModalInputValue('rippleLifeSpan', rippleLifeSpanValue)
        this.setModalInputValue('rippleSpeed', rippleSpeedValue)
        this.setModalInputValue('decayPerTick', decayPerTickValue)
        this.setModalInputValue('hueDeltaTick', hueDeltaTickValue)
        // Disable/enable color buttons based on checkbox state
        const colorEditCheckbox = Array.from(editCheckboxes).find(checkbox => checkbox.dataset.setting === 'startingColor');
        if (colorEditCheckbox) {
            const colorButtons = this.modalElement.querySelectorAll('.color-button-container button')
            colorButtons.forEach(button => {
                button.disabled = !colorEditCheckbox.checked
            })
        }
        //Add event listener to edit checkbox buttons
        editCheckboxes.forEach(checkbox => {
            checkbox.addEventListener('change', () => {
                const setting = checkbox.dataset.setting;
                if (checkbox.checked) {
                    this.enableModalInput(setting);
                } else {
                    this.disableModalInput(setting);
                }
                // Disable/enable color buttons based on checkbox state
                if (setting === 'startingColor') {
                    const colorButtons = this.modalElement.querySelectorAll('.color-button-container button')
                    colorButtons.forEach(button => {
                        button.disabled = !checkbox.checked
                    })
                    // Disable color inputs when the checkbox is unchecked
                    const colorInputs = this.modalElement.querySelectorAll('#modalColorContainer input');
                    colorInputs.forEach(input => {
                        input.disabled = !checkbox.checked;
                    });
                }
            });
        });
    }
    //Helper function to set values in modal input elements
    setModalInputValue(setting, value) {
        if (this.modalInputs[`modal${setting.charAt(0).toUpperCase() + setting.slice(1)}`]) {
            const inputElement = this.modalInputs[`modal${setting.charAt(0).toUpperCase() + setting.slice(1)}`];
            inputElement.value = value;
            // Update display if range
            if (setting === 'rippleSpeed') {
                this.modalInputs.modalRippleSpeedDisplay.textContent = parseFloat(value).toFixed(2);
            }
            if (setting === 'decayPerTick') {
                this.modalInputs.modalDecayPerTickDisplay.textContent = parseFloat(value).toFixed(3);
            }
        }
    }
    //Helper function to get values from modal input elements
    getModalInputValue(setting) {
        return this.modalInputs[`modal${setting.charAt(0).toUpperCase() + setting.slice(1)}`]?.value;
    }
    // Helper function to enable modal input elements
    enableModalInput(setting) {
        const inputElement = this.modalInputs[`modal${setting.charAt(0).toUpperCase() + setting.slice(1)}`];
        if (inputElement) {
            inputElement.disabled = false;
            if (setting === 'rippleSpeed') {
                this.modalInputs.modalRippleSpeedDisplay.textContent = parseFloat(this.getModalInputValue(setting)).toFixed(2);
            }
            if (setting === 'decayPerTick') {
                this.modalInputs.modalDecayPerTickDisplay.textContent = parseFloat(this.getModalInputValue(setting)).toFixed(3);
            }
        }
    }
    // Helper function to disable modal input elements
    disableModalInput(setting) {
        const inputElement = this.modalInputs[`modal${setting.charAt(0).toUpperCase() + setting.slice(1)}`];
        if (inputElement) {
            inputElement.disabled = true;
            const selectedNodes = this.selectedNodes;
            selectedNodes.forEach(node => {
                const nodeId = node.dataset.id;
                if (this.nodeSpecificSettings[nodeId] && this.nodeSpecificSettings[nodeId].hasOwnProperty(setting)) {
                    delete this.nodeSpecificSettings[nodeId][setting];
                }
            })
        }
    }
    // Helper function to disable all modal input elements
    disableModalInputs() {
        const inputs = this.modalElement.querySelectorAll('input:not(#activateNodeCheckbox):not(.edit-button-checkbox), select')
        inputs.forEach(input => {
            input.disabled = true;
        })
        const editCheckboxes = this.modalElement.querySelectorAll('.edit-button-checkbox');
        editCheckboxes.forEach(checkbox => {
            checkbox.checked = false;
        });
        // Disable color buttons
        const colorButtons = this.modalElement.querySelectorAll('.color-button-container button')
        colorButtons.forEach(button => {
            button.disabled = true
        })
        // Disable color inputs
        const colorInputs = this.modalElement.querySelectorAll('#modalColorContainer input');
        colorInputs.forEach(input => {
            input.disabled = true;
        });
    }
    // Function to save node settings
    saveNodeSettings(setActiveNodes, updateNodeStyles, updateCurrentEffect) {
        let newActiveNodes = [...this.activeNodes];
        this.selectedNodes.forEach(node => {
            const nodeId = node.dataset.id;
            const activateCheckbox = this.activateCheckbox;
            const nodeIdNumber = Number(nodeId);
            if (activateCheckbox.checked) {
                if (!newActiveNodes.includes(nodeIdNumber)) {
                    newActiveNodes.push(nodeIdNumber);
                }
            } else {
                newActiveNodes = newActiveNodes.filter(activeNodeId => activeNodeId !== nodeIdNumber);
            }
            const editCheckboxes = this.modalElement.querySelectorAll('.edit-button-checkbox');
            let hasSettings = false;
            editCheckboxes.forEach(checkbox => {
                if (checkbox.checked) {
                    const setting = checkbox.dataset.setting;
                    if (!this.nodeSpecificSettings[nodeId]) {
                        this.nodeSpecificSettings[nodeId] = {};
                    }
                    if (setting === 'startingColor') {
                        this.nodeSpecificSettings[nodeId][setting] = Array.from(this.modalElement.querySelectorAll('#modalColorContainer input')).map(input => input.value);
                    } else {
                        this.nodeSpecificSettings[nodeId][setting] = this.getModalInputValue(setting);
                    }
                    hasSettings = true;
                } else if (this.nodeSpecificSettings[nodeId]) {
                    const setting = checkbox.dataset.setting;
                    if (this.nodeSpecificSettings[nodeId].hasOwnProperty(setting)) {
                        delete this.nodeSpecificSettings[nodeId][setting]
                    }
                }
            });
            if (!hasSettings && this.nodeSpecificSettings[nodeId]) {
                delete this.nodeSpecificSettings[nodeId];
            }
        });
        setActiveNodes(newActiveNodes);
        updateNodeStyles(this.globalSettings, this.nodeSpecificSettings);
        updateCurrentEffect(this.globalSettings, this.nodeSpecificSettings, newActiveNodes);
        window.nodeManager.deselectAllNodes();// Deselect all nodes using the node manager
    }
    // Function to discard node settings
    discardNodeSettings(setActiveNodes, updateNodeStyles, updateCurrentEffect) {
        console.log('Restoring settings to:', this.initialSettings)
        // Restore active nodes
        setActiveNodes(this.initialSettings.activeNodes);
        // Restore node specific settings
        this.nodeSpecificSettings = JSON.parse(JSON.stringify(this.initialSettings.nodeSpecificSettings));
        updateNodeStyles(this.globalSettings, this.nodeSpecificSettings);
        updateCurrentEffect(this.globalSettings, this.nodeSpecificSettings, this.initialSettings.activeNodes);
        window.nodeManager.deselectAllNodes();// Deselect all nodes using the node manager
    }
}


let modal;

function initModalManager(nodeSpecificSettings, globalSettings, updateNodeStyles, updateModal, updateCurrentEffect, setActiveNodes, getActiveNodes) {
    const modalElement = document.getElementById('modal');
    const overlayElement = document.getElementById('overlay');
    const selectedNodesDisplay = document.getElementById('selectedNodesDisplay');
    const activateCheckbox = document.getElementById('activateNodeCheckbox');
    const modalSettings = document.querySelector('.modal-settings');

    modal = new Modal(modalElement, overlayElement, selectedNodesDisplay, activateCheckbox, modalSettings, globalSettings, nodeSpecificSettings);
    // Add listener to the close modal button

    // Add listener for closing the modal with ESC key
    document.addEventListener('keydown', function (event) {
        if (event.key === 'Escape') {
            modal.closeModal(setActiveNodes, updateNodeStyles, updateCurrentEffect);
        }
    });
    const saveNodeButton = document.getElementById('saveNodeButton');
    const discardNodeButton = document.getElementById('discardNodeButton');
    saveNodeButton.addEventListener('click', function () {
        modal.saveNodeSettings(setActiveNodes, updateNodeStyles, updateCurrentEffect);
        modal.closeModal(setActiveNodes, updateNodeStyles, updateCurrentEffect);
    });
    discardNodeButton.addEventListener('click', function () {
        modal.discardNodeSettings(setActiveNodes, updateNodeStyles, updateCurrentEffect);
        modal.closeModal(setActiveNodes, updateNodeStyles, updateCurrentEffect);
    });
    // Add logic for adding more modal colors
    const addModalColorButton = document.getElementById('addModalColorButton');
    const modalColorContainer = document.getElementById('modalColorContainer');
    addModalColorButton.addEventListener('click', () => {
        if (modalColorContainer.children.length < 25) {
            const colorInput = document.createElement('input');
            colorInput.type = 'color';
            colorInput.value = '#ffffff'; // Default color
            colorInput.classList.add('color-swatch');
            // Disable the color swatches when the checkbox is not checked
            const colorEditCheckbox = Array.from(document.getElementById('modal').querySelectorAll('.edit-button-checkbox')).find(checkbox => checkbox.dataset.setting === 'startingColor');
            if (colorEditCheckbox && !colorEditCheckbox.checked) {
                colorInput.disabled = true;
            }
            // Add the event listener here
            colorInput.addEventListener('click', function (event) {
                const colorEditCheckbox = Array.from(document.getElementById('modal').querySelectorAll('.edit-button-checkbox')).find(checkbox => checkbox.dataset.setting === 'startingColor');
                if (colorEditCheckbox && !colorEditCheckbox.checked) {
                    event.preventDefault();
                    return;
                }
                this.showPicker();
            });
            modalColorContainer.appendChild(colorInput);
        }
    });
    // Add logic for removing modal colors
    const removeModalColorButton = document.getElementById('removeModalColorButton');
    removeModalColorButton.addEventListener('click', () => {
        if (modalColorContainer.children.length > 1) {
            modalColorContainer.removeChild(modalColorContainer.lastChild);
        }
    });
    // Add logic for rainbow modal colors button
    const modalRainbowButton = document.getElementById('modalRainbowButton');
    modalRainbowButton.addEventListener('click', () => {
        const colorCount = document.querySelectorAll('#modalColorContainer input').length;
        const rainbowColors = generateRainbowColors(colorCount);
        const colorContainer = document.getElementById('modalColorContainer');
        colorContainer.innerHTML = ''; // Clear existing swatches
        rainbowColors.forEach(color => {
            const colorInput = document.createElement('input');
            colorInput.type = 'color';
            colorInput.value = color;
            colorInput.classList.add('color-swatch');
            // Disable the color swatches when the checkbox is not checked
            const colorEditCheckbox = Array.from(document.getElementById('modal').querySelectorAll('.edit-button-checkbox')).find(checkbox => checkbox.dataset.setting === 'startingColor');
            if (colorEditCheckbox && !colorEditCheckbox.checked) {
                colorInput.disabled = true;
            }
            // Add the event listener here
            colorInput.addEventListener('click', function (event) {
                const colorEditCheckbox = Array.from(document.getElementById('modal').querySelectorAll('.edit-button-checkbox')).find(checkbox => checkbox.dataset.setting === 'startingColor');
                if (colorEditCheckbox && !colorEditCheckbox.checked) {
                    event.preventDefault();
                    return;
                }
                this.showPicker();
            });
            colorContainer.appendChild(colorInput);
        });
    });
    // Add logic for random modal colors button
    const modalRandomButton = document.getElementById('modalRandomButton');
    modalRandomButton.addEventListener('click', () => {
        const colorCount = document.querySelectorAll('#modalColorContainer input').length;
        const randomColors = generateRandomColors(colorCount);
        const colorContainer = document.getElementById('modalColorContainer');
        colorContainer.innerHTML = ''; // Clear existing swatches
        randomColors.forEach(color => {
            const colorInput = document.createElement('input');
            colorInput.type = 'color';
            colorInput.value = color;
            colorInput.classList.add('color-swatch');
            // Disable the color swatches when the checkbox is not checked
            const colorEditCheckbox = Array.from(document.getElementById('modal').querySelectorAll('.edit-button-checkbox')).find(checkbox => checkbox.dataset.setting === 'startingColor');
            if (colorEditCheckbox && !colorEditCheckbox.checked) {
                colorInput.disabled = true;
            }
            // Add the event listener here
            colorInput.addEventListener('click', function (event) {
                const colorEditCheckbox = Array.from(document.getElementById('modal').querySelectorAll('.edit-button-checkbox')).find(checkbox => checkbox.dataset.setting === 'startingColor');
                if (colorEditCheckbox && !colorEditCheckbox.checked) {
                    event.preventDefault();
                    return;
                }
                this.showPicker();
            });
            colorContainer.appendChild(colorInput);
        });
    });
    // Add logic for similar modal colors button
    const modalSimilarButton = document.getElementById('modalSimilarButton');
    modalSimilarButton.addEventListener('click', () => {
        const colorCount = document.querySelectorAll('#modalColorContainer input').length;
        const firstColor = document.querySelector('#modalColorContainer input')?.value;
        const similarColors = generateSimilarColors(colorCount, firstColor);
        const colorContainer = document.getElementById('modalColorContainer');
        colorContainer.innerHTML = ''; // Clear existing swatches
        similarColors.forEach(color => {
            const colorInput = document.createElement('input');
            colorInput.type = 'color';
            colorInput.value = color;
            colorInput.classList.add('color-swatch');
            // Disable the color swatches when the checkbox is not checked
            const colorEditCheckbox = Array.from(document.getElementById('modal').querySelectorAll('.edit-button-checkbox')).find(checkbox => checkbox.dataset.setting === 'startingColor');
            if (colorEditCheckbox && !colorEditCheckbox.checked) {
                colorInput.disabled = true;
            }
            // Add the event listener here
            colorInput.addEventListener('click', function (event) {
                const colorEditCheckbox = Array.from(document.getElementById('modal').querySelectorAll('.edit-button-checkbox')).find(checkbox => checkbox.dataset.setting === 'startingColor');
                if (colorEditCheckbox && !colorEditCheckbox.checked) {
                    event.preventDefault();
                    return;
                }
                this.showPicker();
            });
            colorContainer.appendChild(colorInput);
        });
    });
}

function updateModal(selectedNodes, activeNodes, nodeSpecificSettings, globalSettings, updateNodeStyles) {
    if (modal) {
        modal.globalSettings = globalSettings;
        modal.nodeSpecificSettings = nodeSpecificSettings;
        modal.updateModalDisplay(selectedNodes, activeNodes);
    }
}
export { initModalManager, updateModal };