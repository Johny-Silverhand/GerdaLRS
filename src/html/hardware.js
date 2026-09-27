@@include("i18n-ru.js")

/* eslint-disable no-unused-vars */
/* eslint-disable comma-dangle */
/* eslint-disable require-jsdoc */

document.addEventListener('DOMContentLoaded', onReady, false);

function _(el) {
  return document.getElementById(el);
}

function onReady() {
  gerdaApplyI18n();
  // Add some tooltips to the pin type icons [CSS class name, Label]
  [['icon-input', 'Digital Input'], ['icon-output', 'Digital Output'], ['icon-analog', 'Analog Input'], ['icon-pwm', 'PWM Output']].forEach((t) =>
  {
    let imgs = document.getElementsByClassName(t[0]);
    [...imgs].forEach(i => i.title = t[1]);
  });

  if (window.File && window.FileList && window.FileReader) {
    initFiledrag();
  }

  loadHeader();
  loadData();
}

function loadHeader() {
  const xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState === 4 && this.status === 200) {
      const data = JSON.parse(this.responseText);
      const cfg = data.config || {};
      const opt = data.options || {};
      if (_('product_name') && cfg.product_name) _('product_name').textContent = cfg.product_name;
      if (_('link_state')) _('link_state').textContent = cfg.link_state || 'Wi‑Fi';
      if (_('reg_domain')) {
        const id = (opt['gerda-band'] !== undefined && opt['gerda-band'] !== null) ? Number(opt['gerda-band']) : (Number(opt['gerda-2g4']) === 1 ? 30 : 0);
        const sub = ['AU 915', 'FCC 915', 'EU 868', 'IN 866', 'AU 433', 'EU 433', 'US 433', 'US 433 Wide'];
        let title = 'ISM 2.4';
        if (id === 30) title = 'CUSTOM_2640';
        else if (id >= 10 && id <= 17) title = sub[id - 10];
        else if (id >= 20 && id <= 27) title = 'Dual ' + sub[id - 20] + ' + ISM 2.4';
        _('reg_domain').textContent = title;
      }
    }
  };
  xhr.open('GET', '/config', true);
  xhr.send();
}

function loadData() {
  xmlhttp = new XMLHttpRequest();
  xmlhttp.onreadystatechange = function() {
    if (this.readyState === 4 && this.status === 200) {
      const data = JSON.parse(this.responseText);
      updateHardwareSettings(data);
    }
  };
  xmlhttp.open('GET', '/hardware.json', true);
  xmlhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
  xmlhttp.send();
}

function submitHardwareSettings() {
  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/hardware.json');
  xhr.setRequestHeader('Content-Type', 'application/json');
  const formData = new FormData(_('upload_hardware'));
  xhr.send(JSON.stringify(Object.fromEntries(formData), function(k, v) {
    if (v === '') return undefined;
    if (_(k) && _(k).type === 'checkbox') {
      return v === 'on';
    }
    if (_(k) && _(k).classList.contains('array')) {
      const arr = v.split(',').map((element) => {
        return Number(element);
      });
      return arr.length === 0 ? undefined : arr;
    }
    return isNaN(v) ? v : +v;
  }));
  xhr.onreadystatechange = function() {
    if (this.readyState === 4 && this.status === 200) {
      cuteAlert({
        type: 'question',
        title: 'Upload Succeeded',
        message: 'Reboot to take effect',
        confirmText: 'Reboot',
        cancelText: 'Close'
      }).then((e) => {
        if (e === 'confirm') {
          const xhr = new XMLHttpRequest();
          xhr.open('POST', '/reboot');
          xhr.setRequestHeader('Content-Type', 'application/json');
          xhr.onreadystatechange = function() {};
          xhr.send();
        }
      });
    }
  };
  return false;
}

function updateHardwareSettings(data) {
  for (const [key, value] of Object.entries(data)) {
    if (_(key)) {
      if (_(key).type === 'checkbox') {
        _(key).checked = value;
      } else {
        if (Array.isArray(value)) _(key).value = value.toString();
        else _(key).value = value;
      }
    }
  }
  if (data.customised) _('custom_config').style.display = 'block';
}

function fileDragHover(e) {
  e.stopPropagation();
  e.preventDefault();
  if (e.target === _('filedrag')) e.target.className = (e.type === 'dragover' ? 'hover' : '');
}

function fileSelectHandler(e) {
  fileDragHover(e);
  const files = e.target.files || e.dataTransfer.files;
  _('upload_hardware').reset();
  for (const f of files) {
    parseFile(f);
  }
}

function parseFile(file) {
  const reader = new FileReader();
  reader.onload = function(e) {
    const data = JSON.parse(e.target.result);
    updateHardwareSettings(data);
  };
  reader.readAsText(file);
}

function initFiledrag() {
  const fileselect = _('fileselect');
  const filedrag = _('filedrag');

  fileselect.addEventListener('change', fileSelectHandler, false);

  const xhr = new XMLHttpRequest();
  if (xhr.upload) {
    filedrag.addEventListener('dragover', fileDragHover, false);
    filedrag.addEventListener('dragleave', fileDragHover, false);
    filedrag.addEventListener('drop', fileSelectHandler, false);
    filedrag.style.display = 'block';
  }
}

@@include("libs.js")
