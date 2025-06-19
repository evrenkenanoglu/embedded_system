const scanButton = document.getElementById("scanButton");
const ssidList = document.getElementById("ssidList");
const connectButton = document.getElementById("connectButton");
const ssidInput = document.getElementById("ssid");

const buttonEnabledColor = "#0077b6";
const buttonDisabledColor = "gray";

const ErrorColor = "#ffcccc";
const WarningColor = "#ffff99";
const NormalColor = "#f2f2f2";
const SuccessColor = "#ccffcc";

const timeout = 5000;

document.querySelector("form").addEventListener("submit", async (event) => {
  event.preventDefault();

  const ssid = ssidInput.value;
  const password = document.getElementById("password").value;
  changeBackgroundColorInputFields(NormalColor);

  if (!ssid || !password) {
    changeBackgroundColorInputFields(WarningColor);
    alert("Please fill in all fields!");
    return;
  } else {
    changeBackgroundColorInputFields(SuccessColor);
  }
  disableButtons();

  connectButton.textContent = "CONNECTING...";

  try {
    const response = await fetch("/wifiConnect", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({ ssid, password }),
      timeout,
    });

    if (response.ok) {
      const data = await response.json();
      alert(data.message);
    } else {
      alert("Error: " + response.status);
      changeBackgroundColorInputFields(ErrorColor);
    }
  } catch (error) {
    alert("Timeout: No response received.");
  }

  enableButtons();
  connectButton.textContent = "CONNECT";
});

async function scan() {
  disableButtons();
  scanButton.textContent = "SCANNING...";
  ssidInput.value = "";
  try {
    const response = await fetch("/wifiScan", { timeout });

    if (response.ok) {
      const data = await response.json();
      ssidList.innerHTML = "";
      data.forEach((item) => {
        const option = document.createElement("option");
        option.value = item.ssid;
        option.text = item.ssid;
        ssidList.appendChild(option);
      });
    } else {
      alert("Error: " + response.status);
    }
  } catch (error) {
    alert("Timeout: No response received.");
  }

  scanButton.textContent = "SCAN WIFI";
  enableButtons();
}

function disableButtons() {
  connectButton.disabled = true;
  scanButton.disabled = true;
  connectButton.style.backgroundColor = buttonDisabledColor;
  scanButton.style.backgroundColor = buttonDisabledColor;
}

function enableButtons() {
  connectButton.disabled = false;
  scanButton.disabled = false;
  connectButton.style.backgroundColor = buttonEnabledColor;
  scanButton.style.backgroundColor = buttonEnabledColor;
}

function changeBackgroundColorInputFields(color) {
  document.getElementById("ssid").style.backgroundColor = color;
  document.getElementById("password").style.backgroundColor = color;
}
