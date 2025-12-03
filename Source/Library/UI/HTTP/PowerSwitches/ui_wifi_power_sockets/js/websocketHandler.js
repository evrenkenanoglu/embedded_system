let websocket = null;

function initializeWebSocket() {
    console.log("Trying to open a WebSocket connection...");
    websocket = new WebSocket(websocketUrl);

    websocket.onopen = () => {
        console.log("WebSocket Connected!");
        websocket.send("Hello from browser!");
    };

    websocket.onmessage = (e) => {
        const message = JSON.parse(e.data);
        console.log("Received message:", message);
        wsMessageHandler(message);
    };

    websocket.onclose = () => {
        console.log("WebSocket disconnected");
    };

    websocket.onerror = (error) => {
        console.error("WebSocket error:", error);
    };
}

// Function to expose the websocket object
function getWebSocket() {
    return websocket;
}

function wsMessageHandler(message) {
    if (!message.type) {
        console.warn("Received message without type:", message);
        return;
    }

    switch (message.type) {
        case "UI_UPDATE":
            if (validateMessageFields(message, ["socketCount", "initialStates"]) && Array.isArray(message.initialStates)) {
                generateButtons(message.socketCount);
                initSocketControl(message.socketCount);

                // Set initial states
                for (let i = 0; i < message.socketCount; i++) {
                    setButtonState(ButtonStartIndex + i, message.initialStates[i]);
                }

            } else {
                console.warn(
                    "Received UI_UPDATE message with missing fields:",
                    message
                );
            }
            break;
        case "SWITCH_STATE_UPDATE":
            if (validateMessageFields(message, ["socketId", "state"])) {
                if (message.socketId === ButtonIndexAll) {
                    if (Array.isArray(message.state)) {
                        message.state.forEach((state, index) => {
                            setButtonState(index, state);
                        });
                    }
                } else {
                    setButtonState(message.socketId, message.state);
                }
            } else {
                console.warn(
                    "Received SWITCH_STATE_UPDATE message with missing fields:",
                    message
                );
            }
            break;
        default:
            console.warn("Invalid message type:", message.type);
    }
}

function validateMessageFields(message, requiredFields) {
    for (const field of requiredFields) {
        if (message[field] === undefined) {
            console.warn(
                `Received message missing required field: ${field}`,
                message
            );
            return false;
        }
    }
    return true;
}