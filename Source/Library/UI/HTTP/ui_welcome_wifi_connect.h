#ifndef UI_WELCOME_WIFI_CONNECT_H
#define UI_WELCOME_WIFI_CONNECT_H

#define HTML_UI_WELCOME_WIFI_CONNECT_CONTENT "<!DOCTYPE HTML>\
<html>\
  <head>\
    <title>ESP32 HTTP Server Interface</title>\
    <meta charset=\"UTF-8\">\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
    <link href=\"https://fonts.googleapis.com/css?family=Open+Sans\"\
      rel=\"stylesheet\">\
    <style>\
        body {\
          font-family: 'Open Sans', sans-serif;\
          background-color: #fafafa;\
          color: #333;\
          margin: 0;\
          padding: 0;\
        }\
        .center {\
          display: flex; \
          justify-content: center;\
          align-items: center;\
          height: 100vh;\
        }\
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
        .form__input:focus {  \
          box-shadow: 0 0 10px rgba(0, 0, 0, 0.2); /* Add shadow when input is focused */\
          background-color: #e6e6e6; /* Change background color when input is focused */\
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
          color: #525557; /* Change label text color */\
          font-weight: bold; /* Make label text bold */;\
        }\
      \
\
\
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
            <label for=\"password\">PASSWORD</label>\
            <input type=\"password\" id=\"password\" name=\"password\"\
              class=\"form__input\">\
          </div>\
          <div class=\"form__buttons\">\
            <button type=\"submit\" class=\"form__button\">CONNECT</button>\
            <button type=\"button\" class=\"form__button\"\
              onclick=\"scan()\">SCAN WIFI</button>\
          </div>\
          <div class=\"form__group\">\
            <label for=\"ssidList\">Available SSIDs</label>\
            <select id=\"ssidList\" class=\"form__input\"></select>\
          </div>\
        </form>\
      </div>\
    </div>\
    <script>\
    document.querySelector('form').addEventListener('submit', function(event) {\
  event.preventDefault();\
  var ssid = document.getElementById('ssid').value;\
  var password = document.getElementById('password').value;\
  \
\
  var xhr = new XMLHttpRequest();\
  xhr.open('POST', '/connect', true);\
  xhr.setRequestHeader('Content-Type', 'application/json');\
  xhr.onreadystatechange = function() {\
    if (xhr.readyState === 4 && xhr.status === 200) {\
      var response = JSON.parse(xhr.responseText);\
      alert(response.message);\
    }\
  };\
  var data = JSON.stringify({ ssid: ssid, password: password });\
  xhr.send(data);\
\
  xhr.timeout = 10000;\
  xhr.ontimeout = function() {\
    alert('Timeout: No response received.');\
  };\
});\
    </script>\
  </body>\
</html>\
"

#endif