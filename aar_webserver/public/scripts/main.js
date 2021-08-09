// main.js
// entry point for web page. Initialised WebGL layer and gets ready to render
import { Circle, Quad } from './modules/shapes.js';
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

function Projectile(gl, eventArguments, lifetime) {
    this.renderObject = new RenderObject(gl, Circle(0.3, 15));
    this.uid = JSON.parse(eventArguments[0]);
    this.position = JSON.parse(eventArguments[1]);
    this.velocity = JSON.parse(eventArguments[2]);
    this.endTime = Date.now() * 0.001 + lifetime;
    this.lifetime = lifetime;

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

    this.futurePositions = [];
    this.currentInterpolationTime = 0;
    this.interpolationBeginPosition = this.position;
    this.timeOffset = 0;

    this.update = function(deltaTime) {
        if (this.futurePositions.length <= 2) { return; }
        let desiredPosition = this.futurePositions[0].position;
        let desiredTime = this.futurePositions[0].time - this.timeOffset;

        this.currentInterpolationTime += deltaTime;
        const leftToGo = this.currentInterpolationTime / desiredTime;

        // lerp between known states
        this.position[0] = this.interpolationBeginPosition[0] + leftToGo * (desiredPosition[0] - this.interpolationBeginPosition[0]);
        this.position[1] = this.interpolationBeginPosition[1] + leftToGo * (desiredPosition[1] - this.interpolationBeginPosition[1]);

        if (leftToGo >= 1) {
            this.interpolationBeginPosition = desiredPosition;
            this.timeOffset += desiredTime;
            this.currentInterpolationTime = 0;
            this.futurePositions.shift();
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

function main() {
    var testObject = null;

    const oReq = new XMLHttpRequest();
    oReq.addEventListener('load', function() {
        const parser = new DOMParser();
        const doc = parser.parseFromString(this.responseText, 'image/svg+xml');

        const svg = parseSVGDoc(doc.getElementsByTagName('svg')[0]);

        testObject = new RenderObject(gl, svg.vertices, svg.indices, svg.colours);

        console.log(svg.vertices.length, svg.colours.length, svg.indices.length);
        console.log(svg.vertices);
        console.log(svg.indices);
    });
    oReq.open("GET", '/maps/Altis5.svg');
    oReq.send();

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

    const programInfo = {
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

    var projectiles = new Map();
    var gameObjects = new Map();
    var markers = new Map();

    var camera = new Camera([1920, 1080], canvas);
    camera.position = [10029, 5597];

    var worldSize = 0;

    const ws = new WebSocket("ws://localhost:8082");
    ws.addEventListener("open", () => {
        console.log("conneced to websocket!");
        return;

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
                                console.log(packet.data);
                                break;
                            case "Marker Updated":
                                break;
                            case "Fired":
                                projectiles.set(
                                    uid,
                                    new Projectile(gl, packet.data.arguments, packet.data.metaInfo.lifetime)
                                );
                                break;
                            case "Object Killed":
                                if (gameObjects.has(uid)) {
                                    gameObjects.get(uid).renderObject.setColour([1, 0, 0]);
                                }
                                break;
                            default:
                                console.log(packet.data);
                                break;
                        };
                    }
                    break;
                case 'object_update':
                    gameObjects.get(packet.data.object).updateFromPacket(packet.data.state);
                    break;
                default:
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
            if (now >= projectile.endTime) {
                projectiles.delete(projectile.uid);
                console.log(Date.now(), projectile.endTime, projectile.lifetime);
            } else {
                projectile.update(deltaTime);
                objectsToRender.push(projectile.draw());
            }
        });

        gameObjects.forEach(gameObject => {
            gameObject.update(deltaTime);
            objectsToRender.push(gameObject.draw());
        });

        markers.forEach(marker => {
            objectsToRender.push(marker.draw());
        });

        if (testObject != null) {
            objectsToRender.push(testObject);
        }
        
        drawScene(gl, camera, programInfo, objectsToRender);

        requestAnimationFrame(render);
    };

    requestAnimationFrame(render)
}

export default main;
