document.addEventListener('DOMContentLoaded', () => {
    console.log("Device Dashboard UI Loaded");

    // Example: If you wanted to add specific JS logic beyond simple navigation
    // For instance, confirming navigation or loading content dynamically (more advanced)

    const wifiButton = document.getElementById('wifi-button');
    const appButton = document.getElementById('app-button');
    const settingsButton = document.getElementById('settings-button');

    if (wifiButton) {
        wifiButton.addEventListener('click', (event) => {
            // event.preventDefault(); // Uncomment if you want to handle navigation purely in JS
            console.log("Navigating to WiFi Setup...");
            // window.location.href = 'wifi.html'; // Uncomment if event.preventDefault() is used
        });
    }

    if (appButton) {
        appButton.addEventListener('click', (event) => {
            console.log("Navigating to Application...");
        });
    }

    if (settingsButton) {
        settingsButton.addEventListener('click', (event) => {
            console.log("Navigating to Settings...");
        });
    }
});