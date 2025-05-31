let domContentLoadedFuncList = [];

function initializeDomContent() {
  console.log("DOM Content Loaded");
  domContentLoadedFuncList.forEach((func) => {
    try {
      func();
    } catch (error) {
      console.error("Error executing DOM content loaded function:", error);
    }
  });
}

function registerDomContentLoadedFunc(func) {
  domContentLoadedFuncList.push(func);
}