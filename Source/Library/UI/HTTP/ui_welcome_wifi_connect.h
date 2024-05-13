#ifndef UI_WELCOME_WIFI_CONNECT_H
#define UI_WELCOME_WIFI_CONNECT_H

#define HTML_UI_WELCOME_WIFI_CONNECT_CONTENT "<!DOCTYPE HTML>\
<html>\
\
<head>\
  <title>ESP32 HTTP Server Interface</title>\
  <meta charset=\"UTF-8\">\
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
  <link href=\"https://fonts.googleapis.com/css?family=Open+Sans\" rel=\"stylesheet\">\
  <style>\
    body {\
      font-family: 'Open Sans', sans-serif;\
      background-color: #fafafa;\
      color: #333;\
      margin: 0;\
      padding: 0;\
    }\
\
    .center {\
      display: flex;\
      justify-content: center;\
      align-items: center;\
      height: 100vh;\
    }\
\
    .form {\
      background-color: #fff;\
      border-radius: 10px;\
      box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);\
      padding: 20px;\
      width: 100%;\
      max-width: 500px;\
    }\
\
    .form__title {\
      font-size: 28px;\
      font-weight: 700;\
      margin-bottom: 20px;\
      text-align: center;\
      text-transform: uppercase;\
      letter-spacing: 1px;\
      background: linear-gradient(90deg, #0077b6, #00a8ff);\
      -webkit-background-clip: text;\
      -webkit-text-fill-color: transparent;\
      user-select: none;\
    }\
\
    .form__input {\
      border: none;\
      border-radius: 10px;\
      box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);\
      font-size: 16px;\
      margin-bottom: 20px;\
      padding: 10px;\
      width: 100%;\
      background-color: #f2f2f2;\
      color: #333;\
      box-sizing: border-box;\
      transition: all 0.3s ease;\
    }\
\
    .form__input:focus {\
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.2);\
      /* Add shadow when input is focused */\
      background-color: #e6e6e6;\
      /* Change background color when input is focused */\
    }\
\
    .form__buttons {\
      justify-content: space-between;\
    }\
\
    .form__button {\
      background-color: #0077b6;\
      border: none;\
      border-radius: 10px;\
      color: #fff;\
      cursor: pointer;\
      font-size: 16px;\
      padding: 10px 10px;\
      transition: background-color 0.3s;\
      width: 100%;\
      margin-bottom: 20px;\
      margin-right: 0;\
      box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);\
      height: 50px;\
    }\
\
    .form__button:hover {\
      background: linear-gradient(90deg, #005999, #0077b6);\
      text-shadow: 0px 0px 5px #fff;\
    }\
\
    .form__buttonText {\
      background-color: #0077b6;\
      color: #fff;\
      font-weight: bold;\
    }\
\
    .form__group label {\
      color: #525557;\
      /* Change label text color */\
      font-weight: bold;\
      /* Make label text bold */\
      ;\
    }\
  </style>\
</head>\
\
<body>\
  <div class=\"center\">\
    <div class=\"form\">\
      <div class=\"form__title\">UNIVERSE HOME WELCOME WIFI CONNECT</div>\
      <form method=\"POST\">\
        <div class=\"form__group\">\
          <label for=\"ssid\">SSID</label>\
          <input type=\"text\" id=\"ssid\" name=\"ssid\" list=\"ssidList\" class=\"form__input\">\
          <datalist id=\"ssidList\">\
            <!-- Options will be populated by the scan function --></datalist>\
        </div>\
        <div class=\"form__group\">\
          <label for=\"password\">PASSWORD</label>\
          <input type=\"password\" id=\"password\" name=\"password\" class=\"form__input\">\
        </div>\
        <div class=\"form__buttons\">\
          <button type=\"submit\" class=\"form__button\" id=\"connectButton\">\
            CONNECT\
          </button>\
          <button type=\"button\" class=\"form__button\" id=\"scanButton\" onclick=\"scan()\">SCAN WIFI</button>\
        </div>\
      </form>\
    </div>\
  </div>\
  <script>\
\
    const scanButton = document.getElementById('scanButton');\
    const ssidList = document.getElementById('ssidList');\
    const connectButton = document.getElementById('connectButton');\
    const ssidInput = document.getElementById('ssid');\
    \
    const buttonEnabledColor = '#0077b6';\
    const buttonDisabledColor = 'gray';\
    \
    const timeout = 5000;\
    \
    document.querySelector('form').addEventListener('submit', async (event) => {\
      event.preventDefault();\
      \
      const ssid = ssidInput.value;\
      const password = document.getElementById('password').value;\
\
      disableButtons();\
\
      connectButton.textContent = 'CONNECTING...';\
\
      try {\
        const response = await fetch('/connect', {\
          method: 'POST',\
          headers: {\
            'Content-Type': 'application/json'\
          },\
          body: JSON.stringify({ ssid, password }),\
          timeout\
        });\
\
        if (response.ok) {\
          const data = await response.json();\
          alert(data.message);\
        } else {\
          alert('Error: ' + response.status);\
        }\
      } catch (error) {\
        alert('Timeout: No response received.');\
      }\
\
      enableButtons();\
      connectButton.textContent = 'CONNECT';\
    });\
\
    async function scan() {\
      disableButtons();\
      scanButton.textContent = 'SCANNING...';\
      ssidInput.value = '';\
      try {\
        const response = await fetch('/scan', { timeout });\
\
        if (response.ok) {\
          const data = await response.json();\
          ssidList.innerHTML = '';\
          data.forEach((item) => {\
            const option = document.createElement('option');\
            option.value = item.ssid;\
            option.text = item.ssid;\
            ssidList.appendChild(option);\
          });\
        } else {\
          alert('Error: ' + response.status);\
        }\
      } catch (error) {\
        alert('Timeout: No response received.');\
      }\
\
      scanButton.textContent = 'SCAN WIFI';\
      enableButtons();\
    }\
\
    function disableButtons() {\
      connectButton.disabled = true;\
      scanButton.disabled = true;\
      connectButton.style.backgroundColor = buttonDisabledColor;\
      scanButton.style.backgroundColor = buttonDisabledColor;\
    }\
\
    function enableButtons() {\
      connectButton.disabled = false;\
      scanButton.disabled = false;\
      connectButton.style.backgroundColor = buttonEnabledColor;\
      scanButton.style.backgroundColor = buttonEnabledColor;\
    }\
  </script>\
</body>\
\
</html>"

#endif