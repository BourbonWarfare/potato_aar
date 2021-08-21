// shapes.js
// A set of basic primitives that one can draw

function Quad(extents) {
    const positions = [
        0, 0,
        extents[0], 0,
        extents[0], extents[1],
        0, extents[1],
        0, 0
    ];
    return positions;
}

function Circle(radius, segments) {
    let positions = [];
    let angleIncrement = 2 * Math.PI / segments;
    for (let i = 0; i < 2 * Math.PI; i += angleIncrement) {
        positions.push(0, 0);
        positions.push(radius * Math.cos(i), radius * Math.sin(i));
        positions.push(radius * Math.cos(i + angleIncrement), radius * Math.sin(i + angleIncrement));
    }
    return positions;
}

function Line(x0, y0, x1, y1) {
    const positions = [
        x0, y0,
        x1, y1
    ];
    return positions;
}

export {
    Quad,
    Circle,
    Line
};
