const component_list = [
  "href",
  "protocol",
  "username",
  "password",
  "host",
  "hostname",
  "port",
  "pathname",
  "search",
  "hash",
  "origin"
];

var public_suffix_list = null;

function getUrl() {
  // construct URL
  const url = elemBase.value.length !== 0
    ? new Module.URL(elemUrl.value, elemBase.value)
    : new Module.URL(elemUrl.value);
  // apply setter
  if (elemUseSetter.checked)
    url[elemSetter.value] = elemSetterInp.value;
  return url;
}

function showResult(url) {
  const output = document.getElementById("url-output");
  const error = document.getElementById("url-error");
  if (url.valid) {
    output.hidden = false;
    error.hidden = true;
    for (const component of component_list) {
      const trComponent = output.querySelector(`.${component}`);
      if (trComponent) {
        const elemComponent = trComponent.querySelector("td");
        elemComponent.textContent = url[component];
      }
    }
    if (public_suffix_list) {
      const trComponent = output.querySelector(".domain");
      if (trComponent) {
        trComponent.querySelector("td").textContent =
          public_suffix_list.url_registrable_domain(url);
      }
    }
  } else {
    output.hidden = true;
    error.hidden = false;
    error.textContent = url.base_valid ? "URL parse error" : "Base URL parse error";
  }
}

function onInpChange() {
  const url = getUrl();
  showResult(url);
  url.delete();
}

function onSetterChange() {
  elemSetter.disabled = !elemUseSetter.checked;
  elemSetterInp.disabled = !elemUseSetter.checked;
  onInpChange();
}

async function onInit() {
  onSetterChange();

  // Download Public suffix list
  const psl = new Module.PSL();
  try {
    const response = await fetch("public_suffix_list.dat");
    const reader = response.body.getReader();
    while (true) {
      const { done, value } = await reader.read();
      if (done) {
        if (psl.finalize())
          public_suffix_list = psl;
        else
          psl.delete();
        break;
      }
      psl.push(value);
    }
  } catch (error) {
    psl.delete();
  }
  if (public_suffix_list) {
    // show registrable domain row
    document.querySelector("#url-output .domain").style.display = null;
    onInpChange();
  }
}

// Input elements
const elemUrl = document.getElementById("url");
const elemBase = document.getElementById("base");
const elemUseSetter = document.getElementById("use-setter");
const elemSetter = document.getElementById("setter");
const elemSetterInp = document.getElementById("setter-inp");

// After text change
elemUrl.addEventListener("input", onInpChange);
elemBase.addEventListener("input", onInpChange);
elemUseSetter.addEventListener("change", onSetterChange);
elemSetter.addEventListener("change", onSetterChange);
elemSetterInp.addEventListener("input", onInpChange);

// Fill setters list
for (var component of component_list) {
  if (component !== "origin") {
    var option = document.createElement("option");
    option.text = component;
    option.value = component;
    elemSetter.add(option);
  }
}

// Parse initial URL
if (Module.URL)
  onInit();
else
  Module["onRuntimeInitialized"] = onInit;
