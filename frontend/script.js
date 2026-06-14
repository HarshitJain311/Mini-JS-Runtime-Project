// const fileInput = document.getElementById('fileInput');
// const dropzone = document.getElementById('dropzone');
// const chooseBtn = document.getElementById('chooseBtn');
// const filenameEl = document.getElementById('filename');
// const validationEl = document.getElementById('validation');
// const uploadNotice = document.getElementById('uploadNotice');
// const codePreview = document.getElementById('codePreview');
// const runBtn = document.getElementById('runBtn');
// const spinner = document.getElementById('spinner');
// const consoleBox = document.getElementById('consoleBox');
// const errorBox = document.getElementById('errorBox');
// const clearBtn = document.getElementById('clearBtn');

// const MAX_BYTES = 200 * 1024; // 200KB
// const API_URL = 'http://localhost:8081/run';



// let currentFile = null;

// function escapeHtml(str){
//   return str
//     .replaceAll('&', '&amp;')
//     .replaceAll('<', '<')
//     .replaceAll('>', '>')
//     .replaceAll('"', '"')
//     .replaceAll("'", '&#039;');
// }

// function setNotice(text, type){
//   uploadNotice.textContent = text;
//   uploadNotice.style.color = type === 'error' ? 'var(--red)' : 'var(--muted)';
// }

// function setRunning(running){
//   if (running) {
//     runBtn.classList.add('running');
//     runBtn.disabled = true;
//   } else {
//     runBtn.classList.remove('running');
//     runBtn.disabled = !currentFile;
//   }
// }

// function validateFile(file){
//   if (!file) return { ok:false, msg:'No file selected.' };
//   if (!file.name.toLowerCase().endsWith('.js')) return { ok:false, msg:'Only .js files are allowed.' };
//   if (file.size > MAX_BYTES) return { ok:false, msg:'File too large. Limit is 200 KB.' };
//   return { ok:true, msg:'Looks good.' };
// }

// async function handleFile(file){
//   const v = validateFile(file);
//   filenameEl.textContent = file.name;
//   if (!v.ok){
//     validationEl.textContent = v.msg;
//     setNotice(v.msg, 'error');
//     currentFile = null;
//     runBtn.disabled = true;
//     return;
//   }

//   currentFile = file;
//   validationEl.textContent = v.msg;
//   setNotice('Upload successful. Ready to run.', 'ok');

//   const text = await file.text();
//   // Read-only preview
//   codePreview.textContent = text;
//   // Reset output areas
//   errorBox.textContent = ' ';
//   consoleBox.textContent = ' ';
//   runBtn.disabled = false;
// }

// chooseBtn.addEventListener('click', () => fileInput.click());
// fileInput.addEventListener('change', (e) => {
//   const file = e.target.files && e.target.files[0];
//   if (file) handleFile(file);
// });

// ['dragenter','dragover'].forEach(evt => {
//   dropzone.addEventListener(evt, (e) => {
//     e.preventDefault();
//     dropzone.classList.add('dragover');
//   });
// });

// ['dragleave','drop'].forEach(evt => {
//   dropzone.addEventListener(evt, (e) => {
//     e.preventDefault();
//     dropzone.classList.remove('dragover');
//   });
// });

// dropzone.addEventListener('drop', (e) => {
//   const file = e.dataTransfer.files && e.dataTransfer.files[0];
//   if (file) handleFile(file);
// });

// function appendToConsole(text){
//   consoleBox.textContent = (consoleBox.textContent || '').replace(/\s+$/, '')
//     ? consoleBox.textContent + '\n' + text
//     : text;
// }

// clearBtn.addEventListener('click', () => {
//   consoleBox.textContent = ' ';
//   errorBox.textContent = ' ';
//   setNotice('Console cleared.', 'ok');
// });

// runBtn.addEventListener('click', async () => {
//   if (!currentFile) return;

//   setRunning(true);
//   consoleBox.textContent = 'Running…';
//   errorBox.textContent = ' ';

//   try {
//     const form = new FormData();
//     form.append('file', currentFile);

//     const res = await fetch(API_URL, {
//       method: 'POST',
//       body: form
//     });

//     const data = await res.json();

//     if (data.success) {
//       consoleBox.textContent = data.output || ' ';
//       errorBox.textContent = ' ';
//       setNotice('Execution finished successfully.', 'ok');
//     } else {
//       consoleBox.textContent = ' ';
//       errorBox.textContent = data.error || 'Unknown error';
//       setNotice('Execution failed.', 'error');
//     }
//   } catch (err) {
//     consoleBox.textContent = ' ';
//     errorBox.textContent = String(err);
//     setNotice('Network/server error.', 'error');
//   } finally {
//     setRunning(false);
//   }
// });


// const API_URL = "http://localhost:8081/run";

// const fileInput = document.getElementById("fileInput");
// const runButton = document.getElementById("runButton");
// const outputConsole = document.getElementById("outputConsole");
// const errorConsole = document.getElementById("errorConsole");
// const preview = document.getElementById("codePreview");
// const filenameText = document.getElementById("filename");

// let selectedFile = null;

