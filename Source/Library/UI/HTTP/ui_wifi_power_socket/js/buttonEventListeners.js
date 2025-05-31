let clickTimeout = null;
let longPressTimeout = null;

function addEventListeners(button, index) {
  button.addEventListener("click", function (event) {
    event.preventDefault();
    if (clickTimeout !== null) {
      clearTimeout(clickTimeout);
      clickTimeout = null;
      handleSocketState(button, index);
    } else {
      clickTimeout = setTimeout(() => {
        clickTimeout = null;
      }, 300);
    }
  });

  button.addEventListener("mousedown", function (event) {
    event.preventDefault();
    startLongPress(button, index);
  });

  button.addEventListener("mouseup", function (event) {
    event.preventDefault();
    cancelLongPress();
  });

  button.addEventListener("mouseleave", function (event) {
    event.preventDefault();
    cancelLongPress();
  });

  button.addEventListener("touchstart", function (event) {
    event.preventDefault();
    startLongPress(button, index);
  });

  button.addEventListener("touchend", function (event) {
    event.preventDefault();
    cancelLongPress();
  });

  button.addEventListener("touchcancel", function (event) {
    event.preventDefault();
    cancelLongPress();
  });
}

function startLongPress(button, index) {
  longPressTimeout = setTimeout(() => {
    handleSocketState(button, index);
  }, longPressDuration);
}

function cancelLongPress() {
  if (longPressTimeout !== null) {
    clearTimeout(longPressTimeout);
    longPressTimeout = null;
  }
}
