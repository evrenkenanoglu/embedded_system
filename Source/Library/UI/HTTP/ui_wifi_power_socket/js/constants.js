const ButtonState = {
  OFF: 0,
  ON: 1,
  DISABLED: 2,
};

const ButtonIndexAll = 0xff;

const colorStateOff = "#FD0101";
const colorStateOn = "#00ff00";
const colorStateDisabled = "#808080";

const longPressDuration = 500;
const responseFromHardwareTimeout = 500;

const websocketUri = "/powerSwitchesWs";

const websocketUrl =
  location.protocol === "https:"
    ? "wss://" + location.host + websocketUri
    : "ws://" + location.host + websocketUri;

const websocketReconnectInterval = 5000;
const websocketMaxRetries = 5;
const websocketRetryDelay = 1000;
