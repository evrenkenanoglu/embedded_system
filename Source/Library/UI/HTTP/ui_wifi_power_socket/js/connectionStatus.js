
// Function to update connection status UI
function updateConnectionStatus(isConnected, details) {
  const statusIndicator = document.getElementById("connectionStatus");
  const statusText = document.getElementById("connectionText");

  if (isConnected) {
    statusIndicator.style.backgroundColor = "#00ff00"; 
    statusText.textContent = details || "Connected";
    statusIndicator.style.boxShadow = "0 0 5px rgba(0, 159, 252, 0.5)";
  } else {
    statusIndicator.style.backgroundColor = "#ff0000";
    statusText.textContent = details || "Disconnected";
    statusIndicator.style.boxShadow = "0 0 5px rgba(255, 0, 0, 0.5)";
  }
}

// Add event listeners for WebSocket connection events
if (typeof websocket !== "undefined") {
  websocket.addEventListener("open", function (event) {
    updateConnectionStatus(true, "Connected to server");
  });

  websocket.addEventListener("close", function (event) {
    let reason = "";
    // WebSocket close codes: https://developer.mozilla.org/en-US/docs/Web/API/CloseEvent/code
    switch (event.code) {
      case 1000:
        reason = "Normal closure";
        break;
      case 1001:
        reason = "Server going down";
        break;
      case 1002:
        reason = "Protocol error";
        break;
      case 1003:
        reason = "Invalid data";
        break;
      case 1006:
        reason = "Connection lost";
        break;
      case 1008:
        reason = "Policy violation";
        break;
      case 1011:
        reason = "Server error";
        break;
      default:
        reason = "Connection closed";
    }
    updateConnectionStatus(false, "Disconnected: " + reason);
  });

  websocket.addEventListener("error", function (event) {
    updateConnectionStatus(false, "Connection error");
  });

  // Check initial connection state
  if (websocket.readyState === WebSocket.OPEN) {
    updateConnectionStatus(true);
  } else {
    updateConnectionStatus(false);
  }
}


