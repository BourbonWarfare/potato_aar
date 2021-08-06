// main.js
// entry point for web page. Initialised WebGL layer and gets ready to render

const vsSource = 'attribute vec4 aVertexPosition; uniform mat4 uModelViewMatrix; uniform mat4 uProjectionMatrix; void main() { gl_Position = uProjectionMatrix * uModelViewMatrix * aVertexPosition; }';
const fragSource = 'void main() { gl_FragColor = vec4(1.0); }';

// Initialise shader program to draw data
function initShaderProgram(gl, vsSource, fsSource) {
    const vertexShader = loadShader(gl, gl.VERTEX_SHADER, vsSource);
    const fragmentShader = loadShader(gl, gl.FRAGMENT_SHADER, fsSource);

    const shaderProgram = gl.createProgram();
    gl.attachShader(shaderProgram, vertexShader);
    gl.attachShader(shaderProgram, fragmentShader);
    gl.linkProgram(shaderProgram);

    if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
        alert('Unable to initialize the shader program: ' + gl.getProgramInfoLog(shaderProgram));
        return null;
    }

    return shaderProgram;
}

// Compiles a shader string into machine code
function loadShader(gl, type, source) {
    const shader = gl.createShader(type);

    gl.shaderSource(shader, source);
    gl.compileShader(shader);

    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        alert('An error occurred compiling the shaders: ' + gl.getShaderInfoLog(shader));
        gl.deleteShader(shader);
        return null;
    }

    return shader;
}

function Quad(extents) {
    const positions = [
        0, 0,
        extents[0], 0,
        extents[0], extents[1],
        0, extents[1]
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

function drawScene(gl, programInfo, renderObjects) {
    gl.clearColor(0, 0, 0, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);

    const projectionMatrix = mat4.create();
    mat4.ortho(
        projectionMatrix,
        0, 640,
        0, 480,
        -1, 1
    );

    for (const renderObject of renderObjects) {
        const modelViewMatrix = mat4.create();
        mat4.translate(
            modelViewMatrix,
            modelViewMatrix,
            [renderObject.position[0], renderObject.position[1], 0]
        );
        mat4.rotate(
            modelViewMatrix,
            modelViewMatrix,
            renderObject.rotation,
            [0, 0, 1]
        );

        {
            const numComponents = 2;
            const type = gl.FLOAT;
            const normalise = false;
            const stride = 0;

            const offset = 0;
            gl.bindBuffer(gl.ARRAY_BUFFER, renderObject.vertexBuffer);
            gl.vertexAttribPointer(
                programInfo.attribLocations.vertexPosition,
                numComponents,
                type,
                normalise,
                stride,
                offset
            );
            gl.enableVertexAttribArray(
                programInfo.attribLocations.vertexPosition
            );
        }

        gl.useProgram(programInfo.program);

        gl.uniformMatrix4fv(
            programInfo.uniformLocations.projectionMatrix,
            false,
            projectionMatrix
        );
        gl.uniformMatrix4fv(
            programInfo.uniformLocations.modelViewMatrix,
            false,
            modelViewMatrix
        );

        {
            const offset = 0;
            gl.drawArrays(gl.TRIANGLE_STRIP, offset, renderObject.vertexCount);
        }
    }
}

function RenderObject(gl, shape) {
    this.position = [0, 0];
    this.rotation = 0;

    this.vertexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(shape), gl.STATIC_DRAW);

    this.vertexCount = shape.length;
}

function main() {
    const canvas = document.querySelector("#glCanvas");
    const gl = canvas.getContext("webgl");

    if (gl == null) {
        alert("Unable to initialise WebGL. Your browser or machine may not support it.");
        return;
    }
    
    const shaderProgram = initShaderProgram(gl, vsSource, fragSource);

    const programInfo = {
        program: shaderProgram,
        attribLocations: {
            vertexPosition: gl.getAttribLocation(shaderProgram, 'aVertexPosition')
        },
        uniformLocations: {
            projectionMatrix: gl.getUniformLocation(shaderProgram, 'uProjectionMatrix'),
            modelViewMatrix: gl.getUniformLocation(shaderProgram, 'uModelViewMatrix')
        }
    };

    var r0 = new RenderObject(gl, Circle(50, 20));
    r0.position = [100, 100];

    var then = 0;
    function render(now) {
        now *= 0.001;
        const deltaTime = now - then;
        then = now;

        drawScene(gl, programInfo, [r0]);

        requestAnimationFrame(render);
    };

    requestAnimationFrame(render)
}