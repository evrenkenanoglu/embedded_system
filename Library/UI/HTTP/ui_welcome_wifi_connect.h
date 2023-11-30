#ifndef UI_WELCOME_WIFI_CONNECT_H
#define UI_WELCOME_WIFI_CONNECT_H

#define HTML_UI_WELCOME_WIFI_CONNECT_CONTENT "<!DOCTYPE HTML>\
<html>\
  <head>\
    <title>ESP32 HTTP Server Interface</title>\
    <meta charset=\"UTF-8\">\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
    <link href=\"https://fonts.googleapis.com/css?family=Open+Sans\" rel=\"stylesheet\">\
    <style>\
      body {\
        font-family: 'Open Sans', sans-serif;\
        background-color: #f2f2f2;\
        color: #333;\
        margin: 0;\
      }\
      .center {\
        display: flex; \
        justify-content: center;\
        align-items: center;\
        height: 100vh;\
      }\
      .form {\
        background-color: #fff;\
        border-radius: 5px;\
        box-shadow: 0 2px 5px rgba(0, 0, 0, 0.3);\
        padding: 20px;\
        width: 100%;\
        max-width: 500px;\
      }\
      .form__title {\
        font-size: 24px;\
        font-weight: bold;\
        margin-bottom: 20px;\
        text-align: center;\
      }\
      .form__input {\
        border: none;\
        border-radius: 5px;\
        box-shadow: 0 1px 3px rgba(0, 0, 0, 0.3);\
        font-size: 16px;\
        margin-bottom: 10px;\
        padding: 10px;\
        width: 100%;\
        background-color: #f2f2f2;\
        color: #333;\
      }\
      .form__button {\
        background-color: #00a8ff;\
        border: none;\
        border-radius: 5px;\
        color: #fff;\
        cursor: pointer;\
        font-size: 16px;\
        padding: 10px;\
        transition: background-color 0.3s;\
        width: 100%;\
        margin-bottom: 10px;\
        box-shadow: 0 2px 5px rgba(0, 0, 0, 0.3);\
      }\
      .form__button:hover {\
        background-color: #0077b6;\
      }\
      .form__output {\
        border: none;\
        border-radius: 5px;\
        box-shadow: 0 1px 3px rgba(0, 0, 0, 0.3);\
        font-size: 16px;\
        margin-top: 20px;\
        padding: 10px;\
        resize: none;\
        width: 100%;\
        height: 200px;\
        background-color: #f2f2f2;\
        color: #333;\
      }\
      @media screen and (min-width: 600px) {\
        .form__buttons {\
          display: flex;\
        }\
        .form__button {\
          width: 45%;\
          margin-right: 10%;\
        }\
        .form__button:last-child {\
          margin-right: 0;\
        }\
      }\
    </style>\
  </head>\
  <body>\
    <div class=\"center\">\
      <div class=\"form\">\
        <div class=\"form__title\">ESP32 WELCOME WIFI CONNECT</div>\
        <form method=\"POST\">\
          <div class=\"form__group\">\
            <label for=\"ssid\">SSID</label>\
            <input type=\"text\" id=\"ssid\" name=\"ssid\" class=\"form__input\">\
          </div>\
          <div class=\"form__group\">\
            <label for=\"password\">Password</label>\
            <input type=\"password\" id=\"password\" name=\"password\" class=\"form__input\">\
          </div>\
          <div class=\"form__buttons\">\
            <button type=\"submit\" class=\"form__button\">Connect</button>\
            <button type=\"button\" class=\"form__button\" onclick=\"scan()\">Scan WiFi</button>\
          </div>\
        </form>\
        <label for=\"output\">Output</label>\
        <textarea id=\"output\" name=\"output\" readonly class=\"form__output\"></textarea>\
      </div>\
    </div>\
    <script>\
      function scan() {\
        const output = document.getElementById('output');\
        output.value = 'Scanning...';\
        fetch('/scan')\
        .then(response => response.json())\
        .then(data => {\
          output.value = data.join('\\n');\
        })\
        .catch(error => {\
          output.value = 'Could not scan';\
          console.error(error);\
        });\
      }\
      const form = document.querySelector('form');\
      form.addEventListener('submit', (event) => {\
        event.preventDefault();\
        const btn = document.querySelector('.form__button');\
        const output = document.getElementById('output');\
        const ssid = document.getElementById('ssid').value;\
        const password = document.getElementById('password').value;\
        btn.textContent = 'Connecting...';\
        btn.disabled = true;\
        fetch('/connect', {\
          method: 'post',\
          headers: {\
            ''\
          },\
          body: `ssid=${ssid}&password=${password}`\
        })\
        .then(response => response.text())\
        .then(data => {\
          if (data === 'Connected') {\
            output.value = 'Connected';\
          } else {\
            output.value = 'Could not connect';\
          }\
          btn.textContent = 'Connect';\
          btn.disabled = false;\
        })\
        .catch(error => {\
          output.value = 'Could not connect';\
          btn.textContent = 'Connect';\
          btn.disabled = false;\
          console.error(error);\
        });\
        setTimeout(() => {\
          if (btn.textContent === 'Connecting...') {\
            btn.textContent = 'Connect';\
            btn.disabled = false;\
          }\
        }, 1000);\
      });\
    </script>\
  </body>\
</html>\
"

#endif