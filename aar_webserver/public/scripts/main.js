// main.js
// entry point for web page. Initialised WebGL layer and gets ready to render
import { Circle, Quad } from './modules/shapes.js';

const vsSource = 'attribute vec4 aVertexPosition; uniform mat4 uModelMatrix; uniform mat4 uViewMatrix; uniform mat4 uProjectionMatrix; void main() { mat4 mvp = uProjectionMatrix * uViewMatrix * uModelMatrix; gl_Position = mvp * aVertexPosition; }';
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

function drawScene(gl, camera, worldSize, programInfo, renderObjects) {
    gl.clearColor(0, 0, 0, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);

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

        {
            const offset = 0;
            gl.drawArrays(gl.TRIANGLE_STRIP, offset, renderObject.vertexCount);
        }
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

function RenderObject(gl, shape) {
    this.position = [0, 0];
    this.rotation = 0;
    this.origin = [0, 0];
    
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

    this.vertexCount = shape.length;
}

function Projectile(gl, eventArguments) {
    this.renderObject = new RenderObject(gl, Circle(0.5, 15));
    this.position = JSON.parse(eventArguments[1]);
    this.velocity = JSON.parse(eventArguments[2]);

    this.update = function(deltaTime) {
        this.position[0] += this.velocity[0] * deltaTime;
        this.position[1] += this.velocity[1] * deltaTime;
    }

    this.draw = function() {
        this.renderObject.position = this.position;
        return this.renderObject;
    };
}

function GameObject(gl, eventArguments) {
    this.renderObject = new RenderObject(gl, Quad([1, 1]));
    this.position = JSON.parse(eventArguments[2]);
    this.name = JSON.parse(eventArguments[3]);

    this.update = function(deltaTime) {
        
    }

    this.draw = function() {
        this.renderObject.position = this.position;
        return this.renderObject;
    }
}

function Marker(gl, eventArguments) {
    this.renderObject = new RenderObject(gl, Circle(5, 5));
    this.position = JSON.parse(eventArguments[6]);

    this.draw = function() {
        this.renderObject.position = this.position;
        return this.renderObject;
    };
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
            viewMatrix: gl.getUniformLocation(shaderProgram, 'uViewMatrix'),
            modelMatrix: gl.getUniformLocation(shaderProgram, 'uModelMatrix')
        }
    };

    var projectiles = new Map();
    var gameObjects = new Map();
    var markers = new Map();

    var camera = new Camera([1280, 720], canvas);
    camera.position = [14393, 15939];

    var worldSize = 0;

    const ws = new WebSocket("ws://localhost:8082");
    ws.addEventListener("open", () => {
        console.log("conneced to websocket!");

        ws.addEventListener('message', ({ data: incomingData }) => {
            const packet = JSON.parse(incomingData);

            switch (packet.type) {
                case 'init':
                    {
                        worldSize = packet.data.mapSize;
                    }
                    break;
                case 'event':
                    {
                        const uid = JSON.parse(packet.data.arguments[0]);
                        switch (packet.data.type) {
                            case "Object Created":
                                gameObjects.set(
                                    uid,
                                    new GameObject(gl, packet.data.arguments)
                                );
                                console.log(packet.data.arguments);
                                break;
                            case "Marker Created":
                                markers.set(
                                    uid,
                                    new Marker(gl, packet.data.arguments)
                                );
                                break;
                            case "Marker Updated":
                                break;
                            case "Fired":
                                projectiles.set(
                                    uid,
                                    new Projectile(gl, packet.data.arguments)
                                );
                                break;
                        };
                    }
                    break;
            }
        });
    });
    
    var then = 0;
    function render(now) {
        now *= 0.001;
        const deltaTime = now - then;
        then = now;

        var objectsToRender = [];
        projectiles.forEach(projectile => {
            projectile.update(deltaTime);
            objectsToRender.push(projectile.draw());
        });

        gameObjects.forEach(gameObject => {
            objectsToRender.push(gameObject.draw());
        });

        markers.forEach(marker => {
            objectsToRender.push(marker.draw());
        });
        
        drawScene(gl, camera, worldSize, programInfo, objectsToRender);

        requestAnimationFrame(render);
    };

    requestAnimationFrame(render)
}

export default main;
