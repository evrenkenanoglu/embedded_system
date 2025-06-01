let websocket = null;

function initializeWebSocket() {
  console.log("Trying to open a WebSocket connection...");
  websocket = new WebSocket("ws://" + location.host + "/powerSwitchesWs");

  websocket.onopen = () => {
    console.log("WebSocket Connected!");
    websocket.send("Hello from browser!");
  };

  websocket.onmessage = (e) => {
    const message = JSON.parse(e.data);
    console.log("Received message:", message);
    if (message.socketId !== undefined && message.state !== undefined) {
      setButtonState(message.socketId, message.state);
    } else {
      console.warn("Invalid message format:", message);
    }
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
