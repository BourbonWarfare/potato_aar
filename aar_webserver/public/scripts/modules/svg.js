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
    const x = parseFloat(rect.getAttribute('x'));
    const y = parseFloat(rect.getAttribute('y'));
    const w = parseFloat(rect.getAttribute('width'));
    const h = parseFloat(rect.getAttribute('height'));

    const rgb = getColourFromFill(colourMap, rect.getAttribute('fill'), defaultColour);

    const indexOffset = vertices.length / 2;
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

    const rgb = getColourFromFill(colourMap, polygon.getAttribute('fill'), defaultColour);
    let points = [];
    const indexOffset = vertices.length / 2;
    for (let i = 0; i < xyPairs.length; i += 2) {
        const x = xyPairs[i + 0];
        const y = xyPairs[i + 1];
        points.push(x, y);
        vertices.push(x, y);
        colours.push(rgb[0], rgb[1], rgb[2]);
    }

    earcut(points, null, 2).forEach(index => {
        indices.push(indexOffset + index);
    });
}

function createLineShape(x0, y0, x1, y1, vertices, colours, indices, rgb, width) {
    var directionX = x1 - x0;
    var directionY = y1 - y0;

    var magnitude = Math.sqrt(directionX * directionX + directionY * directionY);
    if (magnitude == 0) {
        return;
    }

    directionX /= magnitude;
    directionY /= magnitude;

    var normalDirectionX = directionY;
    var normalDirectionY = -directionX;
    
    const indexOffset = vertices.length / 2;
    vertices.push(x0 + (-normalDirectionX - directionX) * width, y0 + (-normalDirectionY - directionY) * width);
    vertices.push(x1 + (directionX - normalDirectionX) * width, y1 + (directionY - normalDirectionY) * width);
    vertices.push(x1 + (directionX + normalDirectionX) * width, y1 + (directionY + normalDirectionY) * width);
    vertices.push(x0 + (normalDirectionX - directionX) * width, y0 + (normalDirectionY - directionY) * width);

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

function createLine(line, colourMap, vertices, colours, indices, defaultColour, width) {
    const x0 = parseFloat(line.getAttribute('x1'));
    const y0 = parseFloat(line.getAttribute('y1'));

    const x1 = parseFloat(line.getAttribute('x2'));
    const y1 = parseFloat(line.getAttribute('y2'));

    const rgb = getColourFromFill(colourMap, line.getAttribute('fill'), [0, 0, 0]);

    createLineShape(x0, y0, x1, y1, vertices, colours, indices, rgb, width);
}

function createPolyline(polyline, colourMap, vertices, colours, indices, defaultColour, defaultWidth) {
    const svgPoints = polyline.getAttribute('points');
    const xyPairs = svgPoints.split(/[, ]/).map(x => parseFloat(x));

    const rgb = getColourFromFill(colourMap, polyline.getAttribute('fill'), defaultColour);

    var width = defaultWidth;
    if (polyline.hasAttribute('stroke-width')) {
        width = 0.5 * parseFloat(polyline.getAttribute('stroke-width'));
    }

    for (let i = 0; i < xyPairs.length - 2; i += 2) {
        const x0 = xyPairs[i + 0];
        const y0 = xyPairs[i + 1];

        const x1 = xyPairs[i + 2];
        const y1 = xyPairs[i + 3];

        createLineShape(x0, y0, x1, y1, vertices, colours, indices, rgb, width);
    }
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
            case 'line':
                createLine(child, colourMap, vertices, colours, indices, defaultColour, 0.5);
                break;
            case 'polyline':
                createPolyline(child, colourMap, vertices, colours, indices, defaultColour, 2);
                break;
            case 'g':
                handleGroup(child, colourMap, vertices, colours, indices, defaultColour);
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