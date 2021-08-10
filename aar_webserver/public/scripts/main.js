// main.js
// entry point for web page. Initialised WebGL layer and gets ready to render
import { Circle, Quad, Line } from './modules/shapes.js';
import { parseSVGDoc } from './modules/svg.js';
import { RenderObject, Camera, drawScene, initShaderProgram } from './modules/rendering.js';

const vsSource = `
    attribute vec2 aVertexPosition;
    attribute vec3 aVertexColour;

    uniform mat4 uModelMatrix;
    uniform mat4 uViewMatrix;
    uniform mat4 uProjectionMatrix;

    varying lowp vec4 vColour;
    
    void main() {
        mat4 mvp = uProjectionMatrix * uViewMatrix * uModelMatrix;
        gl_Position = mvp * vec4(aVertexPosition, 0, 1); 
        vColour = vec4(aVertexColour, 1);
    }
`;
const fragSource = `
    varying lowp vec4 vColour;

    void main() {
        gl_FragColor = vColour;
    }
`;

function Projectile(gl, eventArguments, currentTime) {
    this.renderObject = new RenderObject(gl, Line(0, 0, 0, 0));
    this.renderObject.useModelMatrix = false;

    this.uid = JSON.parse(eventArguments[0]);
    this.origin = JSON.parse(eventArguments[1]);
    this.startTime = currentTime;
    this.position = this.origin;
    this.velocity = JSON.parse(eventArguments[2]);
    this.lifetime = JSON.parse(eventArguments[4]);
    this.endTime = currentTime + this.lifetime;

    this.forceState = function(time) {
        this.position[0] = this.origin[0] + this.velocity[0] * (time - this.startTime);
        this.position[1] = this.origin[1] + this.velocity[1] * (time - this.startTime);
    }

    this.update = function(gl, deltaTime) {
        const oldPosX = this.position[0];
        const oldPosY = this.position[1];

        this.position[0] += this.velocity[0] * deltaTime;
        this.position[1] += this.velocity[1] * deltaTime;

        gl.bindBuffer(gl.ARRAY_BUFFER, this.renderObject.vertexBuffer);
        gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(Line(oldPosX, oldPosY, this.position[0], this.position[1])), gl.STATIC_DRAW);
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

    this.futurePositions = [];
    this.currentState = 0;

    this.currentInterpolationTime = 0;
    this.interpolationBeginPosition = this.position;
    this.timeOffset = 0;
    this.firstUpdate = true;
    this.active = true;

    this.forceState = function(desiredTime) {
        this.currentState = 0;
        for (let i = 0; i < this.futurePositions.length; i++) {
            if (this.futurePositions[i].time > desiredTime) {
                break;
            }
            this.currentState = i;
        }

        this.position = this.futurePositions[this.currentState].position;
        this.interpolationBeginPosition = this.position;
        this.timeOffset = desiredTime;
        this.currentInterpolationTime = 0;
        this.firstUpdate = true;
    }

    this.update = function(deltaTime) {
        if (this.currentState >= this.futurePositions.length) { return; }

        this.currentInterpolationTime += deltaTime;
        if (this.firstUpdate) {
            this.position = this.futurePositions[0].position;
            this.interpolationBeginPosition = this.position;

            this.firstUpdate = false;
            this.currentState += 1;
        } else {
            let desiredPosition = this.futurePositions[this.currentState].position;
            let desiredTime = this.futurePositions[this.currentState].time - this.timeOffset;

            const leftToGo = this.currentInterpolationTime / desiredTime;

            // lerp between known states
            this.position[0] = this.interpolationBeginPosition[0] + leftToGo * (desiredPosition[0] - this.interpolationBeginPosition[0]);
            this.position[1] = this.interpolationBeginPosition[1] + leftToGo * (desiredPosition[1] - this.interpolationBeginPosition[1]);

            if (leftToGo >= 1) {
                this.position = desiredPosition;
                this.interpolationBeginPosition = desiredPosition;
                this.timeOffset += desiredTime;
                this.currentInterpolationTime = 0;
                this.currentState += 1;
            }
        }
    }

    this.updateFromPacket = function(state) {
        this.futurePositions.push({
            position: state.position,
            time: state.time
        });
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

function Event(onEvent, onUndo) {
    this.forward = onEvent;
    this.backward = onUndo;
}

function main() {
    var testObject = null;

    const canvas = document.querySelector("#glCanvas");
    const gl = canvas.getContext("webgl");

    if (gl == null) {
        alert("Unable to initialise WebGL. Your browser or machine may not support it.");
        return;
    }

    var ext = gl.getExtension('OES_element_index_uint');
    if (ext == null) {
        alert("Unable to use extension");
        return;
    }
    
    const shaderProgram = initShaderProgram(gl, vsSource, fragSource);

    const objectInfo = {
        program: shaderProgram,
        primitiveType: gl.TRIANGLE_STRIP,
        useIndexBuffer: false,
        attribLocations: {
            vertexPosition: gl.getAttribLocation(shaderProgram, 'aVertexPosition'),
            vertexColour: gl.getAttribLocation(shaderProgram, 'aVertexColour')
        },
        uniformLocations: {
            projectionMatrix: gl.getUniformLocation(shaderProgram, 'uProjectionMatrix'),
            viewMatrix: gl.getUniformLocation(shaderProgram, 'uViewMatrix'),
            modelMatrix: gl.getUniformLocation(shaderProgram, 'uModelMatrix')
        }
    }

    const terrainInfo = {
        program: shaderProgram,
        primitiveType: gl.TRIANGLES,
        useIndexBuffer: true,
        attribLocations: {
            vertexPosition: gl.getAttribLocation(shaderProgram, 'aVertexPosition'),
            vertexColour: gl.getAttribLocation(shaderProgram, 'aVertexColour')
        },
        uniformLocations: {
            projectionMatrix: gl.getUniformLocation(shaderProgram, 'uProjectionMatrix'),
            viewMatrix: gl.getUniformLocation(shaderProgram, 'uViewMatrix'),
            modelMatrix: gl.getUniformLocation(shaderProgram, 'uModelMatrix')
        }
    };

    const lineInfo = {
        program: shaderProgram,
        primitiveType: gl.LINES,
        useIndexBuffer: false,
        attribLocations: {
            vertexPosition: gl.getAttribLocation(shaderProgram, 'aVertexPosition'),
            vertexColour: gl.getAttribLocation(shaderProgram, 'aVertexColour')
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

    var camera = new Camera([1920, 1080], canvas);
    camera.position = [14093, 21834];

    var worldSize = 0;

    var eventQueue = [];
    var currentEvent = 0;

    var missionLength = 0;
    var desiredTime = 0;
    var adjustedTime = false;

    const eventMap = {
        'Object Created': new Event((uid, packetArguments) => {
            console.log(uid, packetArguments);
            gameObjects.set(
                uid,
                new GameObject(gl, packetArguments)
            );
        }, (uid, packetArguments) => {
            gameObjects.delete(uid);
        }),
        'Object Killed': new Event((uid, packetArguments) => {
            if (gameObjects.has(uid)) {
                gameObjects.get(uid).renderObject.setColour([1, 0, 0]);
            }
        }, (uid, packetArguments) => {
            if (gameObjects.has(uid)) {
                gameObjects.get(uid).renderObject.setColour([0, 0, 0]);
            }
        }),
        'Fired': new Event((uid, packetArguments, currentTime) => {
            projectiles.set(
                uid,
                new Projectile(gl, packetArguments, currentTime)
            );
        }, (uid, packetArguments) => {
            projectiles.delete(uid);
        }),
        'Marker Created': new Event((uid, packetArguments) => {
            markers.set(
                uid,
                new Marker(gl, packetArguments)
            );
        }, (uid, packetArguments) => {
            markers.delete(uid);
        })
    };

    var objectStates = new Map();

    const ws = new WebSocket("ws://localhost:8082");
    ws.addEventListener("open", () => {
        console.log("conneced to websocket!");

        let slider = document.getElementById('playbackTime');
        slider.onmouseup = function() {
            const percentage = this.value / slider.max;
            desiredTime = percentage * missionLength;
            adjustedTime = true;
        }

        ws.addEventListener('message', ({ data: incomingData }) => {
            const packet = JSON.parse(incomingData);

            switch (packet.type) {
                case 'init':
                    {
                        missionLength = packet.data.endTime;
                        worldSize = packet.data.mapSize;
                        const oReq = new XMLHttpRequest();
                        oReq.addEventListener('load', function() {
                            const parser = new DOMParser();
                            const doc = parser.parseFromString(this.responseText, 'image/svg+xml');

                            const svg = parseSVGDoc(doc.getElementsByTagName('svg')[0]);

                            for (let i = 1; i < svg.vertices.length; i += 2) {
                                svg.vertices[i] = worldSize - svg.vertices[i];
                            }
                            testObject = new RenderObject(gl, svg.vertices, svg.indices, svg.colours);

                            console.log(svg.vertices.length, svg.colours.length, svg.indices.length);
                        });
                        oReq.open("GET", '/maps/Altis5.svg');
                        oReq.send();
                    }
                    break;
                case 'event':
                    {
                        const uid = JSON.parse(packet.data.arguments[0]);
                        if (eventMap.hasOwnProperty(packet.data.type)) {
                            eventQueue.push({
                                time: packet.data.time,
                                type: packet.data.type,
                                object: uid,
                                arguments: packet.data.arguments
                            });
                        } else {
                            console.log(`Event '${packet.data.type}' not defined`);
                        }
                    }
                    break;
                case 'object_update':
                    if (!objectStates.has(packet.data.object)) {
                        objectStates.set(packet.data.object, {
                            currentState: 0,
                            states: []
                        });
                    }
                    let stateInfo = objectStates.get(packet.data.object);
                    stateInfo.states.push(packet.data.state);
                    break;
                default:
                    break;
            }
        });
    });

    var currentTime = 0;
    var then = 0;
    function render(now) {
        now *= 0.001;
        const deltaTime = now - then;
        then = now;

        if (adjustedTime) {
            if (desiredTime < currentTime) {
                // reverse time
                while (currentEvent >= 0 && desiredTime < eventQueue[currentEvent].time) {
                    const frontEvent = eventQueue[currentEvent];
                    eventMap[frontEvent.type].backward(frontEvent.object, frontEvent.arguments, currentTime);
                    currentEvent -= 1;
                }
                currentEvent = Math.max(0, currentEvent);
            } else {
                // fast forward
                while (currentEvent < eventQueue.length && desiredTime >= eventQueue[currentEvent].time) {
                    const frontEvent = eventQueue[currentEvent];
                    eventMap[frontEvent.type].forward(frontEvent.object, frontEvent.arguments, currentTime);
                    currentEvent += 1;
                }
            }

            currentTime = desiredTime;

            gameObjects.forEach(gameObject => {
                gameObject.forceState(currentTime);
            });

            projectiles.forEach(projectile => {
                projectile.forceState(currentTime);
            });

            adjustedTime = false;
        }
        
        while (currentEvent < eventQueue.length && currentTime >= eventQueue[currentEvent].time) {
            const frontEvent = eventQueue[currentEvent];
            eventMap[frontEvent.type].forward(frontEvent.object, frontEvent.arguments, currentTime);
            currentEvent += 1;
        }

        objectStates.forEach((stateData, uid) => {
            if (stateData.states.length > 0 && stateData.currentState < stateData.states.length) {
                if (gameObjects.has(uid)) {
                    gameObjects.get(uid).updateFromPacket(stateData.states[stateData.currentState]);
                    stateData.currentState += 1;
                }
            }
        });

        currentTime += deltaTime;

        projectiles.forEach(projectile => {
            if (currentTime >= projectile.endTime) {
                projectiles.delete(projectile.uid);
            } else {
                projectile.update(gl, deltaTime);
            }
        });

        gameObjects.forEach(gameObject => {
            if (gameObject.active) {
                gameObject.update(deltaTime);
            }
        });

        var objectsToRender = [];
        var projectilesToRender = [];
        projectiles.forEach(projectile => {
            projectilesToRender.push(projectile.draw());
        });

        gameObjects.forEach(gameObject => {
            if (gameObject.active) {
                objectsToRender.push(gameObject.draw());
            }
        });

        markers.forEach(marker => {
            objectsToRender.push(marker.draw());
        });

        gl.clearColor(0, 0, 0, 1);
        gl.clear(gl.COLOR_BUFFER_BIT);

        if (testObject != null) {
            drawScene(gl, camera, terrainInfo, [testObject]);
        }
        drawScene(gl, camera, objectInfo, objectsToRender);
        drawScene(gl, camera, lineInfo, projectilesToRender);

        requestAnimationFrame(render);
    };

    requestAnimationFrame(render)
}

export default main;
