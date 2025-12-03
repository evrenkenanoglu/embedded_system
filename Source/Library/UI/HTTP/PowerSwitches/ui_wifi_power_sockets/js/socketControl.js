const socketStateMap = new Map();
const sockets = new Map();


function initSocketControl(socketCount) {
    for (let i = ButtonStartIndex; i <= socketCount; i++) {
        socketStateMap.set(i, ButtonState.DISABLED);
    }
    socketStateMap.set(ButtonIndexAll, ButtonState.DISABLED);
}

async function handleSocketState(button, index) {
    if (socketStateMap.get(index) === ButtonState.DISABLED) {
        alert("Button is disabled");
        return;
    }
    try {
        // First get the current state of the socket
        const getToggledState = !socketStateMap.get(index) ? 1 : 0;
        setButtonState(index, ButtonState.DISABLED);
        const response = await fetch("/power-switches-control", {
            method: "PUT",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify({ socketId: index, state: getToggledState }),
            timeout: responseFromHardwareTimeout,
        });
        if (!response.ok) {
            alert("Error: " + response.status);
        }
    } catch (error) {
        alert("Error: " + error.message);
    }
}

function setButtonState(index, state) {
    socketStateMap.set(index, state);

    if (index === ButtonIndexAll) {
        const toggleAllButton = document.getElementById("buttonToggleAll");
        toggleAllButton.disabled = state === ButtonState.DISABLED;
        updateButtonVisuals(toggleAllButton, state);
        sockets.forEach((buttonElement, socketId) => {
            socketStateMap.set(socketId, state);
            updateButtonVisuals(buttonElement, state);
        });
    } else {
        const buttonElement = sockets.get(index);
        // Check if the button exists
        if (buttonElement) {
            updateButtonVisuals(buttonElement, state);
        } else {
            console.error(`Button element not found for index: ${index}`);
        }
    }
}