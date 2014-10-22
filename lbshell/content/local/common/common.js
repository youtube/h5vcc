(function() {  // anonymous namespace
  var recognition = null;
  var speechElementMap = {};

  function createGrammar(action, text) {
    var locale = navigator.language;
    if (!navigator.language) {
      console.log("Failed to detect locale");
      locale = 'en-US';
    }
    var grammar = '<?xml version="1.0" encoding="ISO-8859-1"?> \
        <!DOCTYPE grammar PUBLIC "-//W3C//DTD GRAMMAR 1.0//EN" \
                         "http://www.w3.org/TR/speech-grammar/grammar.dtd"> \
        <!-- the default grammar language is US English --> \
        <grammar xmlns="http://www.w3.org/2001/06/grammar" \
                xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" \
                xsi:schemaLocation="http://www.w3.org/2001/06/grammar \
                                    http://www.w3.org/TR/speech-grammar/grammar.xsd" \
                xml:lang="LOCALE_TEXT" version="1.0"> \
        <rule id="ACTION_TEXT"> \
           <action>ACTION_TEXT</action> \
           <phrase>PHRASE_TEXT</phrase> \
        </rule> \
        </grammar>';
    grammar = grammar.replace(/LOCALE_TEXT/g, locale);
    grammar = grammar.replace(/ACTION_TEXT/g, action);
    return grammar.replace(/PHRASE_TEXT/g, text);
  }

  function onSpeechResult(event) {
    if (!event) {
      return;
    }
    var action = event.interpretation;
    speechElementMap[action].onclick();
  }

  function fillElement(element) {
    var key = element.id;
    var text = window['h5vcc'] && h5vcc.system.getLocalizedString(key);
    if (text != undefined) {
      element.innerText = text;
    } else {
      console.log('ERROR: No text for key: ' + key);
      element.innerText = key;
    }
  }

  function onResize() {
    var defaultHeight = 720;
    var bodyTag = document.body;
    var percent = bodyTag.offsetHeight / defaultHeight;
    document.body.style.fontSize = 100 * percent + '%';
  }

  function changeFocus(className) {
    var focus_element = document.getElementsByClassName(className)[0];
    if (focus_element) {
      focus_element.focus();
    }
  }

  function onKeyUp() {
    event.preventDefault();
    event.stopPropagation();

    switch (event.keyCode) {
      case 39:  // Right.
        changeFocus('right-focus');
        break;

      case 37:  // Left.
        changeFocus('left-focus');
        break;

      case 13:  // Enter.
        if (!!document.activeElement && !!document.activeElement.onclick) {
          document.activeElement.onclick();
        }
        break;
      case 27:  // Escape.
        if (window['h5vcc']) {
          h5vcc.system.minimize();
        }
        break;
    }
  }

  function onKeyDown() {
    event.preventDefault();
    event.stopPropagation();
  }

  function setupText() {
    var elements = document.getElementsByClassName('auto-fill');
    for (var i = 0; i < elements.length; ++i) {
      fillElement(elements[i]);
    }
  }

  function setupSpeech() {
    if (!window['webkitSpeechRecognition']) {
      return;
    }

    var elements = document.getElementsByClassName('speech-target');
    if (!elements.length) {
      return;
    }

    recognition = new webkitSpeechRecognition();
    recognition.continuous = true;
    recognition.interimResults = true;

    for (var i = 0; i < elements.length; ++i) {
      var text = elements[i].innerText;
      var action = elements[i].id
      var grammar = createGrammar(action, text);
      speechElementMap[action] = elements[i];
      recognition.grammars.addFromString(grammar);
      recognition.grammars[i].weight = 0.5;
    }

    recognition.onresult = onSpeechResult;

    recognition.onerror = function() {
      console.log('onerror (code: ' + event.error + ') ' + event.message);
    }

    recognition.start();
  }

  function setupGesture() {
    var elements = document.getElementsByClassName('gesturable');
    for (var i = 0; i < elements.length; ++i) {
      elements[i].onmouseover = function() { this.focus(); };
    }
  }

  function onLoad() {
    onResize();
    setupText();
    setupSpeech();
    setupGesture();
    changeFocus('default-focus');
    h5vcc.system.hideSplashScreen();
  }

  window.addEventListener('resize', onResize);
  document.addEventListener('keydown', onKeyDown);
  document.addEventListener('keyup', onKeyUp);
  document.addEventListener('DOMContentLoaded', onLoad);
})();  // execute
