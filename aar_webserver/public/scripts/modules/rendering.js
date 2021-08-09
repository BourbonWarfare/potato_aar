// rendering.js
// helper functions for rendering a scene in WebGL

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

function drawScene(gl, camera, programInfo, renderObjects) {
    const projectionMatrix = mat4.ortho(
        mat4.create(),
        0, camera.size[0],
        0, camera.size[1],
        -1, 1
    );

    const viewMatrix = mat4.fromRotationTranslationScaleOrigin(
        mat4.create(),
        quat.fromEuler(quat.create(), 0, 0, 0),
        [-camera.position[0] * camera.zoom, -camera.position[1] * camera.zoom, 0],
        [camera.zoom, camera.zoom, 1],
        [camera.size[0] / 2, camera.size[1] / 2, 0]
    );
    mat4.translate(
        viewMatrix,
        viewMatrix,
        [camera.size[0] / 2, camera.size[1] / 2, 0]
    );

    for (const renderObject of renderObjects) {
        const modelMatrix = mat4.fromRotationTranslationScaleOrigin(
            mat4.create(),
            quat.fromEuler(quat.create(), 0, 0, renderObject.rotation),
            [renderObject.position[0], renderObject.position[1], 0],
            [1, 1, 1],
            [renderObject.origin[0], renderObject.origin[1], 0]
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

        {
            const numComponents = 3;
            const type = gl.FLOAT;
            const normalize = false;
            const stride = 0;
            const offset = 0;
            gl.bindBuffer(gl.ARRAY_BUFFER, renderObject.colourBuffer);
            gl.vertexAttribPointer(
                programInfo.attribLocations.vertexColour,
                numComponents,
                type,
                normalize,
                stride,
                offset
            );
            gl.enableVertexAttribArray(
                programInfo.attribLocations.vertexColour
            );
          }

        gl.useProgram(programInfo.program);

        gl.uniformMatrix4fv(
            programInfo.uniformLocations.projectionMatrix,
            false,
            projectionMatrix
        );
        gl.uniformMatrix4fv(
            programInfo.uniformLocations.viewMatrix,
            false,
            viewMatrix
        );
        gl.uniformMatrix4fv(
            programInfo.uniformLocations.modelMatrix,
            false,
            modelMatrix
        );

        if (!programInfo.useIndexBuffer) {
            gl.drawArrays(programInfo.primitiveType, 0, renderObject.vertexCount);
        } else {
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, renderObject.indexBuffer);
            gl.drawElements(programInfo.primitiveType, renderObject.indexCount, gl.UNSIGNED_INT, 0);
        }
    }
}


function RenderObject(gl, shape, indices = [], colours = []) {
    this.position = [0, 0];
    this.rotation = 0;
    this.origin = [0, 0];
    this.vertexCount = shape.length;
    this.indexCount = indices.length;

    let pointCount = shape.length / 2;
    for (let i = 0; i < shape.length; i += 2) {
        this.origin[0] += shape[i + 0];
        this.origin[1] += shape[i + 1];
    }
    this.origin[0] /= pointCount;
    this.origin[1] /= pointCount;

    this.vertexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(shape), gl.STATIC_DRAW);

    this.colourBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, this.colourBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(colours), gl.STATIC_DRAW);

    this.indexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.indexBuffer);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint32Array(indices), gl.STATIC_DRAW);

    this.setColour = function(colour) {
        let colours = [];
        for (let i = 0; i < this.vertexCount; i++) {
            colours.push(colour[0], colour[1], colour[2]);
        }

        gl.bindBuffer(gl.ARRAY_BUFFER, this.colourBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(colours), gl.STATIC_DRAW);
    }
}

function Camera(size, workingElement) {
    this.position = [0, 0];
    this.zoom = 1;
    this.size = size;

    this.minZoom = 0.05;
    this.maxZoom = 50;

    this.clicked = false;
    this.clickPosition = [0, 0];
    this.positionBeforeMove = [0, 0];

    workingElement.addEventListener('mousedown', e => {
        const rect = e.target.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;

        this.clickPosition = [x, y];
        this.positionBeforeMove = this.position;
        this.clicked = true;
    });

    workingElement.addEventListener('mouseup', e => {
        this.clicked = false;
    });

    workingElement.addEventListener('mousemove', e => {
        if (!this.clicked) { return; }

        const rect = e.target.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;

        const offset = [
            x - this.clickPosition[0],
            y - this.clickPosition[1]
        ];

        const sizeModifierX = this.size[0] / rect.width;
        const sizeModifierY = this.size[1] / rect.height;

        this.position = [
            this.positionBeforeMove[0] - offset[0] * sizeModifierX / this.zoom,
            this.positionBeforeMove[1] + offset[1] * sizeModifierY / this.zoom
        ];
    });

    workingElement.addEventListener('wheel', e => {
        e.preventDefault();

        const rect = e.target.getBoundingClientRect();
        const x = e.clientX - rect.left;
        const y = e.clientY - rect.top;

        const mid = (this.maxZoom + this.minZoom) / 2;

        const zoomSpeed = 1;
        const currentZoom = this.zoom;

        const a = currentZoom / mid - 1;

        const zoomIncrement = Math.sqrt(zoomSpeed - a * a);
        this.zoom += zoomIncrement * Math.sign(e.wheelDeltaY);
        this.zoom = Math.max(this.minZoom, Math.min(this.zoom, this.maxZoom));
    });
}

export {
    RenderObject,
    Camera,
    drawScene,
    initShaderProgram
}
