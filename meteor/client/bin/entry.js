var Module = {
  //preRun: [memoryprofiler_add_hooks],
  preRun: [],
  postRun: [InitializeGraphics],
  print: (function() {
    var element = document.getElementById('stdout');
    if (element) element.value = ''; // clear browser cache
    return function(text) {
      text = Array.prototype.slice.call(arguments).join(' ');
      console.log(text);
      if (element) {
        element.value += text + "\n";
        element.scrollTop = element.scrollHeight; // focus on bottom
      }
    };
  })(),
  printErr: function(text) {
    text = Array.prototype.slice.call(arguments).join(' ');
    console.error(text);
  },
  setStatus: function(text) {
    if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
    if (text === Module.setStatus.text) return;
    var match = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
    var now = Date.now();
    if (match && now - Date.now() < 30) return; // if this is a progress update, skip it if too soon
    if (match) {
      text = match[1];
    } else if (!text) {
      // This case occurs when the canvas is loaded.
    }
    console.log(text);
  },
  totalDependencies: 0,
  monitorRunDependencies: function(left) {
    this.totalDependencies = Math.max(this.totalDependencies, left);
    Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
  }
};

Module.setStatus('Downloading...');
// Use the default amount of memory, 16Mb.
//Module.TOTAL_MEMORY = 32*1024*1024;

var bindings = {};
function InitializeGraphics() {
  // The graphics instance will also create animation and engine objects.
  bindings.graphics = new BabelGraphics($('.canvas-wrapper'), bindings);
  bindings.input = new BabelInput(bindings);
};

window.Module = Module;
window.bindings = bindings;

window.onerror = function(event) {
  // TODO: do not warn on ok events like simulating an infinite loop or exitStatus
  Module.setStatus('Exception thrown, see JavaScript console');
  Module.setStatus = function(text) {
    if (text) Module.printErr('[post-exception status] ' + text);
  };
};
