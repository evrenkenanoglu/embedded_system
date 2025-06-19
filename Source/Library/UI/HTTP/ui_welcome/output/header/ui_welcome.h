#ifndef UI_WELCOME_H
#define UI_WELCOME_H

#define HTML_UI_WELCOME_CONTENT "<!DOCTYPE html>\
<html>\
 <head>\
  <meta charset=\"utf-8\"/>\
  <meta content=\"width=device-width, initial-scale=1.0, user-scalable=no\" name=\"viewport\"/>\
  <title>\
   Device Dashboard\
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
   /* === WiFi Setup Page Specific Styles === */\
\
/* Define input background colors as CSS variables (add to :root if not already there for other purposes) */\
:root {\
    /* ... your existing variables ... */\
    --input-bg-normal: #f8f9fa;\
    --input-bg-error: #ffebee;   /* Light red */\
    --input-bg-warning: #fffde7; /* Light yellow */\
    --input-bg-success: #e8f5e9; /* Light green */\
    --input-border-color: #ced4da;\
    --input-focus-border-color: var(--primary-color);\
    --input-focus-box-shadow: 0 0 0 0.2rem rgba(0, 123, 255, 0.25); /* Use your primary color's RGB */\
                                /* If --primary-color is #007bff, then 0,123,255 */\
}\
\
\
.content-page.wifi-setup-page {\
    /* Add any specific overall styling for the wifi page here if needed */\
}\
\
.wifi-form-container {\
    background-color: var(--card-background-color); /* Or transparent if .content-page handles it */\
    padding: 25px;\
    border-radius: var(--border-radius);\
    /* box-shadow: var(--box-shadow); Remove if .content-page already has shadow and this is inside */\
    margin-top: 20px; /* Space between page title/description and form */\
    max-width: 500px; /* Max width for the form itself */\
    margin-left: auto;\
    margin-right: auto;\
}\
\
.wifi-form-container .form__title {\
    font-size: 1.5em; /* Adjust size as needed */\
    font-weight: 500;\
    margin-bottom: 25px;\
    text-align: center;\
    color: var(--primary-color); /* Or keep your gradient if you prefer */\
    /* If keeping gradient from your original:\
    background: linear-gradient(90deg, #0077b6, #00a8ff);\
    -webkit-background-clip: text;\
    -webkit-text-fill-color: transparent;\
    user-select: none;\
    */\
}\
\
.form__group {\
    margin-bottom: 20px;\
}\
\
.form__group label {\
    display: block;\
    font-weight: 500;\
    color: var(--text-color-subtle);\
    margin-bottom: 8px;\
    font-size: 0.9em;\
}\
\
.form__input {\
    width: 100%;\
    padding: 12px 15px;\
    font-size: 1em;\
    font-family: 'Roboto', sans-serif;\
    border: 1px solid var(--input-border-color);\
    border-radius: 8px; /* Modern rounded corners */\
    background-color: var(--input-bg-normal);\
    color: var(--text-color);\
    box-sizing: border-box; /* Important for consistent sizing */\
    transition: border-color 0.2s ease, box-shadow 0.2s ease, background-color 0.2s ease;\
}\
\
.form__input::placeholder {\
    color: #999;\
}\
\
.form__input:focus {\
    outline: none;\
    border-color: var(--input-focus-border-color);\
    box-shadow: var(--input-focus-box-shadow);\
    background-color: #fff; /* Brighter background on focus */\
}\
\
.form__buttons {\
    display: flex;\
    flex-direction: column; /* Stack buttons */\
    gap: 15px; /* Space between stacked buttons */\
    margin-top: 25px;\
}\
\
.form__button {\
    background-color: var(--primary-color);\
    border: none;\
    border-radius: 8px;\
    color: white;\
    cursor: pointer;\
    font-size: 1em;\
    font-weight: 500;\
    padding: 12px 20px;\
    text-align: center;\
    transition: background-color 0.2s ease, transform 0.1s ease;\
    width: 100%;\
    display: flex; /* For aligning text and loading dots */\
    align-items: center;\
    justify-content: center;\
}\
\
.form__button:hover:not(.disabled) {\
    background-color: var(--primary-hover-color);\
}\
\
.form__button:active:not(.disabled) {\
    transform: scale(0.98);\
}\
\
.form__button.scan-button {\
    background-color: #5a6573; /* A different color for secondary action, or keep primary */\
}\
.form__button.scan-button:hover:not(.disabled) {\
    background-color: #48515a;\
}\
\
.form__button.disabled,\
.form__button:disabled { /* Style for disabled state */\
    background-color: #cccccc;\
    color: #888888;\
    cursor: not-allowed;\
    transform: none;\
}\
\
\
/* Loading dots animation */\
.loading-dots span {\
    animation: blink 1.4s infinite both;\
    display: inline-block; /* Ensures they sit correctly with text */\
    margin-left: 1px;\
}\
.loading-dots span:nth-child(2) {\
    animation-delay: 0.2s;\
}\
.loading-dots span:nth-child(3) {\
    animation-delay: 0.4s;\
}\
@keyframes blink {\
    0% { opacity: .2; }\
    20% { opacity: 1; }\
    100% { opacity: .2; }\
}\
\
/* Ensure datalist dropdown matches input style (browser dependent) */\
#ssidList {\
    /* Basic styling, actual appearance is very browser-dependent */\
    font-family: 'Roboto', sans-serif;\
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
    <a class=\"icon-button\" href=\"wifiSetup\" id=\"wifi-button\">\
     <div class=\"icon-symbol\">\
      📶\
     </div>\
     <div class=\"icon-label\">\
      WiFi Setup\
     </div>\
    </a>\
    <a class=\"icon-button\" href=\"application.html\" id=\"app-button\">\
     <div class=\"icon-symbol\">\
      🚀\
     </div>\
     <div class=\"icon-label\">\
      Application\
     </div>\
    </a>\
    <a class=\"icon-button\" href=\"settings.html\" id=\"settings-button\">\
     <div class=\"icon-symbol\">\
      ⚙️\
     </div>\
     <div class=\"icon-label\">\
      Settings\
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