
// Function to Socket State Mapping
registerDomContentLoadedFunc(() => {
    initSocketControl();
});

// Function to generate buttons and add them to the DOM
registerDomContentLoadedFunc(() => { 
    generateUiElements();
});

// Register the WebSocket initialization function
registerDomContentLoadedFunc(() => {
    initializeWebSocket();
});

registerDomContentLoadedFunc(() => {
    initConnectionStatus();
});

document.addEventListener("DOMContentLoaded", initializeDomContent);
// Register the function to be called when the DOM is fully loaded
