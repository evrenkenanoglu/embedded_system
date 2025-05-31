function generateButtons(buttons) {
  // Clear existing content first to refresh all buttons
  document.getElementById("controls").innerHTML = "";

  for (let i = 0; i < 10; i++) {
    const container = document.createElement("div");
    container.className = "socket-container";

    const button = document.createElement("button");
    button.className = "btn button-container off";

    const buttonStateCircle = document.createElement("div");
    buttonStateCircle.className = "button-state-circle";

    const buttonOuterCircle = document.createElement("div");
    buttonOuterCircle.className = "button-outer-circle";

    const buttonInnerCircle = document.createElement("div");
    buttonInnerCircle.className = "button-inner-circle";

    const icon = document.createElement("div");
    icon.className = "icon";

    const iconPower = document.createElement("i");
    iconPower.className = "fas fa-power-off";

    icon.append(iconPower);
    buttonInnerCircle.append(icon);
    button.append(buttonStateCircle);
    button.append(buttonOuterCircle);
    button.append(buttonInnerCircle);

    addEventListeners(button, i);

    sockets.push(button);

    container.append(button);
    document.getElementById("controls").append(container);
  }

  generateButtonToggleAll();

  // Update the button visuals based on the initial state
  function updateInitialButtonStates() {
    sockets.forEach((button, index) => {
      const state = socketStateMap.get(index);
      updateButtonVisuals(button, state);
    });
  }

  updateInitialButtonStates();
}

function updateButtonVisuals(button, state) {
  const stateCircle = button.querySelector(".button-state-circle");
  const outerCircle = button.querySelector(".button-outer-circle");
  const powerIcon = button.querySelector(".fa-power-off");

  switch (state) {
    case ButtonState.DISABLED:
      button.classList.add("disabled");
      button.classList.remove("on", "off");
      stateCircle.style.borderColor = colorStateDisabled;
      powerIcon.style.color = colorStateDisabled;
      outerCircle.style.animation = "none";
      button.style.opacity = 0.5;
      break;
    case ButtonState.ON:
      button.classList.remove("disabled", "off");
      button.classList.add("on");
      button.style.opacity = 1;
      stateCircle.style.borderColor = colorStateOn;
      powerIcon.style.color = colorStateOn;
      outerCircle.style.animation = "glowGreen 1.5s infinite alternate";
      break;
    case ButtonState.OFF:
    default:
      button.classList.remove("disabled", "on");
      button.classList.add("off");
      button.style.opacity = 1;
      stateCircle.style.borderColor = colorStateOff;
      powerIcon.style.color = colorStateOff;
      outerCircle.style.animation = "glowRed 1.5s infinite alternate";
      break;
  }
}

function generateButtonToggleAll() {
  const mainControlsDiv = document.getElementById("mainControls");
  // Clear any existing content
  mainControlsDiv.innerHTML = "";

  // Create container for the main button
  const socketContainer = document.createElement("div");
  socketContainer.className = "socket-container";
  socketContainer.style.margin = "0 auto 20px auto";
  // Create the main toggle button
  const buttonToggleAll = document.createElement("button");
  buttonToggleAll.id = "buttonToggleAll";
  buttonToggleAll.className = "btn button-container off";
  // Create button elements
  const stateCircle = document.createElement("div");
  stateCircle.className = "button-state-circle";

  const outerCircle = document.createElement("div");
  outerCircle.className = "button-outer-circle";

  const innerCircle = document.createElement("div");
  innerCircle.className = "button-inner-circle";

  const iconDiv = document.createElement("div");
  iconDiv.className = "icon";

  const icon = document.createElement("i");
  icon.className = "fas fa-power-off";

  // Build the button hierarchy
  iconDiv.appendChild(icon);
  innerCircle.appendChild(iconDiv);
  buttonToggleAll.appendChild(stateCircle);
  buttonToggleAll.appendChild(outerCircle);
  buttonToggleAll.appendChild(innerCircle);
  socketContainer.appendChild(buttonToggleAll);
  // Add to the DOM
  mainControlsDiv.appendChild(socketContainer);
  // Add event listener for the toggle all button
  addEventListeners(buttonToggleAll, ButtonIndexAll);
}