// // File Selection
// fileInput.addEventListener("change", async (e) => {

//     selectedFile = e.target.files[0];

//     if (!selectedFile) return;

//     filenameText.textContent = selectedFile.name;

//     const text = await selectedFile.text();

//     preview.textContent = text;
// });

// // Run Button
// runButton.addEventListener("click", async () => {

//     if (!selectedFile) {

//         errorConsole.textContent =
//             "Please select a JS file.";

//         return;
//     }

//     outputConsole.textContent = "Running...";
//     errorConsole.textContent = "";

//     try {

//         const formData = new FormData();

//         formData.append("file", selectedFile);

//         const response = await fetch(API_URL, {

//             method: "POST",
//             body: formData
//         });

//         const data = await response.json();

//         if (data.success) {

//             outputConsole.textContent =
//                 data.output;
//         }
//         else {

//             errorConsole.textContent =
//                 data.error;
//         }

//     } catch (err) {

//         errorConsole.textContent =
//             "Network/server error:\n\n" + err;
//     }
// });


const fileInput = document.getElementById('fileInput');
const dropzone = document.getElementById('dropzone');
const chooseBtn = document.getElementById('chooseBtn');
const filenameEl = document.getElementById('filename');
const validationEl = document.getElementById('validation');
const uploadNotice = document.getElementById('uploadNotice');
const codePreview = document.getElementById('codePreview');
const runBtn = document.getElementById('runBtn');
const consoleBox = document.getElementById('consoleBox');
const errorBox = document.getElementById('errorBox');
const clearBtn = document.getElementById('clearBtn');

const MAX_BYTES = 200 * 1024;

const API_URL = 'http://localhost:8081/run';

let currentFile = null;

function setNotice(text, type){

  uploadNotice.textContent = text;

  uploadNotice.style.color =
    type === 'error'
    ? '#ff4d6d'
    : '#39d98a';
}

function setRunning(running){

  if (running) {

    runBtn.classList.add('running');

    runBtn.disabled = true;

  } else {

    runBtn.classList.remove('running');

    runBtn.disabled = !currentFile;
  }
}

function validateFile(file){

  if (!file) {

    return {
      ok:false,
      msg:'No file selected.'
    };
  }

  if (!file.name
      .toLowerCase()
      .endsWith('.js')) {

    return {
      ok:false,
      msg:'Only .js files are allowed.'
    };
  }

  if (file.size > MAX_BYTES) {

    return {
      ok:false,
      msg:'File too large.'
    };
  }

  return {
    ok:true,
    msg:'Looks good.'
  };
}

async function handleFile(file){

  const v = validateFile(file);

  filenameEl.textContent = file.name;

  if (!v.ok){

    validationEl.textContent = v.msg;

    setNotice(v.msg, 'error');

    currentFile = null;

    runBtn.disabled = true;

    return;
  }

  currentFile = file;

  validationEl.textContent = v.msg;

  setNotice(
    'Upload successful.',
    'ok'
  );

  const text =
    await file.text();

  codePreview.textContent = text;

  errorBox.textContent = '';

  consoleBox.textContent = '';

  runBtn.disabled = false;
}

chooseBtn.addEventListener(
  'click',
  () => fileInput.click()
);

fileInput.addEventListener(
  'change',
  (e) => {

    const file =
      e.target.files[0];

    if (file)
      handleFile(file);
  }
);

['dragenter','dragover']
.forEach(evt => {

  dropzone.addEventListener(
    evt,
    (e) => {

      e.preventDefault();

      dropzone.classList.add(
        'dragover'
      );
    }
  );
});

['dragleave','drop']
.forEach(evt => {

  dropzone.addEventListener(
    evt,
    (e) => {

      e.preventDefault();

      dropzone.classList.remove(
        'dragover'
      );
    }
  );
});

dropzone.addEventListener(
  'drop',
  (e) => {

    const file =
      e.dataTransfer.files[0];

    if (file)
      handleFile(file);
  }
);

clearBtn.addEventListener(
  'click',
  () => {

    consoleBox.textContent = '';

    errorBox.textContent = '';

    setNotice(
      'Console cleared.',
      'ok'
    );
  }
);

runBtn.addEventListener(
  'click',
  async () => {

    if (!currentFile)
      return;

    setRunning(true);

    consoleBox.textContent =
      'Running...';

    errorBox.textContent = '';

    try {

      const form =
        new FormData();

      form.append(
        'file',
        currentFile
      );

      const res =
        await fetch(API_URL, {

          method: 'POST',

          body: form
        });

      const data =
        await res.json();

      if (data.success) {

        consoleBox.textContent =
          data.output || '';

        errorBox.textContent = '';

        setNotice(
          'Execution successful.',
          'ok'
        );

      } else {

        consoleBox.textContent = '';

        errorBox.textContent =
          data.error || 'Unknown error';

        setNotice(
          'Execution failed.',
          'error'
        );
      }

    } catch (err) {

      consoleBox.textContent = '';

      errorBox.textContent =
        String(err);

      setNotice(
        'Network/server error.',
        'error'
      );

    } finally {

      setRunning(false);
    }
  }
);

