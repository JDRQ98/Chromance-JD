// colorUtils.js

// Utility function to generate rainbow colors
function generateRainbowColors(numColors) {
    const colors = [];
    for (let i = 0; i < numColors; i++) {
        const hue = (i * (360 / numColors)) % 360;
        const color = hslToHex(hue, 100, 50);
        colors.push(color);
    }
    return colors;
}

// Utility function to generate random colors
function generateRandomColors(numColors) {
    const colors = [];
    for (let i = 0; i < numColors; i++) {
        const color = getRandomHexColor();
        colors.push(color);
    }
    return colors;
}
// Utility function to generate similar colors
function generateSimilarColors(numColors, baseColor) {
    const colors = [];
    if (baseColor) {
        const baseHsl = hexToHsl(baseColor)
        colors.push(baseColor)
        for (let i = 1; i < numColors; i++) {
            const hue = (baseHsl.h + (Math.random() * 60 - 30)) % 360;
            const saturation = Math.max(0, Math.min(100, baseHsl.s + (Math.random() * 20 - 10)));
            const lightness = Math.max(0, Math.min(100, baseHsl.l + (Math.random() * 20 - 10)));
            const color = hslToHex(hue, saturation, lightness);
            colors.push(color);
        }
    }
    return colors;
}
function getRandomHexColor() {
    const randomColor = Math.floor(Math.random() * 16777215).toString(16);
    return "#" + randomColor;
}
function hslToHex(h, s, l) {
    h /= 360;
    s /= 100;
    l /= 100;
    let r, g, b;
    if (s === 0) {
        r = g = b = l; // achromatic
    } else {
        const hue2rgb = (p, q, t) => {
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t < 1 / 6) return p + (q - p) * 6 * t;
            if (t < 1 / 2) return q;
            if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
            return p;
        };
        const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        const p = 2 * l - q;
        r = hue2rgb(p, q, h + 1 / 3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1 / 3);
    }
    const toHex = x => {
        const hex = Math.round(x * 255).toString(16);
        return hex.length === 1 ? '0' + hex : hex;
    };
    return `#${toHex(r)}${toHex(g)}${toHex(b)}`;
}
function hexToHsl(hex) {
    let result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    let r = parseInt(result[1], 16);
    let g = parseInt(result[2], 16);
    let b = parseInt(result[3], 16);
    r /= 255, g /= 255, b /= 255;
    let max = Math.max(r, g, b), min = Math.min(r, g, b);
    let h, s, l = (max + min) / 2;
    if (max === min) {
        h = s = 0; // achromatic
    } else {
        let d = max - min;
        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
        switch (max) {
            case r: h = (g - b) / d + (g < b ? 6 : 0); break;
            case g: h = (b - r) / d + 2; break;
            case b: h = (r - g) / d + 4; break;
        }
        h /= 6;
    }
    return { h: h * 360, s: s * 100, l: l * 100 };
}

export { generateRainbowColors, generateRandomColors, generateSimilarColors };