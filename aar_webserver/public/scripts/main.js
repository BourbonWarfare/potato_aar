// main.js
// entry point for web page. Initialised WebGL layer and gets ready to render
import { Circle, Quad } from './modules/shapes.js';

const vsSource = 'attribute vec4 aVertexPosition; uniform mat4 uModelMatrix; uniform mat4 uViewMatrix; uniform mat4 uProjectionMatrix; void main() { gl_Position = uProjectionMatrix * uModelMatrix * uViewMatrix * aVertexPosition; }';
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

    const viewMatrix = mat4.create();
    mat4.translate(
        viewMatrix,
        viewMatrix,
        [0, 0, 0]
    );

    mat4.scale(
        viewMatrix,
        viewMatrix,
        [1, 1, 1]
    );

    for (const renderObject of renderObjects) {
        const modelMatrix = mat4.create();
        mat4.translate(
            modelMatrix,
            modelMatrix,
            [renderObject.position[0], renderObject.position[1], 0]
        );
        mat4.rotate(
            modelMatrix,
            modelMatrix,
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

function RenderObject(gl, shape) {
    this.position = [0, 0];
    this.rotation = 0;

    this.vertexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, this.vertexBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(shape), gl.STATIC_DRAW);

    this.vertexCount = shape.length;
}

function Projectile(gl, eventArguments) {
    this.renderObject = new RenderObject(gl, Circle(1, 15));
    this.position = JSON.parse(eventArguments[1]);
    this.velocity = JSON.parse(eventArguments[2]);

    this.update = function(deltaTime) {
        this.position[0] += this.velocity[0] * deltaTime;
        this.position[1] += this.velocity[1] * deltaTime;
    }

    this.draw = function(mapScale) {
        this.renderObject.position = [
            this.position[0] * mapScale[0],
            this.position[1] * mapScale[1]
        ];
        return this.renderObject;
    };
}

function GameObject(gl, eventArguments) {
    this.renderObject = new RenderObject(gl, Quad([5, 5]));
    this.position = JSON.parse(eventArguments[2]);
    this.name = JSON.parse(eventArguments[3]);

    this.update = function(deltaTime) {
        
    }

    this.draw = function(mapScale) {
        this.renderObject.position = [
            this.position[0] * mapScale[0],
            this.position[1] * mapScale[1]
        ];
        return this.renderObject;
    }
}

function Marker(gl, eventArguments) {
    this.renderObject = new RenderObject(gl, Circle(5, 5));
    this.position = JSON.parse(eventArguments[6]);

    this.draw = function(mapScale) {
        this.renderObject.position = [
            this.position[0] * mapScale[0],
            this.position[1] * mapScale[1]
        ];
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

    var mapScale = [0, 0];

    const ws = new WebSocket("ws://localhost:8082");
    ws.addEventListener("open", () => {
        console.log("conneced to websocket!");

        ws.addEventListener('message', ({ data: incomingData }) => {
            const packet = JSON.parse(incomingData);

            switch (packet.type) {
                case 'init':
                    {
                        mapScale = [
                            640 / packet.data.mapSize,
                            480 / packet.data.mapSize
                        ];
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
            objectsToRender.push(projectile.draw(mapScale));
        });

        gameObjects.forEach(gameObject => {
            objectsToRender.push(gameObject.draw(mapScale));
        });

        markers.forEach(marker => {
            objectsToRender.push(marker.draw(mapScale));
        });

        drawScene(gl, programInfo, objectsToRender);

        requestAnimationFrame(render);
    };

    requestAnimationFrame(render)
}

export default main;
