// drawVisualizer.js
// Function to calculate the distance between two points
function distance(x1, y1, x2, y2) {
    return Math.sqrt((x2 - x1) ** 2 + (y2 - y1) ** 2);
}

// Function to calculate the angle between two points in degrees
function angle(x1, y1, x2, y2) {
    return Math.atan2(y2 - y1, x2 - x1) * (180 / Math.PI);
}

// Function to draw a line between two nodes
function drawLine(nodeId1, nodeId2) {
    // Get the container and the nodes by their IDs
    const container = document.getElementById('container');
    const nodeWrap1 = document.querySelector(`.hex-wrap [data-id="${nodeId1}"]`).closest('.hex-wrap');
    const nodeWrap2 = document.querySelector(`.hex-wrap [data-id="${nodeId2}"]`).closest('.hex-wrap');

    // If either node doesn't exist, return
    if (!nodeWrap1 || !nodeWrap2) {
        console.error('Invalid node IDs');
        return;
    }

    // Get the positions of the nodes from the wrapper
    const x1 = parseFloat(nodeWrap1.style.left);
    const y1 = parseFloat(nodeWrap1.style.top);
    const x2 = parseFloat(nodeWrap2.style.left);
    const y2 = parseFloat(nodeWrap2.style.top);

    // Calculate the distance and angle
    const dist = distance(x1, y1, x2, y2);
    const ang = angle(x1, y1, x2, y2);

    // Create a new line element
    const line = document.createElement('div');
    line.classList.add('line');

    // Set the line's width to the distance between the nodes
    line.style.width = `${dist}px`;

    // Set the line's rotation to the angle between the nodes
    line.style.transform = `rotate(${ang}deg)`;

    // Set the line's position at the center of node1
    line.style.left = `${x1 + 15}px`; // MAGIC NUMBER: 15 = half of hexagon width. TODO.
    line.style.top = `${y1 + 15}px`; // MAGIC NUMBER: 15 = half of hexagon height. TODO.

    // Append the line to the container
    container.appendChild(line);
}

// Function to draw the hexagon lines
function drawHexagon() {
    drawLine(0, 1);
    drawLine(17, 18);
    drawLine(15, 17);
    drawLine(10, 15);
    drawLine(7, 10);
    drawLine(7, 9);
    drawLine(9, 12);
    drawLine(12, 17);
    drawLine(14, 17);
    drawLine(4, 9);
    drawLine(1, 4);
    drawLine(0, 2);
    drawLine(2, 5);
    drawLine(5, 10);
    drawLine(10, 12);
    drawLine(16, 18);
    drawLine(13, 16);
    drawLine(8, 13);
    drawLine(6, 8);
    drawLine(6, 9);
    drawLine(9, 14);
    drawLine(14, 16);
    drawLine(11, 16);
    drawLine(8, 11);
    drawLine(3, 8);
    drawLine(1, 3);
    drawLine(1, 6);
    drawLine(9, 11);
    drawLine(2, 4);
    drawLine(2, 7);
}
export { drawHexagon }; // Export statement at the top level