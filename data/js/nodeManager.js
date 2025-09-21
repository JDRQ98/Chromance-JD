// File: /js/nodeManager.js

class Node {
    constructor(id, element, wrapper, globalSettings, nodeSpecificSettings) {
        this.id = id;
        this.element = element;
        this.wrapper = wrapper;
        this.state = 'inactive';
        this.globalSettings = globalSettings;
        this.nodeSpecificSettings = nodeSpecificSettings;
        this.element.addEventListener('click', () => this.toggleSelect());
    }
    getSettings() {
        return this.nodeSpecificSettings[this.id] || this.globalSettings
    }
    toggleSelect() {
       console.log(`Node ${this.id} toggleSelect called. Current state: ${this.state}`); // ADDED LOG
        if (this.state === 'inactive' || this.state === 'active') {
             window.nodeManager.addSelectedNode(this)
             this.select();
        } else {
            window.nodeManager.removeSelectedNode(this)
            this.deselect();
        }
        if (window.modal) { // Check if window.modal is defined
            console.log(`Updating modal from Node ${this.id}`);  // ADDED LOG
            window.modal.updateModalDisplay(window.nodeManager.getSelectedNodes().map(node => node.element), window.nodeManager.getActiveNodes().map(node => node.id))
        }
    }
    select() {
        this.state = this.state === 'active' ? 'activeandselected' : 'selected';
        this.updateStyle();
    }

    deselect() {
        this.state = this.state === 'activeandselected' ? 'active' : 'inactive';
        this.updateStyle();
    }

    activate() {
        this.state = this.state === 'selected' ? 'activeandselected' : 'active';
        this.updateStyle();
    }

    deactivate() {
        this.state = this.state === 'activeandselected' ? 'selected' : 'inactive';
        this.updateStyle();
    }
    updateStyle() {
        // Remove all style classes
        this.wrapper.classList.remove('regularNode', 'SelectedNode', 'ActiveNode', 'ActiveandSelectedNode');
        // Add the class according to the node's status
        if (this.state === 'activeandselected') {
            this.wrapper.classList.add('ActiveandSelectedNode');
            this.applyColorAnimation();
        } else if (this.state === 'active') {
            this.wrapper.classList.add('ActiveNode');
            this.applyColorAnimation();
        } else if (this.state === 'selected') {
            this.wrapper.classList.add('SelectedNode');
        } else {
            this.wrapper.classList.add('regularNode');
        }
    }
    applyColorAnimation() {
        let colors = [];
        let duration = 0;
        const settings = this.getSettings();
        if (settings.startingColor) {
            colors = settings.startingColor;
        } else {
            colors = settings.colors;
        }
        if (settings.rippleDelay) {
            duration = settings.rippleDelay;
        } else {
            duration = settings.rippleDelay;
        }
        if (!Array.isArray(colors)) {
            colors = [colors];
        }
        // Ensure duration is a number
        if (typeof duration !== 'number') {
            duration = Number(duration);
        }
        //Set a default color if no colors are present
        if (colors.length === 0) {
            colors = ['#FF0000']
        }
        // Set the CSS variables for the animation
        colors.forEach((color, index) => {
            this.wrapper.style.setProperty(`--active-node-color-${index}`, color);
        });
        // If there are less than 6 colors, repeat the colors to fill the gaps
        for (let i = colors.length; i < 6; i++) {
            this.wrapper.style.setProperty(`--active-node-color-${i}`, colors[i % colors.length]);
        }
        // Set the duration
        this.wrapper.style.setProperty('--pulse-duration', `${duration / 1000}s`);
    }
}

class NodeManager {
    constructor(globalSettings, nodeSpecificSettings) {
        this.nodes = [];
        this.globalSettings = globalSettings;
        this.nodeSpecificSettings = nodeSpecificSettings;
        this.selectedNodes = [];
        this.activeNodes = [];
        this.borderNodes = [0, 3, 5, 13, 15, 18];
        this.triNodes = [4, 6, 7, 11, 12, 14];
        this.quadNodes = [1, 2, 8, 10, 16, 17];
        this.initializeNodes();
        window.nodeManager = this;
    }

    initializeNodes() {
        const nodeElements = document.querySelectorAll('.hex');
        nodeElements.forEach(element => {
            const id = Number(element.dataset.id);
            const wrapper = element.closest('.hex-wrap');
            const node = new Node(id, element, wrapper, this.globalSettings, this.nodeSpecificSettings);
            this.nodes.push(node);
        });
        //Set node 9 as active by default
        this.activateNodes([9])
    }

