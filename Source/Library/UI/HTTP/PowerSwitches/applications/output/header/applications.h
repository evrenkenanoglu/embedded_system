#ifndef APPLICATIONS_H
#define APPLICATIONS_H

#define HTML_APPLICATIONS_CONTENT "<!DOCTYPE html>\
<html>\
    <head>\
        <meta charset=\"utf-8\"/>\
        <meta content=\"width=device-width, initial-scale=1.0, user-scalable=no\" name=\"viewport\"/>\
        <title>\
            Applications\
        </title>\
        <style>\
            :root {\
    /* Existing: A good vibrant blue */\
    --primary-color: #007bff; \
    /* Existing */\
    --primary-hover-color: #0056b3; \
    /* NEW: Lighter shade for subtle backgrounds or accents */\
    --primary-light-color: #e0f0ff; \
\
    /* SLIGHTLY ADJUSTED: A bit cooler and more modern than #f4f7f9 */\
    --background-color: #f0f2f5; \
    /* Existing */\
    --card-background-color:\
    /* ADJUSTED: Darker, slightly desaturated blue-grey for better readability */ #ffffff; \
    --text-color: #2c3e50; \
    /* Existing */\
    --text-color-subtle: #5a6573; \
    /* Existing */\
    --icon-color: var(--primary-color); \
\
    /* NEW: For smaller elements like input fields */\
    --border-radius-sm: 6px; \
    /* ADJUSTED: Slightly smaller for a sharper look on cards/buttons */\
    --border-radius-md: 10px; \
    /* NEW: For larger containers if needed */\
    --border-radius-lg: 16px; \
\
    /* REFINED Shadows for a more modern, layered look */\
    --box-shadow-sm: 0 2px 4px rgba(0, 0, 0, 0.05);\
    --box-shadow-md: 0 6px 12px rgba(0, 0, 0, 0.08);\
    --box-shadow-lg: 0 10px 20px rgba(0, 0, 0, 0.1);\
    --box-shadow-hover-md: 0 8px 16px rgba(0, 0, 0, 0.12);\
    --box-shadow-inset: inset 0 2px 4px rgba(0, 0, 0, 0.05);\
     /* NEW: For pressed states or input fields */\
\
    /* Input States - Kept existing good ones, refined some */\
    --input-bg-normal: #f8f9fa;\
    --input-bg-error: #ffebee;\
    --input-bg-warning: #fffde7;\
    --input-bg-success: #e8f5e9;\
    --input-border-color: #d1d9e6;\
     /* SLIGHTLY ADJUSTED: Softer border */\
    --input-border-color-error: #e57373;\
     /* NEW: Specific error border */\
    --input-border-color-success: #81c784;\
     /* NEW: Specific success border */\
    --input-focus-border-color: var(--primary-color);\
    --input-focus-box-shadow: 0 0 0 0.15rem rgba(0, 123, 255, 0.2);\
     /* REFINED: Slightly softer focus ring */\
    --input-placeholder-color: #90a4ae;\
     /* NEW: For placeholder text */\
\
    --transition-speed: 0.25s;\
     /* NEW: Standard transition speed */\
}\
\
body {\
    font-family: 'Roboto', -apple-system, BlinkMacSystemFont, \"Segoe UI\", \"Helvetica Neue\", Arial, sans-serif;\
     /* Added system fonts as fallback */\
    margin: 0;\
    background-color: var(--background-color);\
    color: var(--text-color);\
    display: flex;\
    flex-direction: column;\
    min-height: 100vh;\
    line-height: 1.6;\
    -webkit-font-smoothing: antialiased;\
     /* Smoother fonts */\
    -moz-osx-font-smoothing: grayscale;\
}\
\
header {\
    background: linear-gradient(90deg, var(--primary-color) 0%, var(--primary-hover-color) 100%);\
     /* MODERN: Subtle gradient */\
    color: white;\
    padding: 20px 25px;\
     /* Increased horizontal padding */\
    text-align: center;\
    box-shadow: var(--box-shadow-sm);\
     /* Using new shadow variable */\
    position: sticky;\
     /* Optional: make header sticky */\
    top: 0;\
    z-index: 1000;\
}\
\
header h1 {\
    margin: 0;\
    font-weight: 400;\
     /* ADJUSTED: Slightly less bold for a cleaner look */\
    font-size: 1.8em;\
     /* ADJUSTED */\
    letter-spacing: 0.5px;\
}\
\
main {\
    flex-grow: 1;\
    display: flex;\
    flex-direction: column;\
    justify-content: center;\
    align-items: center;\
    padding: 30px 20px;\
     /* Increased top/bottom padding */\
}\
\
.icon-grid {\
    display: flex;\
    justify-content: center;\
     /* Center items if they don't fill the row */\
    align-items: stretch;\
    gap: 100px;\
    width: 100%;\
    max-width: 900px;\
    flex-wrap: wrap;\
}\
\
.icon-button {\
    background-color: var(--card-background-color);\
    border-radius: var(--border-radius-md);\
    box-shadow: var(--box-shadow-md);\
    padding: 25px 20px;\
    text-align: center;\
    text-decoration: none;\
    color: var(--text-color);\
    transition: transform var(--transition-speed) ease-in-out, box-shadow var(--transition-speed) ease-in-out, background-color var(--transition-speed) ease;\
    cursor: pointer;\
    display: flex;\
    flex-direction: column;\
    align-items: center;\
    justify-content: center;\
     /* Better vertical centering */\
    min-width: 160px;\
     /* Increased min-width */\
    flex: 1 1 180px;\
     /* Allow shrinking and growing, base width */\
    aspect-ratio: 1 / 1;\
    max-width: 220px;\
    border: 1px solid transparent;\
     /* For hover border transition */\
}\
\
.icon-button:hover {\
    transform: translateY(-6px);\
    box-shadow: var(--box-shadow-hover-md);\
    /* background-color: var(--primary-light-color); Optional: Subtle background change on hover */\
    border-color: var(--primary-color);\
     /* MODERN: Add border highlight on hover */\
}\
\
.icon-button:active {\
    transform: translateY(-2px) scale(0.98);\
     /* Added scale for more feedback */\
    box-shadow: var(--box-shadow-sm);\
}\
\
.icon-symbol {\
    font-size: 3.2em;\
     /* Slightly reduced for balance */\
    color: var(--icon-color);\
    margin-bottom: 15px;\
     /* Increased margin */\
    line-height: 1;\
    transition: transform var(--transition-speed) ease;\
}\
.icon-button:hover .icon-symbol {\
    transform: scale(1.1);\
     /* MODERN: Slight zoom on icon hover */\
}\
\
.icon-label {\
    font-size: 1em;\
    font-weight: 500;\
    color: var(--text-color-subtle);\
    letter-spacing: 0.2px;\
     /* Reduced letter spacing */\
}\
\
footer {\
    text-align: center;\
    padding: 20px;\
     /* Increased padding */\
    font-size: 0.85em;\
     /* Slightly smaller */\
    color: var(--text-color-subtle);\
     /* Using variable */\
    background-color: var(--card-background-color);\
     /* Consistent with cards */\
    border-top: 1px solid #e0e4e8;\
     /* Softer separator */\
}\
\
/* Basic styling for target pages (like wifi.html, settings.html) */\
.content-page {\
    padding: 30px 35px;\
     /* Increased padding */\
    width: 100%;\
    max-width: 700px;\
     /* Slightly reduced for tighter content feel */\
    margin: 20px auto;\
    background-color: var(--card-background-color);\
    border-radius: var(--border-radius-lg);\
     /* Larger radius for content page */\
    box-shadow: var(--box-shadow-lg);\
     /* More prominent shadow for content page */\
}\
\
.content-page h2 {\
    color: var(--primary-color);\
    margin-top: 0;\
    margin-bottom: 10px;\
    font-weight: 500;\
     /* Bolder for page titles */\
    letter-spacing: 0.3px;\
    text-align: center;\
    font-size: 1.8em;\
     /* Larger page title */\
}\
\
.content-page p.page-description { /* Specific class for the description */\
    text-align: center;\
    color: var(--text-color-subtle);\
    margin-bottom: 30px;\
     /* Increased space */\
    font-size: 1em;\
    max-width: 500px;\
     /* Constrain description width */\
    margin-left: auto;\
    margin-right: auto;\
}\
\
.back-link {\
    display: inline-flex;\
     /* For icon alignment if added */\
    align-items: center;\
    justify-content: center;\
    margin: 30px auto 0 auto;\
    padding: 10px 22px;\
    background-color: var(--text-color-subtle);\
     /* Softer back button */\
    color: white;\
    text-decoration: none;\
    border-radius: var(--border-radius-sm);\
    transition: background-color var(--transition-speed) ease, transform var(--transition-speed) ease;\
    font-weight: 500;\
    text-align: center;\
    box-shadow: var(--box-shadow-sm);\
}\
\
.back-link:hover {\
    background-color: var(--text-color);\
     /* Darken on hover */\
    transform: translateY(-2px);\
}\
        </style>\
        <style>\
            /* Power Bar (Socket) Application Styling */\
\
/* Container for the entire power bar */\
.power-bar-container {\
  width: 100%;\
  max-width: 800px;\
  margin: 20px auto;\
  background-color: #2a2a2a;\
  border-radius: 15px;\
  padding: 20px;\
  box-shadow: 0 8px 16px rgba(0, 0, 0, 0.3);\
  border: 2px solid #444;\
}\
\
/* Header for the power bar application */\
.power-bar-header {\
  text-align: center;\
  padding: 10px 0;\
  margin-bottom: 20px;\
  border-bottom: 2px solid #444;\
  color: #f0f0f0;\
}\
\
/* Grid layout for multiple sockets */\
.socket-grid {\
  display: grid;\
  grid-template-columns: repeat(auto-fill, minmax(120px, 1fr));\
  gap: 15px;\
  padding: 15px;\
}\
\
/* Individual socket styling */\
.socket-icon-btn {\
  display: inline-flex;\
  align-items: center;\
  justify-content: center;\
  width: 100px;\
  height: 100px;\
  position: relative;\
  cursor: pointer;\
  transition: all 0.3s ease;\
  border: none;\
  background-color: #333;\
  border-radius: 50%; /* Make socket completely round */\
  box-shadow: inset 0 0 10px rgba(0, 0, 0, 0.5), 0 4px 8px rgba(0, 0, 0, 0.3);\
  padding: 10px;\
}\
\
/* Power outlet circles */\
.socket-icon-btn::before,\
.socket-icon-btn::after {\
  content: '';\
  position: absolute;\
  border-radius: 50%;\
  background-color: #222;\
  width: 20px;\
  height: 20px;\
  transition: background-color 0.3s ease;\
  box-shadow: inset 0 0 5px rgba(0, 0, 0, 0.8);\
}\
\
.socket-icon-btn::before {\
  /* Position in the middle, slightly to the left */\
  top: calc(50% - 10px);\
  left: calc(50% - 25px);\
}\
\
.socket-icon-btn::after {\
  /* Position in the middle, slightly to the right */\
  top: calc(50% - 10px);\
  left: calc(50% + 5px);\
}\
\
/* Power switch indicator - small indicator light at the bottom */\
.power-indicator {\
  position: absolute;\
  bottom: 15px;\
  width: 15px;\
  height: 15px;\
  background-color: #555;\
  border-radius: 50%;\
  transition: all 0.3s ease;\
  border: 2px solid #222;\
}\
\
/* Hover state */\
.socket-icon-btn:hover {\
  transform: translateY(-4px);\
  box-shadow: 0 6px 12px rgba(0, 0, 0, 0.3), inset 0 0 10px rgba(0, 0, 0, 0.5);\
  background-color: #3a3a3a;\
}\
\
/* Active/pressed state */\
.socket-icon-btn:active {\
  transform: translateY(0);\
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2), inset 0 0 10px rgba(0, 0, 0, 0.5);\
}\
\
/* When socket is turned on */\
.socket-icon-btn.active {\
  background-color: #444;\
}\
\
.socket-icon-btn.active::before,\
.socket-icon-btn.active::after {\
  background-color: #4CAF50; /* Green dots when active */\
  box-shadow: inset 0 0 5px rgba(0, 0, 0, 0.5), 0 0 8px rgba(76, 175, 80, 0.7);\
}\
\
.socket-icon-btn.active .power-indicator {\
  background-color: #4CAF50;\
  box-shadow: 0 0 10px rgba(76, 175, 80, 0.7);\
}\
\
/* Socket labels */\
.socket-label {\
  position: absolute;\
  bottom: -25px;\
  width: 100%;\
  text-align: center;\
  color: #f0f0f0;\
  font-size: 14px;\
}\
\
/* Power status display */\
.power-status {\
  display: flex;\
  justify-content: space-between;\
  margin-top: 30px;\
  padding: 15px;\
  background-color: #333;\
  border-radius: 10px;\
  color: #f0f0f0;\
}\
\
/* Power consumption meter */\
.power-meter {\
  height: 10px;\
  width: 100%;\
  background-color: #444;\
  border-radius: 5px;\
  margin-top: 5px;\
  overflow: hidden;\
}\
\
.power-meter-fill {\
  height: 100%;\
  width: 30%;\
  background-color: #4CAF50;\
  transition: width 0.3s ease;\
}\
\
/* Master control switch */\
.master-control {\
  margin-top: 20px;\
  text-align: center;\
}\
\
.master-switch {\
  background-color: #555;\
  color: white;\
  border: none;\
  padding: 10px 20px;\
  border-radius: 30px;\
  cursor: pointer;\
  transition: all 0.3s ease;\
  font-weight: bold;\
}\
\
.master-switch:hover {\
  background-color: #666;\
}\
\
.master-switch.all-on {\
  background-color: #4CAF50;\
}\
\
.master-switch.all-off {\
  background-color: #f44336;\
}\
        </style>\
    </head>\
    <body>\
        <header>\
            <div style=\"text-align: center; padding: 10px 0;\">\
                <div style=\"text-align: center; padding: 5px 0 10px 0;\">\
                    <span style=\"font-family: 'Arial Black', Gadget, sans-serif; font-size: 45px; font-weight: 900; color: #eee;\">\
                        UNIVERSE\
                        <span style=\"color: #b85c5c; font-weight: normal; font-family: Arial, sans-serif;\">\
                            Home\
                        </span>\
                    </span>\
                </div>\
                <h1>\
                    Control Panel\
                </h1>\
            </div>\
        </header>\
        <main id=\"welcome-page\">\
            <div class=\"icon-grid\">\
                <a class=\"icon-button\" href=\"powerSwitchesApp\" id=\"app-button\">\
                    <div class=\"icon-symbol\">\
                        🔌\
                    </div>\
                    <div class=\"icon-label\">\
                        Socket Control\
                    </div>\
                </a>\
                <a class=\"icon-button\" href=\"powerSwitchesScheduler\" id=\"app-button\">\
                    <div class=\"icon-symbol\">\
                        ⏰\
                    </div>\
                    <div class=\"icon-label\">\
                        Socket Scheduler\
                    </div>\
                </a>\
            </div>\
        </main>\
        <footer>\
            <p>\
                Device OS v1.0\
            </p>\
        </footer>\
        <script>\
            console.log('No inline scripts found');\
        </script>\
    </body>\
</html>\
"

#endif