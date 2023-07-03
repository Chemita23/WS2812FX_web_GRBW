#include <pgmspace.h>

/*
The tiny Javascript/canvas based color picker is based on the clever work of the folks
at Sparkbox. https://seesparkbox.com/foundry/how_i_built_a_canvas_color_picker
*/

char main_js[] PROGMEM = R"=====(

var activeButton = null;
var colorCanvas = null;
var white = 250;
var r = 180;
var g = 80;
var b = 0;

window.addEventListener('DOMContentLoaded', (event) => {
  // init the canvas color picker
  colorCanvas = document.getElementById('color-canvas');
  var colorctx = colorCanvas.getContext('2d');

  // Create color gradient
  var gradient = colorctx.createLinearGradient(0, 0, colorCanvas.width - 1, 0);
  gradient.addColorStop(0,    "rgb(255,   0,   0)");
  gradient.addColorStop(0.16, "rgb(255,   0, 255)");
  gradient.addColorStop(0.33, "rgb(0,     0, 255)");
  gradient.addColorStop(0.49, "rgb(0,   255, 255)");
  gradient.addColorStop(0.66, "rgb(0,   255,   0)");
  gradient.addColorStop(0.82, "rgb(255, 255,   0)");
  gradient.addColorStop(1,    "rgb(255,   0,   0)");

  // Apply gradient to canvas
  colorctx.fillStyle = gradient;
  colorctx.fillRect(0, 0, colorCanvas.width - 1, colorCanvas.height - 1);

  // Create semi transparent gradient (white -> transparent -> black)
  gradient = colorctx.createLinearGradient(0, 0, 0, colorCanvas.height - 1);
  gradient.addColorStop(0,    "rgba(255, 255, 255, 1)");
  gradient.addColorStop(0.48, "rgba(255, 255, 255, 0)");
  gradient.addColorStop(0.52, "rgba(0,     0,   0, 0)");
  gradient.addColorStop(1,    "rgba(0,     0,   0, 1)");

  // Apply gradient to canvas
  colorctx.fillStyle = gradient;
  colorctx.fillRect(0, 0, colorCanvas.width - 1, colorCanvas.height - 1);

  // setup the canvas click listener
  colorCanvas.addEventListener('click', (event) => {
    var imageData = colorCanvas.getContext('2d').getImageData(event.offsetX, event.offsetY, 1, 1);

    var selectedColor = 'rgb(' + white + ',' + imageData.data[0] + ',' + imageData.data[1] + ',' + imageData.data[2] + ')'; 
    console.log('click: ' + event.offsetX + ', ' + event.offsetY + ', ' + selectedColor);
    r = imageData.data[0];
    g = imageData.data[1];
    b = imageData.data[2];
    document.getElementById('color-value').value = selectedColor;
    document.getElementById('white-value').value = white;
    document.getElementById('red-value').value = r;
    document.getElementById('green-value').value = g;
    document.getElementById('blue-value').value = b;

    selectedColor = white * 16777216 + imageData.data[0] * 65536 + imageData.data[1] * 256 + imageData.data[2];
    submitVal('c', selectedColor);
  });

  // get list of modes from ESP
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
   if (xhttp.readyState == 4 && xhttp.status == 200) {
     document.getElementById('modes').innerHTML = xhttp.responseText;
     modes = document.querySelectorAll('ul#modes li a');
     modes.forEach(initMode);
   }
  };
  xhttp.open('GET', 'modes', true);
  xhttp.send();
});

function initMode(mode, index) {
  mode.addEventListener('click', (event) => onMode(event, index));
}

function onColor(event, color) {
  event.preventDefault();
  var match = color.match(/rgb\(([0-9]*),([0-9]*),([0-9]*),([0-9]*)\)/);
  if(match) {
    var colorValue = Number(match[1]) * 16777216 + Number(match[2]) * 65536 + Number(match[3]) * 256 + Number(match[4]);
    white = Number(match[1]);
    r = Number(match[2]);
    g = Number(match[3]);
    b = Number(match[4]);    
    console.log('onColor:' + match[1] + "," + match[2] + "," + match[3] + "," + match[4] + "," + colorValue);
    document.getElementById('white-value').value = white;
    document.getElementById('red-value').value = r;
    document.getElementById('green-value').value = g;
    document.getElementById('blue-value').value = b;
    submitVal('c', colorValue);
  }
}

function onRed(event, color) {
  event.preventDefault();
  var match = color.match(/([0-9]*)/);
  if(match) {
    r = Number(match[1]);
    console.log('Red: ' + match[1]);
    var colorValue = white * 16777216 + Number(match[1]) * 65536 + g * 256 + b;
    var selectedColor = 'rgb(' + white + ',' + match[1] + ',' + g + ',' + b + ')'; 
    document.getElementById('color-value').value = selectedColor;
    submitVal('c', colorValue);
  }
}

function onGreen(event, color) {
  event.preventDefault();
  var match = color.match(/([0-9]*)/);
  if(match) {
    g = Number(match[1]);
    console.log('Green: ' + match[1]);
    var colorValue = white * 16777216 + r * 65536 + Number(match[1]) * 256 + b;
    var selectedColor = 'rgb(' + white + ',' + r + ',' + match[1] + ',' + b + ')'; 
    document.getElementById('color-value').value = selectedColor;
    submitVal('c', colorValue);
  }
}

function onBlue(event, color) {
  event.preventDefault();
  var match = color.match(/([0-9]*)/);
  if(match) {
    b = Number(match[1]);
    console.log('Blue: ' + match[1]);
    var colorValue = white * 16777216 + r * 65536 + g * 256 + Number(match[1]);
    var selectedColor = 'rgb(' + white + ',' + r + ',' + g + ',' + match[1] + ')'; 
    document.getElementById('color-value').value = selectedColor;
    submitVal('c', colorValue);
  }
}

function onWhite(event, color) {
  event.preventDefault();
  var match = color.match(/([0-9]*)/);
  if(match) {
    white = Number(match[1]);
    console.log('White: ' + match[1] + "   R: " + r + "   G: " + g + "   B: " + b);
    var colorValue = Number(match[1]) * 16777216  + r * 65536 + g * 256 + b;
    var selectedColor = 'rgb(' + Number(match[1]) + ',' + r + ',' + g + ',' + b + ')'; 
    document.getElementById('color-value').value = selectedColor;
    submitVal('c', colorValue);
  }
}


function onMode(event, mode) {
  event.preventDefault();
  if(activeButton) activeButton.classList.remove('active')
  activeButton = event.target;
  activeButton.classList.add('active');
  console.log('Mode: ' + mode);
  submitVal('m', mode);
}

function onBrightness(event, dir) {
  event.preventDefault();
  console.log('Brigh: ' + dir);
  submitVal('b', dir);
}



function onSped(event, dir) {
  event.preventDefault();
  console.log('Delay: ' + dir);
  submitVal('x', dir);
}

function onAuto(event, dir) {
  event.preventDefault();
  submitVal('a', dir);
}

function submitVal(name, val) {
  var xhttp = new XMLHttpRequest();
  xhttp.open('GET', 'set?' + name + '=' + val, true);
  xhttp.send();
}
)=====";
