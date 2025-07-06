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
    if (message.socketId === undefined || message.state === undefined) {
      console.warn("Invalid message format:", message);
    }

    // check if the message state is an array
    if (message.socketId === ButtonIndexAll) {
      if (Array.isArray(message.state)) {
        // If state is an array, update all buttons
        message.state.forEach((state, index) => {
          setButtonState(index, state);
        });
        return;
      }
    }

    setButtonState(message.socketId, message.state);
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
