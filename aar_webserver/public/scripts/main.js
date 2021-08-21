// main.js
// entry point for web page. Initialised WebGL layer and gets ready to render
import { Circle, Quad, Line } from './modules/shapes.js';
import { parseSVGDoc } from './modules/svg.js';
import { RenderObject, Texture, Camera, drawScene, initShaderProgram } from './modules/rendering.js';

const textureVsSource = `
    attribute vec2 aVertexPosition;
    attribute vec3 aVertexColour;
    attribute vec2 aTextureCoord;

    uniform mat4 uModelMatrix;
    uniform mat4 uViewMatrix;
    uniform mat4 uProjectionMatrix;

    varying lowp vec4 vColour;
    varying highp vec2 vTextureCoord;
    
    void main() {
        mat4 mvp = uProjectionMatrix * uViewMatrix * uModelMatrix;
        gl_Position = mvp * vec4(aVertexPosition, 0, 1); 
        vColour = vec4(aVertexColour, 1);
        vTextureCoord = aTextureCoord;
    }
`;
const textureFragSource = `
    varying lowp vec4 vColour;
    varying highp vec2 vTextureCoord;

    uniform sampler2D uSampler;

    void main() {
        gl_FragColor = vColour * texture2D(uSampler, vTextureCoord);
    }
`;

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

    this.uid = eventArguments[0];
    this.origin = eventArguments[1];
    this.startTime = currentTime;
    this.position = [this.origin[0], this.origin[1]];
    this.velocity = eventArguments[2];
    this.lifetime = eventArguments[4];
    this.endTime = currentTime + this.lifetime;

    this.forceState = function(time) {
        this.position[0] = this.origin[0] + this.velocity[0] * (time - this.startTime);
        this.position[1] = this.origin[1] + this.velocity[1] * (time - this.startTime);
    }

    this.update = function(gl, deltaTime) {
        if (deltaTime == 0) { return; }

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

function GameObject(gl, eventArguments, texture = null) {
    this.renderObject = new RenderObject(gl, Quad([1, 1]));

    if (texture) {
        this.renderObject.setTexture(texture);
    }

    this.sideColour = [0, 0, 0];
    this.className = eventArguments[1];
    if (this.className.includes('potato_w')) {
        this.sideColour = [0, 0, 1];
    } else if (this.className.includes('potato_e')) {
        this.sideColour = [1, 0, 0];
    } else if (this.className.includes('potato_i')) {
        this.sideColour = [0, 1, 0];
    }
    this.renderObject.setColour(this.sideColour);

    this.position = eventArguments[2];
    this.name = eventArguments[3];

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

        if (this.currentState >= this.futurePositions.length) { return; }

        this.position = this.futurePositions[this.currentState].position;
        this.interpolationBeginPosition = this.position;
        this.timeOffset = desiredTime;
        this.currentInterpolationTime = 0;
        this.firstUpdate = true;
    }

    this.update = function(deltaTime) {
        if (this.currentState >= this.futurePositions.length || deltaTime == 0) { return; }

        this.currentInterpolationTime += deltaTime;
        if (this.firstUpdate) {
            this.position = this.futurePositions[this.currentState].position;
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
    this.position = eventArguments[6];

    this.draw = function() {
        this.renderObject.position = this.position;
        return this.renderObject;
    };
}

function Event(onEvent, onUndo) {
    this.forward = onEvent;
    this.backward = onUndo;
}

function formatTime(timeInSeconds) {
    let minutes = Math.floor(timeInSeconds / 60);
    let seconds = Math.floor(timeInSeconds - minutes * 60);

    let major = (new Array(3).join('0')+minutes).slice(-2);
    let minor = (new Array(3).join('0')+seconds).slice(-2);

    return `${major}:${minor}`;
}

function toggleMissionTray(force = null) {
    const tray = document.getElementById('missionTableContainer');
    if (typeof force === 'boolean' && force) {
        if (force) {
            tray.classList.add('slide');
        } else {
            tray.classList.remove('slide');
        }
    } else {
        tray.classList.toggle('slide');
    }
}

function main() {
    const canvas = document.querySelector("#glCanvas");
    const gl = canvas.getContext("webgl", { alpha: false });

    if (gl == null) {
        alert("Unable to initialise WebGL. Your browser or machine may not support it.");
        return;
    }

    var ext = gl.getExtension('OES_element_index_uint');
    if (ext == null) {
        alert("Unable to use extension");
        return;
    }

    gl.enable(gl.BLEND);
    gl.blendFunc(gl.SRC_COLOR, gl.DST_COLOR);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

    const textureShaderProgram = initShaderProgram(gl, textureVsSource, textureFragSource);
    const shaderProgram = initShaderProgram(gl, vsSource, fragSource);

    const objectInfo = {
        program: textureShaderProgram,
        primitiveType: gl.TRIANGLE_STRIP,
        useIndexBuffer: false,
        useTextureMapping: true,
        attribLocations: {
            vertexPosition: gl.getAttribLocation(textureShaderProgram, 'aVertexPosition'),
            vertexColour: gl.getAttribLocation(textureShaderProgram, 'aVertexColour'),
            textureCoord: gl.getAttribLocation(textureShaderProgram, 'aTextureCoord')
        },
        uniformLocations: {
            projectionMatrix: gl.getUniformLocation(textureShaderProgram, 'uProjectionMatrix'),
            viewMatrix: gl.getUniformLocation(textureShaderProgram, 'uViewMatrix'),
            modelMatrix: gl.getUniformLocation(textureShaderProgram, 'uModelMatrix'),
            textureSampler: gl.getUniformLocation(textureShaderProgram, 'uSampler')
        }
    }

    const terrainInfo = {
        program: shaderProgram,
        primitiveType: gl.TRIANGLES,
        useIndexBuffer: true,
        useTextureMapping: false,
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
        useTextureMapping: false,
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
    
    var textures = new Map();
    textures.set('none', new Texture(gl, ''));
    textures.set('death_icon', new Texture(gl, '/icons/death_skull.png'));

    var testObject = null;
    var projectiles = new Map();
    var gameObjects = new Map();
    var markers = new Map();

    var camera = new Camera([1920, 1080], canvas);
    camera.position = [0, 0];
    var setCameraPosition = false;

    var worldSize = 0;

    var eventQueue = [];
    var currentEvent = 0;

    var missionLength = 0;
    var desiredTime = 0;
    var adjustedTime = false;
    var paused = false;
    document.getElementById('playbackButton').classList.toggle('paused', !paused);

    var objectStates = new Map();
    var settingTime = false;
    var latestUpdateTimeRecieved = 0;

    var currentTime = 0;

    document.getElementById('trayButton').onclick = toggleMissionTray;
    toggleMissionTray(true);

    function init() {
        testObject = null;
        projectiles.clear();
        gameObjects.clear();
        markers.clear();
        
        worldSize = 0;
        eventQueue = [];
        currentEvent = 0;

        missionLength = 0;
        desiredTime = 0;
        adjustedTime = false;
        paused = false;

        document.getElementById('playbackButton').classList.toggle('paused', !paused);

        objectStates.clear();
        settingTime = false;
        latestUpdateTimeRecieved = 0;

        currentTime = 0;

        camera.position = [0, 0];
        setCameraPosition = false;

        console.log('init');
    }

    const eventMap = {
        'Object Created': new Event((uid, packetArguments) => {
            const classname = packetArguments[1].replaceAll('\"', '');
            if (classname === 'potato_spectate_spectator' || classname === 'potato_spectate_playableSpectator' || classname === 'potato_spectate_holder') { return; } // temp fix for bad data
            console.log(uid, classname, packetArguments);
            gameObjects.set(
                uid,
                new GameObject(gl, packetArguments, new Texture(gl, ''))
            );
        }, (uid, packetArguments) => {
            gameObjects.delete(uid);
        }),
        'Object Killed': new Event((uid, packetArguments) => {
            if (gameObjects.has(uid)) {
                gameObjects.get(uid).renderObject.setColour([1, 1, 1]);
                gameObjects.get(uid).renderObject.setTexture(textures.get('death_icon'));
            }
        }, (uid, packetArguments) => {
            if (gameObjects.has(uid)) {
                gameObjects.get(uid).renderObject.setColour(gameObjects.get(uid).sideColour);
                gameObjects.get(uid).renderObject.setTexture(textures.get('none'));
            }
        }),
        'Fired': new Event((uid, packetArguments, currentTime) => {
            projectiles.set(
                uid,
                new Projectile(gl, packetArguments, currentTime)
            );
        }, (uid, packetArguments) => {
            projectiles.delete(uid);
        })/*,
        'Marker Created': new Event((uid, packetArguments) => {
            console.log('marker created', packetArguments);
            markers.set(
                uid,
                new Marker(gl, packetArguments)
            );
        }, (uid, packetArguments) => {
            markers.delete(uid);
        })*/
    };

    const ws = new WebSocket("ws://localhost:8082");
    ws.addEventListener("open", () => {
        console.log("conneced to websocket!");

        let slider = document.getElementById('playbackTime');
        slider.onmousedown = function() {
            settingTime = true;
        }
        slider.onmouseup = function() {
            const percentage = this.value / this.max;
            desiredTime = percentage * missionLength;
            adjustedTime = true;
            settingTime = false;
        }

        let playPause = document.getElementById('playbackButton');
        playPause.onclick = function() {
            playPause.classList.toggle('paused');
            paused = !paused;
        }

        {
            const missionTable = document.getElementById('missionTable');
            const elements = missionTable.getElementsByClassName('missionEntry');
            for (let i = 0; i < elements.length; i++ ){
                const entry = elements[i];
                const entryObject = {
                    type: 'missionRequest',
                    data: {
                        name: entry.children[0].innerHTML,
                        map: entry.children[1].innerHTML,
                        playerCount: entry.children[2].innerHTML,
                        date: entry.children[3].innerHTML,
                        realDate: entry.children[5].innerHTML,
                        dbKey: entry.children[6].innerHTML
                    }
                };
    
                entry.getElementsByTagName('button')[0].onclick = function() {
                    ws.send(JSON.stringify(entryObject));
                }
            }
        }

        ws.addEventListener('message', ({ data: incomingData }) => {
            const packet = JSON.parse(incomingData);

            switch (packet.type) {
                case 'init':
                    {
                        init();
                        toggleMissionTray();

                        missionLength = packet.data.endTime;
                        document.getElementById('totalTime').innerText = formatTime(missionLength);
                        
                        const map = JSON.parse(packet.data.map);
                        console.log(map);
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
                        });
                        oReq.open("GET", `/maps/${map}.svg`);
                        oReq.send();
                    }
                    break;
                case 'missions':
                    {
                        missions = packet.data;

                        document.createElement('missionTable');
                    }
                    break;
                case 'event':
                    {
                        if (!packet.hasOwnProperty('data')) {
                            console.log(`Unknown event packet: ${packet}`);
                            break;
                        }
                        if (packet.data.type != 'Mission End'){
                            const uid = JSON.parse(packet.data.arguments[0]);
                            if (eventMap.hasOwnProperty(packet.data.type)) {
                                var event = {
                                    time: packet.data.time,
                                    type: packet.data.type,
                                    object: uid,
                                    arguments: []
                                };

                                packet.data.arguments.forEach(argument => {
                                    event.arguments.push(JSON.parse(argument))
                                });

                                eventQueue.push(event);
                                latestUpdateTimeRecieved = Math.max(latestUpdateTimeRecieved, event.time);
                            } else {
                                console.log(`Event '${packet.data.type}' not defined`);
                            }
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

                    if (stateInfo.states.length >= 1) {
                        const lastState = stateInfo.states[stateInfo.states.length - 1];

                        const dx = lastState.position[0] - packet.data.state.position[0];
                        const dy = lastState.position[1] - packet.data.state.position[1];

                        const object = gameObjects.get(packet.data.object);
                        if (!setCameraPosition && object != null && object.className != 'HeadlessClient_F') {
                            setCameraPosition = true;
                            camera.position = [
                                packet.data.state.position[0],
                                packet.data.state.position[1]
                            ];
                        }

                        const distance = Math.sqrt(dx * dx + dy * dy);
                        if (distance <= 1) {
                            break;
                        }
                    }
                    stateInfo.states.push(packet.data.state);
                    break;
                default:
                    break;
            }
        });
    });

    var then = 0;
    const maxDelta = 5/60;
    function render(now) {
        var width = gl.canvas.clientWidth;
        var height = gl.canvas.clientHeight;

        if (gl.canvas.width != width || gl.canvas.has != height) {
            gl.canvas.width = width;
            gl.canvas.height = height;

            gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);
        }

        now *= 0.001;
        let deltaTime = now - then;
        then = now;

        if (deltaTime >= maxDelta) {
            deltaTime = maxDelta; // don't interpolate a lot if we are unresponsive
        }

        if (!settingTime) {
            let slider = document.getElementById('playbackTime');
            slider.value = slider.max * currentTime / missionLength;
        } else {
            let slider = document.getElementById('playbackTime');
            slider.value = Math.min(slider.value, slider.max * latestUpdateTimeRecieved / missionLength);
        }

        if (adjustedTime) {
            if (desiredTime < currentTime) {
                // reverse time
                currentEvent = Math.min(currentEvent, eventQueue.length - 1);
                while (currentEvent >= 0 && desiredTime < eventQueue[currentEvent].time) {
                    const frontEvent = eventQueue[currentEvent];
                    eventMap[frontEvent.type].backward(frontEvent.object, frontEvent.arguments, currentTime);
                    currentEvent -= 1;
                }
                currentEvent = Math.max(0, currentEvent);
            } else {
                // fast forward
                desiredTime = Math.min(desiredTime, latestUpdateTimeRecieved);
                
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

        objectStates.forEach((stateData, uid) => {
            if (stateData.states.length > 0 && stateData.currentState < stateData.states.length) {
                latestUpdateTimeRecieved = Math.max(latestUpdateTimeRecieved, stateData.states[stateData.currentState].time);
                if (gameObjects.has(uid)) {
                    gameObjects.get(uid).updateFromPacket(stateData.states[stateData.currentState]);
                    stateData.currentState += 1;
                }
            }
        });

        if (!paused) {
            while (currentEvent < eventQueue.length && currentTime >= eventQueue[currentEvent].time) {
                const frontEvent = eventQueue[currentEvent];
                eventMap[frontEvent.type].forward(frontEvent.object, frontEvent.arguments, currentTime);
                currentEvent += 1;
            }
            
            currentTime += deltaTime;
            document.getElementById('currentTime').innerText = formatTime(currentTime);
            if (settingTime) {
                const slider = document.getElementById('playbackTime');
                document.getElementById('currentTime').innerText = formatTime(missionLength * slider.value / slider.max);
            }

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
        }

        if (missionLength != 0) {
            let slider = document.getElementById('playbackTime');
            const validAmount = slider.clientWidth * latestUpdateTimeRecieved / missionLength;
            document.getElementById('playbackTime').style.boxShadow = `${validAmount}px 0 0 0 var(--bw-low-contrast) inset`;
        }

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
