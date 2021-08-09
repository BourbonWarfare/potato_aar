// svg.js
// interprets SVG document and returns vertex array. Just enough functionality to get ARMA SVG exports working

function hexToRGB(hex) {
    var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? [
        parseInt(result[1], 16) / 255,
        parseInt(result[2], 16) / 255,
        parseInt(result[3], 16) / 255
    ] : null;
}

function handleDefs(defs, colourMap) {
    for (let i = 0; i < defs.children.length; i++) {
        let child = defs.children[i];
        if (child.tagName == 'linearGradient') {
            colourMap.set(child.id, hexToRGB(child.children[0].getAttribute('stop-color')));
        }
    }
}

function getColourFromFill(colourMap, colour, defaultColour = [1, 1, 1]) {
    if (colour == null) {
        return defaultColour;
    }

    if (colour.match(/(?=url)/)) {
        const colourID = /(?<=url\(#).*(?=\))/.exec(colour)[0];
        if (colourMap.has(colourID)) {
            return colourMap.get(colourID);
        }
        return defaultColour;
    }
    return hexToRGB(colour);
}

function createRect(rect, colourMap, vertices, colours, indices, defaultColour) {
    let x = parseFloat(rect.getAttribute('x'));
    let y = parseFloat(rect.getAttribute('y'));
    let w = parseFloat(rect.getAttribute('width'));
    let h = parseFloat(rect.getAttribute('height'));

    let rgb = getColourFromFill(colourMap, rect.getAttribute('fill'), defaultColour);

    let indexOffset = vertices.length / 2;
    vertices.push(x + 0, y + 0);
    vertices.push(x + w, y + 0);
    vertices.push(x + w, y + h);
    vertices.push(x + 0, y + h);

    indices.push(indexOffset + 0);
    indices.push(indexOffset + 1);
    indices.push(indexOffset + 2);
    indices.push(indexOffset + 2);
    indices.push(indexOffset + 3);
    indices.push(indexOffset + 0);

    colours.push(rgb[0], rgb[1], rgb[2]);
    colours.push(rgb[0], rgb[1], rgb[2]);
    colours.push(rgb[0], rgb[1], rgb[2]);
    colours.push(rgb[0], rgb[1], rgb[2]);
}

function createPolygon(polygon, colourMap, vertices, colours, indices, defaultColour) {
    const svgPoints = polygon.getAttribute('points');
    const xyPairs = svgPoints.split(/[, ]/).map(x => parseFloat(x));

    let rgb = getColourFromFill(colourMap, polygon.getAttribute('fill'), defaultColour);
    let points = [];
    const indexOffset = vertices.length / 2;
    for (let i = 0; i < xyPairs.length; i += 2) {
        let x = xyPairs[i + 0];
        let y = xyPairs[i + 1];
        points.push(x, y);
        vertices.push(x, y);
        colours.push(rgb[0], rgb[1], rgb[2]);
    }

    earcut(points, null, 2).forEach(index => {
        indices.push(indexOffset + index);
    });
}

function handleGroup(group, colourMap, vertices, colours, indices, defaultColour) {
    let defaultColourStr = group.getAttribute('fill');
    if (defaultColourStr != null) {
        defaultColour = getColourFromFill(colourMap, defaultColourStr, defaultColour);
    }

    for (let i = 0; i < group.children.length; i++) {
        let child = group.children[i];
        switch (child.tagName) {
            case 'rect':
                createRect(child, colourMap, vertices, colours, indices, defaultColour);
                break;
            case 'polygon':
                createPolygon(child, colourMap, vertices, colours, indices, defaultColour);
                break;
            case 'g':
                handleGroup(child, colourMap, vertices, colours, indices, defaultColour);
                break;
            case 'line':
                break;
            case 'polyline':
                break;
            default:
                break;
        }
    }
}

function parseSVGDoc(document) {
    let colourMap = new Map();

    let vertices = [];
    let colours = [];
    let indices = [];
    
    for (let i = 0; i < document.children.length; i++) {
        let child = document.children[i];
        switch (child.tagName) {
            case 'defs':
                handleDefs(child, colourMap);
                break;
            case 'g':
                handleGroup(child, colourMap, vertices, colours, indices, [1, 1, 1]);
                break;
            default:
                console.log(child.tagName);
                break;
        }
    }

    return {
        vertices: vertices,
        colours: colours,
        indices: indices
    };
}

export { parseSVGDoc };