    getNode(id) {
        return this.nodes.find(node => node.id === id);
    }
    getSelectedNodes() {
        return this.selectedNodes;
    }
    getActiveNodes() {
        return this.activeNodes;
    }
    addSelectedNode(node){
           if (!this.selectedNodes.includes(node)){
               this.selectedNodes.push(node)
           }
    }
    removeSelectedNode(node){
         this.selectedNodes = this.selectedNodes.filter(selectedNode => selectedNode !== node)
    }
    selectNodes(ids) {
        this.nodes.forEach(node => {
            if (ids.includes(node.id)) {
                if (!this.selectedNodes.includes(node)) {
                    this.selectedNodes.push(node)
                }
                node.toggleSelect();
            }
        });
        if (window.modal) {
            console.log("Updating modal from NodeManager selectNodes"); // ADDED LOG
            window.modal.updateModalDisplay(this.selectedNodes.map(node => node.element), this.activeNodes.map(node => node.id))
        }
    }
    deselectNodes(ids) {
        this.nodes.forEach(node => {
            if (ids.includes(node.id)) {
                 this.selectedNodes = this.selectedNodes.filter(selectedNode => selectedNode !== node)
                node.deselect();
            }
        });
        if (window.modal) {
            console.log("Updating modal from NodeManager deselectNodes"); // ADDED LOG
            window.modal.updateModalDisplay(this.selectedNodes.map(node => node.element), this.activeNodes.map(node => node.id))
        }
    }
    activateNodes(ids) {
      this.nodes.forEach(node => {
           if (ids.includes(node.id)) {
                if (!this.activeNodes.includes(node)){
                   this.activeNodes.push(node)
                }
                node.activate();
            }
        });
    }
    deactivateNodes(ids) {
        this.nodes.forEach(node => {
            if (ids.includes(node.id)) {
                 this.activeNodes = this.activeNodes.filter(activeNode => activeNode !== node);
                node.deactivate();
            }
        });
    }
    deactivateAllNodes(){
        this.nodes.forEach(node => {
            this.activeNodes = [];
             node.deactivate();
         });
        this.selectedNodes = [];
        if (window.modal) {
            console.log("Updating modal from NodeManager deactivateAllNodes"); // ADDED LOG
            window.modal.updateModalDisplay(this.selectedNodes.map(node => node.element), this.activeNodes.map(node => node.id))
        }
    }
     updateStyles() {
        this.nodes.forEach(node => node.updateStyle());
    }
      deselectAllNodes(){ //Added method
         this.selectedNodes.forEach(node => node.deselect());
          this.selectedNodes = [];
    }
     selectBiNodes() {
         this.selectNodes(this.borderNodes);
     }
     selectTriNodes() {
          this.selectNodes(this.triNodes);
     }
     selectQuadNodes() {
          this.selectNodes(this.quadNodes);
     }
      selectAllActive() {
          this.selectNodes(this.activeNodes.map(node => node.id));
     }
}


let nodeManager;

function initNodeManager(updateNodeStyles, updateModal, globalSettings, nodeSpecificSettings, updateCurrentEffect) {
    nodeManager = new NodeManager(globalSettings, nodeSpecificSettings);
       // Event listeners for selecting node categories
    console.log('initNodeManager called') // ADDED LOG
    document.getElementById('selectBiNodes').addEventListener('click', () => {
         console.log('selectBiNodes button pressed') // ADDED LOG
          nodeManager.selectBiNodes();
           updateModal(nodeManager.getSelectedNodes().map(node => node.element), nodeManager.getActiveNodes().map(node => node.id), nodeSpecificSettings, globalSettings, updateNodeStyles);
    });
    document.getElementById('selectTriNodes').addEventListener('click', () => {
         console.log('selectTriNodes button pressed') // ADDED LOG
        nodeManager.selectTriNodes()
          updateModal(nodeManager.getSelectedNodes().map(node => node.element), nodeManager.getActiveNodes().map(node => node.id), nodeSpecificSettings, globalSettings, updateNodeStyles);
    });
    document.getElementById('selectQuadNodes').addEventListener('click', () => {
         console.log('selectQuadNodes button pressed') // ADDED LOG
        nodeManager.selectQuadNodes()
          updateModal(nodeManager.getSelectedNodes().map(node => node.element), nodeManager.getActiveNodes().map(node => node.id), nodeSpecificSettings, globalSettings, updateNodeStyles);
    });
     document.getElementById('selectAllActive').addEventListener('click', () => {
          console.log('selectAllActive button pressed') // ADDED LOG
         nodeManager.selectAllActive();
           updateModal(nodeManager.getSelectedNodes().map(node => node.element), nodeManager.getActiveNodes().map(node => node.id), nodeSpecificSettings, globalSettings, updateNodeStyles);
     });
     document.getElementById('deselectAll').addEventListener('click', () => {
          console.log('deselectAll button pressed') // ADDED LOG
         nodeManager.deselectAllNodes();
         updateModal([], nodeManager.getActiveNodes().map(node => node.id), nodeSpecificSettings, globalSettings, updateNodeStyles);
    });
    document.getElementById('deactivateAllNodes').addEventListener('click', () => {
          console.log('deactivateAllNodes button pressed') // ADDED LOG
         nodeManager.deactivateAllNodes()
        updateModal([], nodeManager.getActiveNodes().map(node => node.id), nodeSpecificSettings, globalSettings, updateNodeStyles);
        updateCurrentEffect(globalSettings, nodeSpecificSettings, nodeManager.getActiveNodes().map(node => node.id));
    });
    document.getElementById('applyChanges').addEventListener('click', () => {
        console.log('Apply Changes button pressed');
        // Call the function to send the configuration to the microcontroller.
        window.mainJS.sendConfigurationToMicrocontroller();  // Access through window to avoid scope issues
    });
      updateNodeStyles(globalSettings, nodeSpecificSettings);
}


//Functions to be used outside this module
function updateNodeStyles(globalSettings, nodeSpecificSettings) {
    if (nodeManager) {
      nodeManager.globalSettings = globalSettings;
        nodeManager.nodeSpecificSettings = nodeSpecificSettings;
        nodeManager.updateStyles();
    }
}
function setActiveNodes(newActiveNodes) {
    if (nodeManager) {
        nodeManager.deactivateNodes(nodeManager.getActiveNodes().map(node => node.id))
        nodeManager.activateNodes(newActiveNodes)
    }
}
function getActiveNodes() {
     if (nodeManager) {
         return nodeManager.getActiveNodes().map(node => node.id)
     }
     return [];
}
export {
    initNodeManager,
    updateNodeStyles,
    setActiveNodes,
    getActiveNodes
};